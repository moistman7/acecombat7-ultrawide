#define COBJMACROS
#define CINTERFACE
#include <d3d11_4.h>
#include <stdint.h>

typedef struct { void *object; void **original; ULONGLONG menuEpoch; UINT menuMasks; } HookObject;
static HookObject devices[16], contexts[32];
static LONG deviceCount,contextCount,captureBudget;
static WCHAR diagnosticPath[MAX_PATH];
static LONG shaderCount;
static int diagnostics;
static int hudWidth=3440,hudHeight=1440;
static LONG textureCapture;
static ID3D11DeviceContext *textureCaptureContext;
static SRWLOCK hookLock=SRWLOCK_INIT;
typedef struct { void *shader; uint64_t hash; } ShaderEntry;
static ShaderEntry shaders[8192];
static void UILog(const char *s) { Log(s); }
static uint64_t ShaderHash(const BYTE *p,SIZE_T size) {
    uint64_t h=0xcbf29ce484222325ull;
    for(SIZE_T i=0;i<size;++i){h*=0x100000001b3ull;h^=p[i];}return h;
}
static void SaveData(const WCHAR *name,const void *data,DWORD size) {
    WCHAR path[MAX_PATH];lstrcpyW(path,diagnosticPath);lstrcatW(path,name);
    HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(f!=INVALID_HANDLE_VALUE){DWORD wrote;WriteFile(f,data,size,&wrote,0);CloseHandle(f);}
}
static void HexName(WCHAR *out,uint64_t hash) {
    const WCHAR *hex=L"0123456789abcdef";
    for(int i=0;i<16;++i)out[i]=hex[(hash >> ((15-i)*4))&15];out[16]=0;
}
static void CaptureTexture(ID3D11DeviceContext *ctx,ID3D11Resource *resource,const WCHAR *stem) {
    if(!resource)return;D3D11_RESOURCE_DIMENSION dimension;ID3D11Resource_GetType(resource,&dimension);if(dimension!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return;
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);if(desc.SampleDesc.Count!=1||desc.Width>8192||desc.Height>8192)return;
    UINT captureRows=((desc.Format>=70&&desc.Format<=84)||(desc.Format>=94&&desc.Format<=99))?(desc.Height+3)/4:desc.Height;
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);ID3D11Texture2D *copy=0;
    if(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&copy))){ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)copy,resource);D3D11_MAPPED_SUBRESOURCE map;if(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)copy,0,D3D11_MAP_READ,0,&map))){WCHAR name[100];lstrcpyW(name,stem);lstrcatW(name,L"-pixels.bin");SaveData(name,map.pData,map.RowPitch*captureRows);UINT info[4]={desc.Width,desc.Height,desc.Format,map.RowPitch};lstrcpyW(name,stem);lstrcatW(name,L"-image.bin");SaveData(name,info,sizeof(info));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)copy,0);}ID3D11Texture2D_Release(copy);}ID3D11Device_Release(device);
}
static void CaptureTarget(ID3D11DeviceContext *ctx,const WCHAR *stem) {
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(rt){ID3D11Resource *resource=0;ID3D11RenderTargetView_GetResource(rt,&resource);CaptureTexture(ctx,resource,stem);ID3D11Resource_Release(resource);ID3D11RenderTargetView_Release(rt);}
}
static uint64_t GetShaderHash(void *shader) {
    if(!shader)return 0;UINT start=(UINT)(((uintptr_t)shader>>4)&8191);
    for(UINT n=0;n<8192;++n){ShaderEntry *entry=&shaders[(start+n)&8191];if(entry->shader==shader)return entry->hash;if(!entry->shader)return 0;}return 0;
}
static void RegisterShader(void *shader,const void *code,SIZE_T size,const WCHAR *kind) {
    uint64_t h=ShaderHash(code,size);InterlockedIncrement(&shaderCount);UINT start=(UINT)(((uintptr_t)shader>>4)&8191);
    for(UINT n=0;n<8192;++n){ShaderEntry *entry=&shaders[(start+n)&8191];void *key=InterlockedCompareExchangePointer(&entry->shader,shader,0);if(!key||key==shader){InterlockedExchange64((LONG64*)&entry->hash,(LONG64)h);break;}}
    if(diagnostics && size<32768) {
        WCHAR name[50];HexName(name,h);lstrcatW(name,kind);SaveData(name,code,(DWORD)size);
    }
}
static void **Original(void *object,HookObject *entries,LONG count) {
    for(LONG i=0;i<count;++i)if(entries[i].object==*(void***)object)return entries[i].original;return 0;
}
typedef HRESULT (STDMETHODCALLTYPE *CreateShaderFn)(ID3D11Device*,const void*,SIZE_T,ID3D11ClassLinkage*,void**);
static HRESULT STDMETHODCALLTYPE CreateVS(ID3D11Device *device,const void *code,SIZE_T size,ID3D11ClassLinkage *link,ID3D11VertexShader **out) {
    void **v=Original(device,devices,deviceCount);HRESULT hr=((CreateShaderFn)v[12])(device,code,size,link,(void**)out);
    if(SUCCEEDED(hr)&&out&&*out)RegisterShader(*out,code,size,L"-vs.cso");return hr;
}
static HRESULT STDMETHODCALLTYPE CreatePS(ID3D11Device *device,const void *code,SIZE_T size,ID3D11ClassLinkage *link,ID3D11PixelShader **out) {
    void **v=Original(device,devices,deviceCount);HRESULT hr=((CreateShaderFn)v[15])(device,code,size,link,(void**)out);
    if(SUCCEEDED(hr)&&out&&*out)RegisterShader(*out,code,size,L"-ps.cso");return hr;
}
static void DumpBuffer(ID3D11DeviceContext *context,ID3D11Buffer *buffer,const WCHAR *name) {
    if(!buffer)return;D3D11_BUFFER_DESC desc;ID3D11Buffer_GetDesc(buffer,&desc);
    if(desc.ByteWidth>131072)desc.ByteWidth=131072;
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(context,&device);
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    ID3D11Buffer *copy=0;
    if(SUCCEEDED(ID3D11Device_CreateBuffer(device,&desc,0,&copy))) {
        D3D11_BOX region={0,0,0,desc.ByteWidth,1,1};ID3D11DeviceContext_CopySubresourceRegion(context,(ID3D11Resource*)copy,0,0,0,0,(ID3D11Resource*)buffer,0,&region);
        D3D11_MAPPED_SUBRESOURCE map;
        if(SUCCEEDED(ID3D11DeviceContext_Map(context,(ID3D11Resource*)copy,0,D3D11_MAP_READ,0,&map))) {
            SaveData(name,map.pData,desc.ByteWidth);ID3D11DeviceContext_Unmap(context,(ID3D11Resource*)copy,0);
        }
        ID3D11Buffer_Release(copy);
    }
    ID3D11Device_Release(device);
}
static void InspectDraw(ID3D11DeviceContext *context,UINT count,UINT first,INT base) {
    if(captureBudget<=0)return;
    ID3D11Buffer *cb=0;ID3D11DeviceContext_VSGetConstantBuffers(context,0,1,&cb);if(!cb)return;
    D3D11_BUFFER_DESC desc;ID3D11Buffer_GetDesc(cb,&desc);
    if(desc.ByteWidth>256){ID3D11Buffer_Release(cb);return;}
    ID3D11VertexShader *vs=0;ID3D11PixelShader *ps=0;
    ID3D11DeviceContext_VSGetShader(context,&vs,0,0);ID3D11DeviceContext_PSGetShader(context,&ps,0,0);
    uint64_t vhash=GetShaderHash(vs),phash=GetShaderHash(ps);
    if(vhash==0x8f3937c557a87704ull&&phash==0x5eae2582c19da3f5ull&&InterlockedCompareExchange(&textureCapture,0,1)==1){ID3D11ShaderResourceView *srv=0;ID3D11DeviceContext_PSGetShaderResources(context,0,1,&srv);if(srv){ID3D11Resource *source=0;ID3D11ShaderResourceView_GetResource(srv,&source);CaptureTexture(context,source,L"composite-source");ID3D11Resource_Release(source);ID3D11ShaderResourceView_Release(srv);}CaptureTarget(context,L"composite-before");textureCaptureContext=context;}
    if(vhash!=0x47897216336c5479ull&&vhash!=0x8f3937c557a87704ull){ID3D11Buffer_Release(cb);if(vs)ID3D11VertexShader_Release(vs);if(ps)ID3D11PixelShader_Release(ps);return;}
    LONG number=InterlockedDecrement(&captureBudget);if(number<0){ID3D11Buffer_Release(cb);if(vs)ID3D11VertexShader_Release(vs);if(ps)ID3D11PixelShader_Release(ps);return;}
    WCHAR prefix[100],num[17],name[140];HexName(num,(uint64_t)number);lstrcpyW(prefix,num+12);lstrcatW(prefix,L"-");HexName(num,vhash);lstrcatW(prefix,num);lstrcatW(prefix,L"-");HexName(num,phash);lstrcatW(prefix,num);
    lstrcpyW(name,prefix);lstrcatW(name,L"-cb.bin");DumpBuffer(context,cb,name);ID3D11Buffer_Release(cb);
    ID3D11Buffer *vb=0,*ib=0;UINT stride=0,offset=0,ibOffset=0;DXGI_FORMAT format;
    ID3D11DeviceContext_IAGetVertexBuffers(context,0,1,&vb,&stride,&offset);
    ID3D11DeviceContext_IAGetIndexBuffer(context,&ib,&format,&ibOffset);
    lstrcpyW(name,prefix);lstrcatW(name,L"-vb.bin");DumpBuffer(context,vb,name);
    lstrcpyW(name,prefix);lstrcatW(name,L"-ib.bin");DumpBuffer(context,ib,name);
    UINT info[8]={count,first,(UINT)base,stride,offset,ibOffset,(UINT)format,desc.ByteWidth};
    lstrcpyW(name,prefix);lstrcatW(name,L"-draw.bin");SaveData(name,info,sizeof(info));
    D3D11_VIEWPORT viewport[16];UINT viewportCount=16;ID3D11DeviceContext_RSGetViewports(context,&viewportCount,viewport);lstrcpyW(name,prefix);lstrcatW(name,L"-viewport.bin");SaveData(name,viewport,viewportCount*sizeof(D3D11_VIEWPORT));
    D3D11_RECT scissors[16];UINT scissorCount=16;ID3D11DeviceContext_RSGetScissorRects(context,&scissorCount,scissors);lstrcpyW(name,prefix);lstrcatW(name,L"-scissor.bin");SaveData(name,scissors,scissorCount*sizeof(D3D11_RECT));
    ID3D11RenderTargetView *captureRT=0;ID3D11DeviceContext_OMGetRenderTargets(context,1,&captureRT,0);
    if(captureRT){ID3D11Resource *targetResource=0;ID3D11RenderTargetView_GetResource(captureRT,&targetResource);D3D11_RESOURCE_DIMENSION dimension;ID3D11Resource_GetType(targetResource,&dimension);if(dimension==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC targetDesc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)targetResource,&targetDesc);lstrcpyW(name,prefix);lstrcatW(name,L"-target.bin");SaveData(name,&targetDesc,sizeof(targetDesc));}ID3D11Resource_Release(targetResource);ID3D11RenderTargetView_Release(captureRT);}
    if(vb)ID3D11Buffer_Release(vb);if(ib)ID3D11Buffer_Release(ib);if(vs)ID3D11VertexShader_Release(vs);if(ps)ID3D11PixelShader_Release(ps);
}
static void MenuTextureUpload(ID3D11Resource*,UINT,const D3D11_BOX*,const void*,UINT);
#include "UIAdjust.c"
#include "HUDFix.c"
#include "ViewportFix.c"
static void ResetMenuMatte(ID3D11Resource*);
static void CopyMenuMatte(ID3D11DeviceContext*,ID3D11Resource*,UINT,UINT,UINT,UINT,ID3D11Resource*,UINT,const D3D11_BOX*);
#include "MenuLayers.c"
#include "MenuMatte.c"
#include "MenuEdges.c"
#include "CompositeDiagnostics.c"
#include "DrawLog.c"
#include "MenuTrace.c"
#include "MenuCapture.c"
static void STDMETHODCALLTYPE DrawIndexedHook(ID3D11DeviceContext *context,UINT count,UINT first,INT base) {
    NoteMenuLayer(context,count,first,base);CaptureMenuLayer(context,count,first,base,1);InspectSceneComposite(context);InspectDraw(context,count,first,base);LogDraw(context,0,count,first,base);NoteMenuWorldDraw(context,count);if(skipPlate&&ShouldSkipPlate(context))return;void **v=Original(context,contexts,contextCount);
    int inheritedMatte=menuBlackBars&&(menuEdgeExtension||menuFit)&&InheritMenuMatte(context,count);MenuEdgeState edge;int changedEdge=ApplyMenuEdges(context,&edge);FixState state;int changed=ApplyUI(context,count,first,base,&state);
    HUDState hud;int changedHUD=ApplyHUD(context,&hud);
    ((void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,INT))v[12])(context,count,first,base);
    if(textureCaptureContext==context){CaptureTarget(context,L"composite-after");textureCaptureContext=0;}
    if(changed)RestoreUI(context,&state);
    if(changedHUD)RestoreHUD(context,&hud);CaptureSceneAfter(context);if(changedEdge)RestoreMenuEdges(context,&edge);if(inheritedMatte&&(!changedEdge||!edge.matte))ApplyMenuMatte(context);
}
static void STDMETHODCALLTYPE DrawHook(ID3D11DeviceContext *context,UINT count,UINT first) {
    NoteMenuLayer(context,0,0,0);CaptureMenuLayer(context,count,first,0,0);InspectSceneComposite(context);InspectDraw(context,count,first,0);LogDraw(context,1,count,first,0);NoteMenuWorldDraw(context,count);void **v=Original(context,contexts,contextCount);
    int inheritedMatte=menuBlackBars&&(menuEdgeExtension||menuFit)&&InheritMenuMatte(context,count);MenuEdgeState edge;int changedEdge=ApplyMenuEdges(context,&edge);HUDState hud;int changed=ApplyHUD(context,&hud);
    ((void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))v[13])(context,count,first);
    if(changed)RestoreHUD(context,&hud);CaptureSceneAfter(context);if(changedEdge)RestoreMenuEdges(context,&edge);if(inheritedMatte&&(!changedEdge||!edge.matte))ApplyMenuMatte(context);
}
static void STDMETHODCALLTYPE DrawIndexedInstancedHook(ID3D11DeviceContext *context,UINT indices,UINT instances,UINT first,INT base,UINT startInstance) {
    NoteMenuLayer(context,0,0,0);InspectSceneComposite(context);LogDraw(context,2,indices,first,base);if(instances)NoteMenuWorldDraw(context,indices);void **v=Original(context,contexts,contextCount);int inheritedMatte=instances&&menuBlackBars&&(menuEdgeExtension||menuFit)&&InheritMenuMatte(context,indices);MenuEdgeState edge;int changedEdge=ApplyMenuEdges(context,&edge);HUDState hud;int changed=ApplyHUD(context,&hud);
    ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,UINT,INT,UINT))v[20])(context,indices,instances,first,base,startInstance);
    if(changed)RestoreHUD(context,&hud);CaptureSceneAfter(context);if(changedEdge)RestoreMenuEdges(context,&edge);if(inheritedMatte&&(!changedEdge||!edge.matte))ApplyMenuMatte(context);
}
static void STDMETHODCALLTYPE DrawInstancedHook(ID3D11DeviceContext *context,UINT vertices,UINT instances,UINT first,UINT startInstance) {
    NoteMenuLayer(context,0,0,0);InspectSceneComposite(context);LogDraw(context,3,vertices,first,0);if(instances)NoteMenuWorldDraw(context,vertices);void **v=Original(context,contexts,contextCount);int inheritedMatte=instances&&menuBlackBars&&(menuEdgeExtension||menuFit)&&InheritMenuMatte(context,vertices);MenuEdgeState edge;int changedEdge=ApplyMenuEdges(context,&edge);HUDState hud;int changed=ApplyHUD(context,&hud);
    ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,UINT,UINT))v[21])(context,vertices,instances,first,startInstance);
    if(changed)RestoreHUD(context,&hud);CaptureSceneAfter(context);if(changedEdge)RestoreMenuEdges(context,&edge);if(inheritedMatte&&(!changedEdge||!edge.matte))ApplyMenuMatte(context);
}
static void Instrument(ID3D11Device *device,ID3D11DeviceContext *context) {
    AcquireSRWLockExclusive(&hookLock);
    if(device && deviceCount<16 && !Original(device,devices,deviceCount)) {
        LONG i=deviceCount;void **v=*(void***)device;void **copy=HeapAlloc(GetProcessHeap(),0,43*sizeof(void*));
        if(copy){for(int j=0;j<43;++j)copy[j]=v[j];devices[i].object=v;devices[i].original=copy;InterlockedIncrement(&deviceCount);DWORD old,ignore;if(VirtualProtect(v+12,4*sizeof(void*),PAGE_READWRITE,&old)){InterlockedExchangePointer(v+12,CreateVS);InterlockedExchangePointer(v+15,CreatePS);VirtualProtect(v+12,4*sizeof(void*),old,&ignore);}if(VirtualProtect(v+5,sizeof(void*),PAGE_READWRITE,&old)){InterlockedExchangePointer(v+5,CreateMenuTexture);VirtualProtect(v+5,sizeof(void*),old,&ignore);}}
    }
    if(context && contextCount<32 && !Original(context,contexts,contextCount)) {
        LONG i=contextCount;void **v=*(void***)context;SIZE_T bytes=sizeof(ID3D11DeviceContextVtbl);void *advanced=0;
        if(SUCCEEDED(ID3D11DeviceContext_QueryInterface(context,&IID_ID3D11DeviceContext4,&advanced))){if(advanced==context)bytes=sizeof(ID3D11DeviceContext4Vtbl);ID3D11DeviceContext_Release((ID3D11DeviceContext*)advanced);}
        else if(SUCCEEDED(ID3D11DeviceContext_QueryInterface(context,&IID_ID3D11DeviceContext3,&advanced))){if(advanced==context)bytes=sizeof(ID3D11DeviceContext3Vtbl);ID3D11DeviceContext_Release((ID3D11DeviceContext*)advanced);}
        else if(SUCCEEDED(ID3D11DeviceContext_QueryInterface(context,&IID_ID3D11DeviceContext2,&advanced))){if(advanced==context)bytes=sizeof(ID3D11DeviceContext2Vtbl);ID3D11DeviceContext_Release((ID3D11DeviceContext*)advanced);}
        else if(SUCCEEDED(ID3D11DeviceContext_QueryInterface(context,&IID_ID3D11DeviceContext1,&advanced))){if(advanced==context)bytes=sizeof(ID3D11DeviceContext1Vtbl);ID3D11DeviceContext_Release((ID3D11DeviceContext*)advanced);}
        void **copy=HeapAlloc(GetProcessHeap(),0,bytes);
        if(copy){for(SIZE_T j=0;j<bytes/sizeof(void*);++j)copy[j]=v[j];contexts[i].object=copy;contexts[i].original=v;InterlockedIncrement(&contextCount);copy[12]=DrawIndexedHook;copy[13]=DrawHook;copy[14]=MapHook;copy[15]=UnmapHook;copy[44]=ViewportHook;copy[45]=ScissorHook;copy[46]=CopyMenuRegion;copy[47]=CopyMenuResource;copy[48]=UpdateHook;copy[50]=ClearMenuTarget;copy[20]=DrawIndexedInstancedHook;copy[21]=DrawInstancedHook;if(bytes>=sizeof(ID3D11DeviceContext1Vtbl))copy[__builtin_offsetof(ID3D11DeviceContext1Vtbl,UpdateSubresource1)/sizeof(void*)]=Update1Hook;InterlockedExchangePointer((void**)context,copy);}
    }
    ReleaseSRWLockExclusive(&hookLock);
    UILog("Native D3D11 device/context observed.\r\n");
}
typedef HRESULT (WINAPI *CreateDeviceFn)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
static CreateDeviceFn originalCreateDevice;
static HRESULT WINAPI CreateDeviceHook(IDXGIAdapter*a,D3D_DRIVER_TYPE b,HMODULE c,UINT d,const D3D_FEATURE_LEVEL*e,UINT f,UINT g,ID3D11Device**h,D3D_FEATURE_LEVEL*i,ID3D11DeviceContext**j) {
    HRESULT hr=originalCreateDevice(a,b,c,d,e,f,g,h,i,j);
    if(SUCCEEDED(hr))Instrument(h?*h:0,j?*j:0);return hr;
}
static DWORD WINAPI UIWorker(void *unused) {
    (void)unused;WCHAR request[MAX_PATH];lstrcpyW(request,diagnosticPath);lstrcatW(request,L"capture.request");
    for(;;){if(diagnostics&&GetFileAttributesW(request)!=INVALID_FILE_ATTRIBUTES){DeleteFileW(request);InterlockedExchange(&captureBudget,120);InterlockedExchange(&textureCapture,1);InterlockedExchange(&sceneCapture,1);primaryTraceCount=0;primaryTraceDraws=0;primaryTraceEnabled=1;UILog("Native UI capture requested.\r\n");ArmDrawLog("file capture");}DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F6)&1)){LONG mode=InterlockedXor(&hudMode,1)^1;WritePrivateProfileStringW(L"Ultrawide",L"HUDMode",mode?L"Screen":L"16:9",ini);UILog(mode?"HUD mode: Screen.\r\n":"HUD mode: centered 16:9.\r\n");}if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F7)&1)){LONG enabled=InterlockedXor(&menuEdgeExtension,1)^1;WritePrivateProfileStringW(L"Ultrawide",L"MenuEdgeExtension",enabled?L"1":L"0",ini);UILog(enabled?"Menu edge extension: on.\r\n":"Menu edge extension: off.\r\n");}{int f8=GetAsyncKeyState(VK_F8)&1,f12=GetAsyncKeyState(VK_F12)&1;if(process==GetCurrentProcessId()&&(f8||f12))ArmDrawLog(f12?"F12 screenshot":"F8");}
        if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F5)&1)){LONG on=InterlockedXor(&menuSpan,1)^1;WritePrivateProfileStringW(L"Ultrawide",L"MenuSpan",on?L"1":L"0",ini);StampLog(on?"F5: hangar menus spread to the screen edges.\r\n":"F5: hangar menus centered.\r\n");}
        if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F9)&1)){LONG on=InterlockedXor(&forceWiden,1)^1;StampLog(on?"F9: forced menu-background widening ON.\r\n":"F9: forced menu-background widening OFF.\r\n");}
        if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F10)&1)){LONG on=InterlockedXor(&skipPlate,1)^1;StampLog(on?"F10: UI-plane composite hidden.\r\n":"F10: UI-plane composite shown.\r\n");}
        if(process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F11)&1)){InterlockedExchange(&bailBudget,60);StampLog("---- F11: logging ApplyUI decisions for the 5eae background ----\r\n");}
        Sleep(100);}return 0;
}
static void InstallUIHooks(void) {
    diagnostics=GetPrivateProfileIntW(L"Ultrawide",L"Diagnostics",0,ini)!=0;
    primaryTraceEnabled=diagnostics&&GetPrivateProfileIntW(L"Ultrawide",L"AutoCompositeCapture",0,ini)!=0;
    sceneCapture=diagnostics&&GetPrivateProfileIntW(L"Ultrawide",L"AutoCompositeCapture",0,ini)!=0;
    menuEdgeExtension=GetPrivateProfileIntW(L"Ultrawide",L"MenuEdgeExtension",1,ini)!=0;
    menuSpan=GetPrivateProfileIntW(L"Ultrawide",L"MenuSpan",0,ini)!=0;
    menuBlackBars=GetPrivateProfileIntW(L"Ultrawide",L"MenuBlackBars",0,ini)!=0;
    menuFit=GetPrivateProfileIntW(L"Ultrawide",L"MenuFit",1,ini)!=0;
    expandConstrainedScenes=GetPrivateProfileIntW(L"Ultrawide",L"ExpandConstrainedScenes",0,ini)!=0;
    legacyUI=GetPrivateProfileIntW(L"Ultrawide",L"ExperimentalLegacyUI",0,ini)!=0;
    textCorrection=GetPrivateProfileIntW(L"Ultrawide",L"ExperimentalTextCorrection",0,ini)!=0;
    WCHAR mode[32];GetPrivateProfileStringW(L"Ultrawide",L"HUDMode",L"16:9",mode,32,ini);hudMode=!lstrcmpiW(mode,L"Screen");hudWidth=GetPrivateProfileIntW(L"Ultrawide",L"Width",3440,ini);hudHeight=GetPrivateProfileIntW(L"Ultrawide",L"Height",1440,ini);
    lstrcpyW(diagnosticPath,ini);WCHAR *last=diagnosticPath;for(WCHAR*p=diagnosticPath;*p;++p)if(*p==L'\\')last=p+1;*last=0;lstrcatW(diagnosticPath,L"Ace7NativeUI\\");CreateDirectoryW(diagnosticPath,0);
    BYTE*base=(BYTE*)GetModuleHandleW(0);IMAGE_DOS_HEADER*dos=(void*)base;IMAGE_NT_HEADERS64*nt=(void*)(base+dos->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR*imports=(void*)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(;imports->Name;++imports) {
        if(lstrcmpiA((char*)base+imports->Name,"d3d11.dll"))continue;
        IMAGE_THUNK_DATA64 *names=(void*)(base+imports->OriginalFirstThunk),*slots=(void*)(base+imports->FirstThunk);
        for(;names->u1.AddressOfData;++names,++slots) {
            if(IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal))continue;
            IMAGE_IMPORT_BY_NAME*name=(void*)(base+names->u1.AddressOfData);
            if(lstrcmpA((char*)name->Name,"D3D11CreateDevice"))continue;
            DWORD old,ignore;if(VirtualProtect(&slots->u1.Function,sizeof(void*),PAGE_READWRITE,&old)) {
                originalCreateDevice=(CreateDeviceFn)(uintptr_t)slots->u1.Function;
                InterlockedExchangePointer((void**)&slots->u1.Function,CreateDeviceHook);VirtualProtect(&slots->u1.Function,sizeof(void*),old,&ignore);
                UILog("Standalone D3D11 import hook installed.\r\n");
            }
        }
    }
    HANDLE t=CreateThread(0,0,UIWorker,0,0,0);if(t)CloseHandle(t);
}
