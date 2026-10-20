#include "UI4.h"
#include "UI6.h"
typedef struct { ID3D11Resource *resource; BYTE *data; UINT size; void *mapped; int valid; } BufferMirror;
static BufferMirror mirrors[512];
static LONG mirrorCount;
static UINT mirrorBytes;
static LONG backgroundHits,textHits;
static int textCorrection;
static int legacyUI;
static LONG forceWiden,bailBudget;
static void BailLog(ID3D11DeviceContext *ctx,int line);
#define UIBAIL() do{if(six&&bailBudget>0)BailLog(ctx,__LINE__);}while(0)
static SRWLOCK mirrorLock=SRWLOCK_INIT;
static void BytesCopy(void *dst,const void *src,UINT count) {for(UINT i=0;i<count;++i)((BYTE*)dst)[i]=((const BYTE*)src)[i];}
static BufferMirror *Mirror(ID3D11Resource *resource,int add) {
    if(!resource)return 0;
    for(LONG i=0;i<mirrorCount;++i)if(mirrors[i].resource==resource)return &mirrors[i];
    if(!add || mirrorCount>=512)return 0;
    D3D11_RESOURCE_DIMENSION dimension;ID3D11Resource_GetType(resource,&dimension);if(dimension!=D3D11_RESOURCE_DIMENSION_BUFFER)return 0;
    D3D11_BUFFER_DESC desc;ID3D11Buffer_GetDesc((ID3D11Buffer*)resource,&desc);
    if(!desc.ByteWidth)return 0;
    /* UE's transient UI allocator uses large shared buffers. Retain only the
       prefix needed by these small draws, never the entire allocator. */
    if(desc.ByteWidth>131072)desc.ByteWidth=131072;
    AcquireSRWLockExclusive(&mirrorLock);
    for(LONG i=0;i<mirrorCount;++i)if(mirrors[i].resource==resource){ReleaseSRWLockExclusive(&mirrorLock);return &mirrors[i];}
    BufferMirror *result=0;
    if(mirrorCount<512 && mirrorBytes+desc.ByteWidth<=16*1024*1024){BYTE *data=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,desc.ByteWidth);if(data){result=&mirrors[mirrorCount];result->data=data;result->size=desc.ByteWidth;result->resource=resource;ID3D11Resource_AddRef(resource);mirrorBytes+=desc.ByteWidth;InterlockedIncrement(&mirrorCount);}}
    ReleaseSRWLockExclusive(&mirrorLock);return result;
}
static BufferMirror *SmallConstant(ID3D11Resource *resource) {
    /* Register lazily at the matched UI/HUD draw, never on unrelated game
       uploads. Otherwise transient scene constants exhaust the bounded pool. */
    return Mirror(resource,0);
}
static HRESULT STDMETHODCALLTYPE MapHook(ID3D11DeviceContext *ctx,ID3D11Resource *resource,UINT sub,D3D11_MAP mode,UINT flags,D3D11_MAPPED_SUBRESOURCE *out) {
    void **v=Original(ctx,contexts,contextCount);
    HRESULT hr=((HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT,D3D11_MAP,UINT,D3D11_MAPPED_SUBRESOURCE*))v[14])(ctx,resource,sub,mode,flags,out);
    if(SUCCEEDED(hr)&&sub==0&&mode!=D3D11_MAP_READ){BufferMirror *m=SmallConstant(resource);if(m)m->mapped=out->pData;}
    return hr;
}
static void STDMETHODCALLTYPE UnmapHook(ID3D11DeviceContext *ctx,ID3D11Resource *resource,UINT sub) {
    BufferMirror *m=Mirror(resource,0);if(m&&m->mapped&&sub==0){BytesCopy(m->data,m->mapped,m->size);m->mapped=0;m->valid=1;}
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT))v[15])(ctx,resource,sub);
}
static void STDMETHODCALLTYPE UpdateHook(ID3D11DeviceContext *ctx,ID3D11Resource *resource,UINT sub,const D3D11_BOX *box,const void *data,UINT row,UINT depth) {
    MenuTextureUpload(resource,sub,box,data,row);
    if(sub==0&&!box){BufferMirror *m=SmallConstant(resource);if(m){BytesCopy(m->data,data,m->size);m->valid=1;}}
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT,const D3D11_BOX*,const void*,UINT,UINT))v[48])(ctx,resource,sub,box,data,row,depth);
}
typedef struct {ID3D11DeviceContext *context;ID3D11VertexShader *vs4,*vs6;ID3D11Buffer *constant;} FixResources;
static void STDMETHODCALLTYPE Update1Hook(ID3D11DeviceContext1 *ctx,ID3D11Resource *resource,UINT sub,const D3D11_BOX *box,const void *data,UINT row,UINT depth,UINT flags) {
    MenuTextureUpload(resource,sub,box,data,row);
    if(sub==0&&!box){BufferMirror *m=SmallConstant(resource);if(m){BytesCopy(m->data,data,m->size);m->valid=1;}}
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext1*,ID3D11Resource*,UINT,const D3D11_BOX*,const void*,UINT,UINT,UINT))v[__builtin_offsetof(ID3D11DeviceContext1Vtbl,UpdateSubresource1)/sizeof(void*)])(ctx,resource,sub,box,data,row,depth,flags);
}
static FixResources fixResources[32];
static LONG fixCount;
static void InitialSnapshot(ID3D11DeviceContext *ctx,BufferMirror *mirror) {
    if(!mirror||mirror->valid)return;
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);
    D3D11_BUFFER_DESC desc={mirror->size,D3D11_USAGE_STAGING,0,D3D11_CPU_ACCESS_READ,0,0};ID3D11Buffer *copy=0;
    if(SUCCEEDED(ID3D11Device_CreateBuffer(device,&desc,0,&copy))){D3D11_BOX region={0,0,0,mirror->size,1,1};ID3D11DeviceContext_CopySubresourceRegion(ctx,(ID3D11Resource*)copy,0,0,0,0,mirror->resource,0,&region);D3D11_MAPPED_SUBRESOURCE map;if(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)copy,0,D3D11_MAP_READ,0,&map))){BytesCopy(mirror->data,map.pData,mirror->size);mirror->valid=1;ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)copy,0);}ID3D11Buffer_Release(copy);}
    ID3D11Device_Release(device);
}
typedef struct {ID3D11VertexShader *original;ID3D11Buffer *constant;D3D11_VIEWPORT vp[16];UINT vpCount;D3D11_RECT scissor[16];UINT scissorCount;int changedViewport;} FixState;
typedef struct {ID3D11DeviceContext *context;float left,right,y;ULONGLONG tick;} ProjectedLabel;
static ProjectedLabel projectedLabel;
static LONG projectedLabelHits;
/* Briefing names are projected in full-screen coordinates but composed through
   a centered 1920x1080 plane. Recognize their full-layer-clipped underline and
   the following matching glyph draw; remove the extra pillarbox offset once. */
