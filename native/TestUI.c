#include "Ultrawide.c"
static BYTE originalCode[32768],menuArtBytes[1036800];
static void Fail(UINT code) {ExitProcess(code);}
void TestMain(void) {
    if(__builtin_offsetof(ID3D11DeviceContextVtbl,UpdateSubresource)/sizeof(void*)!=48)Fail(100+(UINT)(__builtin_offsetof(ID3D11DeviceContextVtbl,UpdateSubresource)/sizeof(void*)));
    Paths();
    WCHAR path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);lstrcatW(path,L"\\d3d11.dll");HMODULE module=LoadLibraryW(path);if(!module)Fail(1);
    CreateDeviceFn create=(CreateDeviceFn)GetProcAddress(module,"D3D11CreateDevice");if(!create)Fail(2);
    ID3D11Device *device=0;ID3D11DeviceContext *ctx=0;
    if(FAILED(create(0,D3D_DRIVER_TYPE_WARP,0,0,0,0,D3D11_SDK_VERSION,&device,0,&ctx)))Fail(3);
    Instrument(device,ctx);
    legacyUI=1;textCorrection=1; /* Exercise the experimental path explicitly. */
    D3D11_BUFFER_DESC unrelatedDesc={64,D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};ID3D11Buffer *unrelated=0;if(FAILED(ID3D11Device_CreateBuffer(device,&unrelatedDesc,0,&unrelated)))Fail(48);UINT unrelatedData[16]={0};LONG beforeUploads=mirrorCount;for(UINT upload=0;upload<700;++upload)ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)unrelated,0,0,unrelatedData,0,0);if(mirrorCount!=beforeUploads)Fail(49);ID3D11Buffer_Release(unrelated);
    if((*(void***)ctx)[48]!=(void*)UpdateHook)Fail(29);
    HANDLE file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\47897216336c5479-vs.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(4);
    DWORD size=0;ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);
    ID3D11VertexShader *vs=0;if(FAILED(ID3D11Device_CreateVertexShader(device,originalCode,size,0,&vs)))Fail(5);
    ID3D11DeviceContext_VSSetShader(ctx,vs,0,0);
    float matrix[64]={0};matrix[0]=2.0f/1920;matrix[5]=-2.0f/1080;matrix[10]=-.005f;matrix[12]=-1;matrix[13]=1;matrix[14]=.5f;matrix[15]=1;
    D3D11_BUFFER_DESC cbdesc={256,D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA initial={matrix,0,0};ID3D11Buffer *cb=0;
    if(FAILED(ID3D11Device_CreateBuffer(device,&cbdesc,&initial,&cb)))Fail(6);ID3D11DeviceContext_VSSetConstantBuffers(ctx,0,1,&cb);
    float vertices[40]={0};vertices[16]=1920;vertices[27]=1080;vertices[36]=1920;vertices[37]=1080;
    D3D11_BUFFER_DESC vbdesc={4*1024*1024,D3D11_USAGE_DYNAMIC,D3D11_BIND_VERTEX_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};initial.pSysMem=vertices;ID3D11Buffer *vb=0;
    if(FAILED(ID3D11Device_CreateBuffer(device,&vbdesc,0,&vb)))Fail(7);USHORT indices[12]={3,3,3,3,3,3,0,1,2,2,1,3};D3D11_BUFFER_DESC ibdesc={sizeof(indices),D3D11_USAGE_DEFAULT,D3D11_BIND_INDEX_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA idata={indices,0,0};ID3D11Buffer *ib=0;if(FAILED(ID3D11Device_CreateBuffer(device,&ibdesc,&idata,&ib)))Fail(30);ID3D11DeviceContext_IASetIndexBuffer(ctx,ib,DXGI_FORMAT_R16_UINT,12);D3D11_MAPPED_SUBRESOURCE firstMap;if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&firstMap)))Fail(39);BytesCopy(firstMap.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);UINT stride=40,offset=0;ID3D11DeviceContext_IASetVertexBuffers(ctx,0,1,&vb,&stride,&offset);
    D3D11_TEXTURE2D_DESC target={3440,1440,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_RENDER_TARGET,0,0};ID3D11Texture2D *texture=0;ID3D11RenderTargetView *rt=0;
    if(FAILED(ID3D11Device_CreateTexture2D(device,&target,0,&texture)))Fail(8);if(FAILED(ID3D11Device_CreateRenderTargetView(device,(ID3D11Resource*)texture,0,&rt)))Fail(9);
    ID3D11DeviceContext_OMSetRenderTargets(ctx,1,&rt,0);expandConstrainedScenes=1;D3D11_VIEWPORT vp={440,0,2560,1440,0,1};ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);
    UINT initialCount=1;D3D11_VIEWPORT widenedTest;ID3D11DeviceContext_RSGetViewports(ctx,&initialCount,&widenedTest);if(widenedTest.TopLeftX!=0||widenedTest.Width!=3440)Fail(50);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,const D3D11_VIEWPORT*))Original(ctx,contexts,contextCount)[44])(ctx,1,&vp);FixState state;if(ApplyUI(ctx,6,0,0,&state))Fail(10); /* A centered full-height quad (video pillarbox) must not be widened; only the menu composite is. */
    UINT n=1;D3D11_VIEWPORT actual;
    n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&actual);if(actual.TopLeftX!=440||actual.Width!=2560)Fail(12);
    ID3D11VertexShader *restored=0;ID3D11DeviceContext_VSGetShader(ctx,&restored,0,0);if(restored!=vs)Fail(13);ID3D11VertexShader_Release(restored);
    /* A narrow panel must not be mistaken for a full background. */
    vertices[16]=200;vertices[36]=200;D3D11_MAPPED_SUBRESOURCE map;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(14);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(ApplyUI(ctx,6,0,0,&state))Fail(15);
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\0b39f446865515f9-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(16);
    ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);ID3D11PixelShader *ps=0;
    if(FAILED(ID3D11Device_CreatePixelShader(device,originalCode,size,0,&ps)))Fail(17);ID3D11DeviceContext_PSSetShader(ctx,ps,0,0);
    if((*(void***)ctx)[48]!=(void*)UpdateHook)Fail(28);
    matrix[0]=2.0f/3440;matrix[5]=-2.0f/1440;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)cb,0,0,matrix,0,0);
    vp.TopLeftX=0;vp.Width=3440;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);
    vertices[6]=1030;vertices[7]=20;vertices[16]=1530;vertices[17]=20;vertices[26]=1030;vertices[27]=80;vertices[36]=1530;vertices[37]=80;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(18);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(GetShaderHash(ps)!=0x0b39f446865515f9ull)Fail(23);
    BufferMirror *check=Mirror((ID3D11Resource*)cb,0);if(!check)Fail(24);if(((float*)check->data)[0]!=matrix[0]){if(((float*)check->data)[0]==2.0f/1920)Fail(26);Fail(27);}
    check=Mirror((ID3D11Resource*)vb,0);if(!check||((float*)check->data)[6]!=1030)Fail(25);
    if(!ApplyUI(ctx,6,0,0,&state))Fail(19);
    ID3D11Buffer *fixCB=0;ID3D11DeviceContext_VSGetConstantBuffers(ctx,13,1,&fixCB);BufferMirror *fixMirror=Mirror((ID3D11Resource*)fixCB,1);InitialSnapshot(ctx,fixMirror);ID3D11Buffer_Release(fixCB);
    if(!fixMirror||((float*)fixMirror->data)[1]<.2558f||((float*)fixMirror->data)[1]>.2559f)Fail(20);
    RestoreUI(ctx,&state);
    for(UINT i=0;i<4;++i)vertices[i*10+6]+=1500; /* Entirely outside the 16:9 area: must be left alone. */
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(21);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(ApplyUI(ctx,6,0,0,&state))Fail(22);
    /* A small marker (<< or >>) inside the 16:9 area but away from its center must still be recentered. */
    vertices[6]=vertices[26]=740;vertices[16]=vertices[36]=800;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(60);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(!ApplyUI(ctx,6,0,0,&state))Fail(61);RestoreUI(ctx,&state);
    /* A cutscene subtitle the game already centered on the full screen must not be shifted again. */
    vertices[6]=vertices[26]=1192;vertices[16]=vertices[36]=2252;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(62);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(ApplyUI(ctx,6,0,0,&state))Fail(63);
    /* HUD changes must be reversible and leave the game's original buffer intact. */
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\e718d83b50978835-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(31);
    ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);ID3D11PixelShader *hudPS=0;
    if(FAILED(ID3D11Device_CreatePixelShader(device,originalCode,size,0,&hudPS)))Fail(32);ID3D11DeviceContext_PSSetShader(ctx,hudPS,0,0);
    static float hudData[516];hudData[508]=3440;hudData[509]=1440;hudData[510]=1.0f/3440;hudData[511]=1.0f/1440;
    D3D11_BUFFER_DESC hudDesc={sizeof(hudData),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};initial.pSysMem=hudData;ID3D11Buffer *hudCB=0;
    if(FAILED(ID3D11Device_CreateBuffer(device,&hudDesc,&initial,&hudCB)))Fail(33);ID3D11DeviceContext_PSSetConstantBuffers(ctx,1,1,&hudCB);
    for(int mode=0;mode<2;++mode){hudMode=mode;HUDState hs;if(!ApplyHUD(ctx,&hs))Fail(34);ID3D11Buffer *adjusted=0;ID3D11DeviceContext_PSGetConstantBuffers(ctx,1,1,&adjusted);BufferMirror *hm=Mirror((ID3D11Resource*)adjusted,1);InitialSnapshot(ctx,hm);if(!hm||!hm->valid)Fail(35);float *values=(float*)hm->data;if(!mode&&(values[504]<439.99f||values[504]>440.01f))Fail(36);if(mode&&values[509]!=1440)Fail(37);ID3D11Buffer_Release(adjusted);RestoreHUD(ctx,&hs);ID3D11DeviceContext_PSGetConstantBuffers(ctx,1,1,&adjusted);if(adjusted!=hudCB)Fail(38);ID3D11Buffer_Release(adjusted);}
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\8f3937c557a87704-vs.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(40);ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);ID3D11VertexShader *compositeVS=0;if(FAILED(ID3D11Device_CreateVertexShader(device,originalCode,size,0,&compositeVS)))Fail(41);ID3D11DeviceContext_VSSetShader(ctx,compositeVS,0,0);
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\5eae2582c19da3f5-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(42);ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);ID3D11PixelShader *compositePS=0;if(FAILED(ID3D11Device_CreatePixelShader(device,originalCode,size,0,&compositePS)))Fail(43);ID3D11DeviceContext_PSSetShader(ctx,compositePS,0,0);
    for(UINT i=0;i<64;++i)matrix[i]=0;matrix[8]=2.0f/3440;matrix[13]=-2.0f/1440;matrix[20]=-1;matrix[21]=1;matrix[23]=1;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)cb,0,0,matrix,0,0);
    vertices[6]=0;vertices[7]=0;vertices[16]=2560;vertices[17]=0;vertices[26]=0;vertices[27]=1440;vertices[36]=2560;vertices[37]=1440;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(44);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    if(!ApplyUI(ctx,6,0,0,&state))Fail(45);ID3D11DeviceContext_VSGetConstantBuffers(ctx,13,1,&fixCB);fixMirror=Mirror((ID3D11Resource*)fixCB,0);ID3D11Buffer_Release(fixCB);if(!fixMirror)Fail(46);float *compositeFix=(float*)fixMirror->data;if(compositeFix[0]!=1.34375f||compositeFix[1]<.34374f||compositeFix[1]>.34376f)Fail(47);RestoreUI(ctx,&state);
    D3D11_RECT fullScene={440,0,3000,1440},partialWidget={440,0,800,600},actualRect;UINT rectCount=1;
    ID3D11DeviceContext_RSSetScissorRects(ctx,1,&fullScene);ID3D11DeviceContext_RSGetScissorRects(ctx,&rectCount,&actualRect);if(actualRect.left!=0||actualRect.right!=3440)Fail(51);
    ID3D11DeviceContext_RSSetScissorRects(ctx,1,&partialWidget);rectCount=1;ID3D11DeviceContext_RSGetScissorRects(ctx,&rectCount,&actualRect);if(actualRect.left!=440||actualRect.right!=800||actualRect.bottom!=600)Fail(52);
    vp.TopLeftX=100;vp.Width=2560;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&actual);if(actual.TopLeftX!=100||actual.Width!=2560)Fail(53);
    legacyUI=0;if(ApplyUI(ctx,6,0,0,&state))Fail(54);
    expandConstrainedScenes=0;vp.TopLeftX=440;vp.Width=2560;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&actual);if(actual.TopLeftX!=440||actual.Width!=2560)Fail(55);
    ID3D11DeviceContext_RSSetScissorRects(ctx,1,&fullScene);rectCount=1;ID3D11DeviceContext_RSGetScissorRects(ctx,&rectCount,&actualRect);if(actualRect.left!=440||actualRect.right!=3000)Fail(56);
    vp.TopLeftX=0;vp.Width=3440;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&actual);if(actual.TopLeftX!=0||actual.Width!=3440)Fail(57);
    /* Captured briefing name + underline in the 1920x1080 layer. Their input
       x includes the wide screen margin, so only this pair moves left 330. */
    target.Width=1920;target.Height=1080;ID3D11Texture2D *labelTarget=0;ID3D11RenderTargetView *labelRT=0;if(FAILED(ID3D11Device_CreateTexture2D(device,&target,0,&labelTarget)))Fail(70);if(FAILED(ID3D11Device_CreateRenderTargetView(device,(ID3D11Resource*)labelTarget,0,&labelRT)))Fail(71);ID3D11DeviceContext_OMSetRenderTargets(ctx,1,&labelRT,0);
    vp.TopLeftX=vp.TopLeftY=0;vp.Width=1920;vp.Height=1080;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);D3D11_RECT labelClip={0,0,1920,1080};ID3D11DeviceContext_RSSetScissorRects(ctx,1,&labelClip);ID3D11DeviceContext_VSSetShader(ctx,vs,0,0);
    for(UINT i=0;i<64;++i)matrix[i]=0;matrix[0]=2.0f/1920;matrix[5]=-2.0f/1080;matrix[12]=-1;matrix[13]=1;matrix[15]=1;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)cb,0,0,matrix,0,0);
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\2d58759ed62f1f84-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(72);ReadFile(file,originalCode,sizeof(originalCode),&size,0);CloseHandle(file);ID3D11PixelShader *linePS=0;if(FAILED(ID3D11Device_CreatePixelShader(device,originalCode,size,0,&linePS)))Fail(73);
    target.Width=target.Height=1;target.BindFlags=D3D11_BIND_SHADER_RESOURCE;ID3D11Texture2D *lineTexture=0;ID3D11ShaderResourceView *lineView=0;if(FAILED(ID3D11Device_CreateTexture2D(device,&target,0,&lineTexture)))Fail(74);if(FAILED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)lineTexture,0,&lineView)))Fail(75);ID3D11DeviceContext_PSSetShaderResources(ctx,0,1,&lineView);
    vertices[6]=vertices[26]=1314;vertices[16]=vertices[36]=1389;vertices[7]=vertices[17]=540;vertices[27]=vertices[37]=541;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(76);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    ID3D11DeviceContext_PSSetShader(ctx,ps,0,0);if(ApplyUI(ctx,6,0,0,&state))Fail(77); /* Ordinary unpaired glyphs stay put. */
    ID3D11DeviceContext_PSSetShader(ctx,linePS,0,0);if(!ApplyUI(ctx,6,0,0,&state))Fail(78);ID3D11DeviceContext_VSGetConstantBuffers(ctx,13,1,&fixCB);fixMirror=Mirror((ID3D11Resource*)fixCB,0);ID3D11Buffer_Release(fixCB);if(!fixMirror||((float*)fixMirror->data)[1]<-.34376f||((float*)fixMirror->data)[1]>-.34374f)Fail(79);RestoreUI(ctx,&state);
    vertices[7]=vertices[17]=547;vertices[27]=vertices[37]=561;vertices[16]=vertices[36]=1388;if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(80);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);ID3D11DeviceContext_PSSetShader(ctx,ps,0,0);
    if(!ApplyUI(ctx,6,0,0,&state))Fail(81);ID3D11DeviceContext_VSGetConstantBuffers(ctx,13,1,&fixCB);fixMirror=Mirror((ID3D11Resource*)fixCB,0);ID3D11Buffer_Release(fixCB);if(!fixMirror||((float*)fixMirror->data)[1]<-.34376f||((float*)fixMirror->data)[1]>-.34374f)Fail(82);RestoreUI(ctx,&state);if(ApplyUI(ctx,6,0,0,&state))Fail(83); /* Consumed pair cannot move unrelated text. */
    labelClip.left=1314;labelClip.top=540;labelClip.right=1389;labelClip.bottom=570;ID3D11DeviceContext_RSSetScissorRects(ctx,1,&labelClip);projectedLabel.context=ctx;projectedLabel.tick=GetTickCount64();if(ApplyUI(ctx,6,0,0,&state))Fail(84);
    /* Exact known artwork recognition, full-layer copy propagation, and invalidation. */
    file=CreateFileW(L"C:\\Users\\aids7\\dev\\acecombat7-ultrawide\\native\\build\\menu-background.bc1",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(file==INVALID_HANDLE_VALUE)Fail(85);ReadFile(file,menuArtBytes,sizeof(menuArtBytes),&size,0);CloseHandle(file);if(size!=sizeof(menuArtBytes))Fail(86);
    D3D11_TEXTURE2D_DESC artDesc={1920,1080,1,1,DXGI_FORMAT_BC1_UNORM,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE,0,0};D3D11_SUBRESOURCE_DATA artData={menuArtBytes,3840,0};ID3D11Texture2D *art=0,*unrelatedArt=0;ID3D11ShaderResourceView *artView=0;
    if(FAILED(ID3D11Device_CreateTexture2D(device,&artDesc,&artData,&art))||!MenuArt((ID3D11Resource*)art))Fail(87);
    menuArtBytes[0]^=1;if(FAILED(ID3D11Device_CreateTexture2D(device,&artDesc,&artData,&unrelatedArt))||MenuArt((ID3D11Resource*)unrelatedArt))Fail(88);
    menuArtBytes[0]^=1;ID3D11Texture2D *lateArt=0;if(FAILED(ID3D11Device_CreateTexture2D(device,&artDesc,0,&lateArt))||MenuArt((ID3D11Resource*)lateArt))Fail(96);
    ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)lateArt,0,0,menuArtBytes,3840,0);if(!MenuArt((ID3D11Resource*)lateArt))Fail(97);
    menuArtBytes[0]^=1;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)lateArt,0,0,menuArtBytes,3840,0);if(MenuArt((ID3D11Resource*)lateArt))Fail(98);
    menuArtBytes[0]^=1;((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT,const D3D11_BOX*,const void*,UINT,UINT))Original(ctx,contexts,contextCount)[48])(ctx,(ID3D11Resource*)lateArt,0,0,menuArtBytes,3840,0);
    LONG probeBefore=menuProbeCount;if(!DetectMenuArt(ctx,(ID3D11Resource*)lateArt)||menuProbeCount!=probeBefore+1)Fail(99);if(!DetectMenuArt(ctx,(ID3D11Resource*)lateArt)||menuProbeCount!=probeBefore+1)Fail(100);
    ID3D11Texture2D *copiedArt=0;if(FAILED(ID3D11Device_CreateTexture2D(device,&artDesc,0,&copiedArt)))Fail(101);ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)copiedArt,(ID3D11Resource*)art);if(!MenuArt((ID3D11Resource*)copiedArt))Fail(102);
    D3D11_BOX artPart={0,0,0,960,1080,1};ID3D11DeviceContext_CopySubresourceRegion(ctx,(ID3D11Resource*)copiedArt,0,0,0,0,(ID3D11Resource*)art,0,&artPart);if(MenuArt((ID3D11Resource*)copiedArt))Fail(103);
    ID3D11DeviceContext_CopySubresourceRegion(ctx,(ID3D11Resource*)copiedArt,0,0,0,0,(ID3D11Resource*)art,0,0);if(!MenuArt((ID3D11Resource*)copiedArt))Fail(104);
    if(FAILED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)art,0,&artView)))Fail(89);ID3D11DeviceContext_PSSetShaderResources(ctx,0,1,&artView);ID3D11DeviceContext_PSSetShader(ctx,linePS,0,0);ID3D11DeviceContext_VSSetShader(ctx,vs,0,0);
    labelClip=(D3D11_RECT){0,0,1920,1080};ID3D11DeviceContext_RSSetScissorRects(ctx,1,&labelClip);
    vertices[6]=vertices[26]=0;vertices[16]=vertices[36]=1920;vertices[7]=vertices[17]=0;vertices[27]=vertices[37]=1080;
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)vb,0,D3D11_MAP_WRITE_DISCARD,0,&map)))Fail(90);BytesCopy(map.pData,vertices,sizeof(vertices));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)vb,0);
    NoteMenuLayer(ctx,6,0,0);if(!MenuArt((ID3D11Resource*)labelTarget))Fail(91);
    NoteMenuLayer(ctx,12,0,0);if(MenuArt((ID3D11Resource*)labelTarget)||!MenuBackground((ID3D11Resource*)labelTarget))Fail(92);
    labelClip.right=100;ID3D11DeviceContext_RSSetScissorRects(ctx,1,&labelClip);NoteMenuLayer(ctx,6,0,0);if(MenuArt((ID3D11Resource*)labelTarget))Fail(93);
    labelClip.right=1920;ID3D11DeviceContext_RSSetScissorRects(ctx,1,&labelClip);NoteMenuLayer(ctx,6,0,0);if(!MenuArt((ID3D11Resource*)labelTarget))Fail(94);
    const float clear[4]={0};ID3D11DeviceContext_ClearRenderTargetView(ctx,labelRT,clear);if(MenuArt((ID3D11Resource*)labelTarget))Fail(95);
    /* A normal Steam screenshot must not arm logging or GPU capture when
       diagnostics are disabled, even though F12 remains a screenshot key. */
    diagnostics=0;LONG markerBefore=drawLogMarker,budgetBefore=drawLogBudget,rawBefore=rawDumpBudget,hudBefore=hudDumpBudget;
    ArmDrawLog("release screenshot");if(drawLogMarker!=markerBefore||drawLogBudget!=budgetBefore||rawDumpBudget!=rawBefore||hudDumpBudget!=hudBefore)Fail(105);
    ExitProcess(0);
}
