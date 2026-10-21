/* ClearView writes opaque side masks without changing pipeline bindings.
   Marks follow full-screen composition; scene draws and clears reset them. */
static const GUID menuMatteTag={0x43405c27,0xeaf1,0x407d,{0x9c,0xa8,0x79,0xb1,0xce,0x82,0x09,0x13}};
/* Each scene gets a new context epoch. This invalidates masks on all of its
   intermediate textures, including temporal inputs that were not cleared. */
static const GUID menuMatteEpochTag={0x744b7642,0x436b,0x4b1b,{0x97,0xcf,0x0e,0xfa,0x6a,0x18,0x13,0xe8}};
static LONG64 menuMatteSerial;
/* Context hooks already have process-lifetime storage. Keep the epoch and an
   empty-scene flag there, avoiding COM state queries on mask-free draws. The
   private-data fallback retains behavior for contexts outside the hook table. */
static HookObject *MatteHook(ID3D11DeviceContext *ctx){
    void *table=*(void**)ctx;for(LONG i=0;i<contextCount;++i)if(contexts[i].object==table)return &contexts[i];return 0;
}
static ULONGLONG MenuMatteEpoch(ID3D11DeviceContext *ctx,int advance){
    HookObject *hook=MatteHook(ctx);ULONGLONG epoch=hook?hook->menuEpoch:0;
    if(!hook){UINT size=sizeof(epoch);ID3D11DeviceContext_GetPrivateData(ctx,&menuMatteEpochTag,&size,&epoch);}
    if(advance||!epoch){epoch=(ULONGLONG)InterlockedIncrement64(&menuMatteSerial);ID3D11DeviceContext_SetPrivateData(ctx,&menuMatteEpochTag,sizeof(epoch),&epoch);if(hook){hook->menuEpoch=epoch;hook->menuMasks=0;}}return epoch;
}
static UINT ResourceMenuMatte(ID3D11DeviceContext *ctx,ID3D11Resource *r){
    ULONGLONG epoch=0;UINT size=sizeof(epoch);if(r)ID3D11Resource_GetPrivateData(r,&menuMatteTag,&size,&epoch);return epoch&&epoch==MenuMatteEpoch(ctx,0);
}
static void ResetMenuMatte(ID3D11Resource *r){ULONGLONG value=0;if(r)ID3D11Resource_SetPrivateData(r,&menuMatteTag,sizeof(value),&value);}
static UINT CurrentMenuMatte(ID3D11DeviceContext *ctx){
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt)return 0;
    ID3D11Resource *r=0;ID3D11RenderTargetView_GetResource(rt,&r);ID3D11RenderTargetView_Release(rt);
    UINT marked=ResourceMenuMatte(ctx,r);ID3D11Resource_Release(r);return marked;
}
static void ResetCurrentMenuMatte(ID3D11DeviceContext *ctx){
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt)return;
    ID3D11Resource *r=0;ID3D11RenderTargetView_GetResource(rt,&r);ResetMenuMatte(r);ID3D11Resource_Release(r);ID3D11RenderTargetView_Release(rt);
}
static int ApplyMenuMatte(ID3D11DeviceContext *ctx){
    UINT width=0,height=0;if(!PrimaryTarget(ctx,&width,&height)||width*9<=height*16)return 0;
    ID3D11DeviceContext1 *advanced=0;if(FAILED(ID3D11DeviceContext_QueryInterface(ctx,&IID_ID3D11DeviceContext1,(void**)&advanced)))return 0;
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt){ID3D11DeviceContext1_Release(advanced);return 0;}
    LONG edge=(LONG)((width-height*(16.0/9.0))*.5+.5);D3D11_RECT rects[2]={{0,0,edge,(LONG)height},{(LONG)width-edge,0,(LONG)width,(LONG)height}};const float black[4]={0,0,0,1};
    ID3D11DeviceContext1_ClearView(advanced,(ID3D11View*)rt,black,rects,2);
    ID3D11Resource *r=0;ID3D11RenderTargetView_GetResource(rt,&r);ULONGLONG epoch=MenuMatteEpoch(ctx,0);HookObject *hook=MatteHook(ctx);if(hook)hook->menuMasks=1;ID3D11Resource_SetPrivateData(r,&menuMatteTag,sizeof(epoch),&epoch);ID3D11Resource_Release(r);
    ID3D11RenderTargetView_Release(rt);ID3D11DeviceContext1_Release(advanced);return 1;
}