static float ProjectedLabelShift(ID3D11DeviceContext *ctx,UINT count,uint64_t ph,float left,float right,float top,float bottom) {
    D3D11_RECT clip[16];UINT clips=16;ID3D11DeviceContext_RSGetScissorRects(ctx,&clips,clip);
    if(clips!=1||clip[0].left||clip[0].top||clip[0].right!=1920||clip[0].bottom!=1080)return 0;
    if(hudWidth*9<=hudHeight*16||hudHeight<=0)return 0;
    int line=0;
    if(ph==0x2d58759ed62f1f84ull&&count==6&&right-left>40&&right-left<200&&bottom-top>.5f&&bottom-top<1.5f&&top>=0&&bottom<=1080){
        ID3D11ShaderResourceView *srv=0;ID3D11DeviceContext_PSGetShaderResources(ctx,0,1,&srv);
        if(srv){ID3D11Resource *res=0;ID3D11ShaderResourceView_GetResource(srv,&res);D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(res,&dim);if(dim==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)res,&desc);line=desc.Width==1&&desc.Height==1;}ID3D11Resource_Release(res);ID3D11ShaderResourceView_Release(srv);}
    }
    if(line){projectedLabel.context=ctx;projectedLabel.left=left;projectedLabel.right=right;projectedLabel.y=top;projectedLabel.tick=GetTickCount64();}
    else if(!(ph==0x0b39f446865515f9ull&&projectedLabel.context==ctx&&GetTickCount64()-projectedLabel.tick<100&&left>projectedLabel.left-3&&left<projectedLabel.left+3&&right>projectedLabel.right-3&&right<projectedLabel.right+3&&top>projectedLabel.y+3&&top<projectedLabel.y+15&&bottom<projectedLabel.y+30))return 0;
    else projectedLabel.context=0;
    if(InterlockedIncrement(&projectedLabelHits)==1)UILog("Briefing projected name/underline offset correction active.\r\n");
    float sourceOffset=((float)hudWidth*1080/hudHeight-1920)*.5f;
    return -sourceOffset*2/1920;
}
static int ApplyUI(ID3D11DeviceContext *ctx,UINT count,UINT first,INT base,FixState *state) {
    if(!legacyUI&&!textCorrection&&!forceWiden)return 0;
    if(count<6 || count%6 || base<0)return 0;
    ID3D11VertexShader *vs=0;ID3D11DeviceContext_VSGetShader(ctx,&vs,0,0);
    uint64_t hash=GetShaderHash(vs);int six=(hash==0x8f3937c557a87704ull);
    if(hash!=0x47897216336c5479ull&&!six){if(vs)ID3D11VertexShader_Release(vs);return 0;}
    ID3D11Buffer *cb=0,*vb=0;UINT stride=0,offset=0;
    ID3D11DeviceContext_VSGetConstantBuffers(ctx,0,1,&cb);ID3D11DeviceContext_IAGetVertexBuffers(ctx,0,1,&vb,&stride,&offset);
    BufferMirror *cm=Mirror((ID3D11Resource*)cb,1),*vm=Mirror((ID3D11Resource*)vb,1);
    InitialSnapshot(ctx,cm);InitialSnapshot(ctx,vm);
    if(cb)ID3D11Buffer_Release(cb);if(vb)ID3D11Buffer_Release(vb);
    if(!cm||!vm||stride!=40||cm->size<96){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    ID3D11Buffer *ib=0;DXGI_FORMAT format;UINT indexOffset=0;
    ID3D11DeviceContext_IAGetIndexBuffer(ctx,&ib,&format,&indexOffset);
    BufferMirror *im=Mirror((ID3D11Resource*)ib,1);InitialSnapshot(ctx,im);if(ib)ID3D11Buffer_Release(ib);
    UINT indexSize=format==DXGI_FORMAT_R16_UINT?2:format==DXGI_FORMAT_R32_UINT?4:0;
    if(!cm->valid||!vm->valid||!im||!im->valid||!indexSize||count>12288 || (uint64_t)indexOffset+((uint64_t)first+count)*indexSize>im->size){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    float *matrix=(float*)cm->data;UINT x=six?8:0,y=six?12:4,t=six?20:12;
    if(matrix[x]<=0 || matrix[y+1]>=0){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    float minx=100000,maxx=-100000,miny=100000,maxy=-100000;
    for(UINT i=0;i<count;++i){BYTE *ip=im->data+indexOffset+(first+i)*indexSize;UINT index=indexSize==2?*(USHORT*)ip:*(UINT*)ip;uint64_t position=(uint64_t)offset+((uint64_t)(UINT)base+index)*stride;if(position+stride>vm->size){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}float *vertex=(float*)(vm->data+position);float px=vertex[6],py=vertex[7];float cx=px*matrix[x]+py*matrix[y]+matrix[t],cy=px*matrix[x+1]+py*matrix[y+1]+matrix[t+1];if(cx<minx)minx=cx;if(cx>maxx)maxx=cx;if(cy<miny)miny=cy;if(cy>maxy)maxy=cy;}
    state->vpCount=16;ID3D11DeviceContext_RSGetViewports(ctx,&state->vpCount,state->vp);if(state->vpCount!=1){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    D3D11_VIEWPORT vp=state->vp[0];
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    ID3D11Resource *resource=0;ID3D11RenderTargetView_GetResource(rt,&resource);ID3D11RenderTargetView_Release(rt);D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);
    if(dim!=D3D11_RESOURCE_DIMENSION_TEXTURE2D){ID3D11Resource_Release(resource);ID3D11VertexShader_Release(vs);return 0;}
    D3D11_TEXTURE2D_DESC target;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&target);ID3D11Resource_Release(resource);
    float width=(float)target.Width,height=(float)target.Height;
    float left=vp.TopLeftX+(minx+1)*vp.Width*.5f,right=vp.TopLeftX+(maxx+1)*vp.Width*.5f;
    float top=vp.TopLeftY+(1-maxy)*vp.Height*.5f,bottom=vp.TopLeftY+(1-miny)*vp.Height*.5f;
    float center=(left+right)*.5f,boundsWidth=right-left,scale=1,shift=0;int background=0;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t ph=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(textCorrection&&!six&&target.Width==1920&&target.Height==1080&&vp.TopLeftX==0&&vp.TopLeftY==0&&vp.Width==1920&&vp.Height==1080)shift=ProjectedLabelShift(ctx,count,ph,left,right,top,bottom);
    if(!shift&&width/height<1.9f){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    int menuComposite=six&&ph==0x5eae2582c19da3f5ull&&left>-2&&left<2&&top>-2&&top<2&&boundsWidth>height*1.77f&&boundsWidth<height*1.79f;
    if(shift){ /* Matched projected label: retain the scale and negative shift. */ }
    else if((legacyUI||forceWiden) && count==6 && bottom-top>height*.94f && boundsWidth>height*1.70f && boundsWidth<width*.94f && menuComposite){
        float widen=width/boundsWidth;scale=vp.Width*widen/width;shift=(vp.TopLeftX+vp.Width*.5f-center)*widen*2/width;background=1;
    } else {
        if(!textCorrection){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
        float expected=height*(16.0f/9.0f)*.5f;
        /* Subtitle lines are centered on the 16:9 area, but the << >> markers and the skip prompt are
           separate small draws (same text-shader family) that sit anywhere inside that area. */
        int textShader=ph==0x0b39f446865515f9ull||ph==0xc99a185ea1aeb967ull||ph==0xf03b6e4961e6e971ull||ph==0x2d58759ed62f1f84ull;
        int atCenter=center>expected-30&&center<expected+30,inArea=left>-2&&right<expected*2+2;
        /* Cutscene subtitles are already centered on the full screen; shifting them again pushes them right. */
        if(center>width*.5f-30&&center<width*.5f+30)atCenter=inArea=0;
        if(textShader && vp.TopLeftX<1 && vp.Width>width*.99f && matrix[x]<2.1f/width && (atCenter||inArea) && (bottom<height*.20f || top>height*.80f))shift=(width*.5f-expected)*2/vp.Width;
        else {UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    }
    FixResources *fix=0;for(LONG i=0;i<fixCount;++i)if(fixResources[i].context==ctx)fix=&fixResources[i];
    if(!fix&&fixCount<32){fix=&fixResources[fixCount++];fix->context=ctx;}
    if(!fix){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);
    ID3D11VertexShader **replacement=six?&fix->vs6:&fix->vs4;
    if(!*replacement)ID3D11Device_CreateVertexShader(device,six?UI6:UI4,six?sizeof(UI6):sizeof(UI4),0,replacement);
    if(!fix->constant){D3D11_BUFFER_DESC desc={16,D3D11_USAGE_DYNAMIC,D3D11_BIND_CONSTANT_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};ID3D11Device_CreateBuffer(device,&desc,0,&fix->constant);}
    ID3D11Device_Release(device);
    if(!*replacement||!fix->constant){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    D3D11_MAPPED_SUBRESOURCE mapped;if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)fix->constant,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))){UIBAIL();ID3D11VertexShader_Release(vs);return 0;}
    float *params=mapped.pData;params[0]=scale;params[1]=shift;params[2]=params[3]=0;ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)fix->constant,0);
    state->original=vs;state->constant=0;state->changedViewport=background;ID3D11DeviceContext_VSGetConstantBuffers(ctx,13,1,&state->constant);
    state->scissorCount=16;ID3D11DeviceContext_RSGetScissorRects(ctx,&state->scissorCount,state->scissor);
    if(background){D3D11_RECT full={0,0,(LONG)width,(LONG)height};ID3D11DeviceContext_RSSetScissorRects(ctx,1,&full);vp.TopLeftX=0;vp.TopLeftY=0;vp.Width=width;vp.Height=height;ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);}
    ID3D11DeviceContext_VSSetConstantBuffers(ctx,13,1,&fix->constant);ID3D11DeviceContext_VSSetShader(ctx,*replacement,0,0);
    if(background&&bailBudget>0)BailLog(ctx,-1);
    if(background){if(InterlockedIncrement(&backgroundHits)==1)UILog("Native menu background expanded to full width.\r\n");}
    else if(InterlockedIncrement(&textHits)==1)UILog("Native cutscene dialogue/skip text recentered.\r\n");
    return 1;
}
static void RestoreUI(ID3D11DeviceContext *ctx,FixState *state) {
    ID3D11DeviceContext_VSSetShader(ctx,state->original,0,0);ID3D11VertexShader_Release(state->original);
    ID3D11DeviceContext_VSSetConstantBuffers(ctx,13,1,&state->constant);if(state->constant)ID3D11Buffer_Release(state->constant);
    ID3D11DeviceContext_RSSetScissorRects(ctx,state->scissorCount,state->scissor);
    if(state->changedViewport)ID3D11DeviceContext_RSSetViewports(ctx,state->vpCount,state->vp);
}
