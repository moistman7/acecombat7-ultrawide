/* Correct a constrained primary scene viewport before rasterization. The
   Slate render textures and partial widget scissors keep their own sizes. */
static LONG viewportHits;
/* Preserve the game's shared scene/label projection for pillarboxed menus.
   Expanding just the scene detaches projected briefing labels from map markers. */
static int expandConstrainedScenes;
static int PrimaryTarget(ID3D11DeviceContext *ctx,UINT *width,UINT *height) {
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt)return 0;
    ID3D11Resource *resource=0;ID3D11RenderTargetView_GetResource(rt,&resource);ID3D11RenderTargetView_Release(rt);
    D3D11_RESOURCE_DIMENSION dimension;ID3D11Resource_GetType(resource,&dimension);int primary=0;
    if(dimension==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);*width=desc.Width;*height=desc.Height;primary=desc.Width==(UINT)hudWidth&&desc.Height==(UINT)hudHeight;}
    ID3D11Resource_Release(resource);return primary;
}
static void STDMETHODCALLTYPE ViewportHook(ID3D11DeviceContext *ctx,UINT count,const D3D11_VIEWPORT *viewports) {
    void **v=Original(ctx,contexts,contextCount);D3D11_VIEWPORT widened;
    if(expandConstrainedScenes&&count==1&&viewports&&viewports[0].TopLeftX>1&&viewports[0].TopLeftY<1&&viewports[0].Height>0){
        UINT width=0,height=0;float ratio=viewports[0].Width/viewports[0].Height;
        if(ratio>1.77f&&ratio<1.79f&&PrimaryTarget(ctx,&width,&height)&&width>viewports[0].Width&&viewports[0].Height>(float)height-1){
            float center=viewports[0].TopLeftX+viewports[0].Width*.5f;
            if(center>(float)width*.5f-1&&center<(float)width*.5f+1){widened=viewports[0];widened.TopLeftX=0;widened.Width=(float)width;viewports=&widened;if(InterlockedIncrement(&viewportHits)==1)UILog("Primary centered 16:9 scene viewport expanded before rasterization.\r\n");}
        }
    }
    ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,const D3D11_VIEWPORT*))v[44])(ctx,count,viewports);
}
static void STDMETHODCALLTYPE ScissorHook(ID3D11DeviceContext *ctx,UINT count,const D3D11_RECT *rects) {
    void **v=Original(ctx,contexts,contextCount);D3D11_RECT widened;
    if(expandConstrainedScenes&&count==1&&rects&&rects[0].left>0&&rects[0].top==0){UINT width=0,height=0;float ratio=(float)(rects[0].right-rects[0].left)/(float)rects[0].bottom;
        if(ratio>1.77f&&ratio<1.79f&&PrimaryTarget(ctx,&width,&height)&&rects[0].bottom==(LONG)height&&rects[0].left+rects[0].right==(LONG)width){widened=rects[0];widened.left=0;widened.right=(LONG)width;rects=&widened;}}
    ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,const D3D11_RECT*))v[45])(ctx,count,rects);
}