/* Full-screen postprocessing can move the menu between primary-sized targets.
   Carry its mask through sampled inputs, then clear the sides after the draw.
   Only small full-viewport composites may acquire a mark from an input. Once
   marked, subsequent overlays on that target also keep its sides opaque. */
static int InheritMenuMatte(ID3D11DeviceContext *ctx,UINT count){
    HookObject *hook=MatteHook(ctx);if(hook&&!hook->menuMasks)return 0;
    if(CurrentMenuMatte(ctx))return 1;
    if(count!=3&&count!=6)return 0;
    UINT width=0,height=0;if(!PrimaryTarget(ctx,&width,&height)||width*9<=height*16)return 0;
    D3D11_VIEWPORT vp;UINT n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&vp);
    if(n!=1||vp.TopLeftX!=0||vp.TopLeftY!=0||vp.Width!=width||vp.Height!=height)return 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;ID3D11DeviceContext_IAGetPrimitiveTopology(ctx,&topology);if(topology!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST&&topology!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)return 0;
    ID3D11RasterizerState *raster=0;ID3D11DeviceContext_RSGetState(ctx,&raster);int clipped=0;
    if(raster){D3D11_RASTERIZER_DESC desc;ID3D11RasterizerState_GetDesc(raster,&desc);clipped=desc.ScissorEnable;ID3D11RasterizerState_Release(raster);}
    if(clipped){D3D11_RECT rect;UINT rectCount=1;ID3D11DeviceContext_RSGetScissorRects(ctx,&rectCount,&rect);if(rectCount!=1||rect.left>0||rect.top>0||rect.right<(LONG)width||rect.bottom<(LONG)height)return 0;}
    ID3D11ShaderResourceView *views[16]={0};ID3D11DeviceContext_PSGetShaderResources(ctx,0,16,views);int marked=0;
    for(UINT i=0;i<16;++i)if(views[i]){ID3D11Resource *r=0;ID3D11ShaderResourceView_GetResource(views[i],&r);
        if(ResourceMenuMatte(ctx,r))marked=1;ID3D11Resource_Release(r);ID3D11ShaderResourceView_Release(views[i]);}
    return marked;
}
static void CopyMenuMatte(ID3D11DeviceContext *ctx,ID3D11Resource *dst,UINT dstSub,UINT x,UINT y,UINT z,ID3D11Resource *src,UINT srcSub,const D3D11_BOX *box){
    /* Partial writes cannot establish complete menu composition. Never retain
       the old destination mark after an unrelated replacement/copy. */
    int marked=0;
    if(!dstSub&&!srcSub&&!x&&!y&&!z&&ResourceMenuMatte(ctx,src)){
        D3D11_RESOURCE_DIMENSION a,b;ID3D11Resource_GetType(src,&a);ID3D11Resource_GetType(dst,&b);
        if(a==D3D11_RESOURCE_DIMENSION_TEXTURE2D&&b==a){D3D11_TEXTURE2D_DESC sd,dd;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)src,&sd);ID3D11Texture2D_GetDesc((ID3D11Texture2D*)dst,&dd);
            marked=sd.Width==dd.Width&&sd.Height==dd.Height&&(!box||(!box->left&&!box->top&&!box->front&&box->right==sd.Width&&box->bottom==sd.Height&&box->back==1));}
    }
    ULONGLONG epoch=marked?MenuMatteEpoch(ctx,0):0;ID3D11Resource_SetPrivateData(dst,&menuMatteTag,sizeof(epoch),&epoch);
}
