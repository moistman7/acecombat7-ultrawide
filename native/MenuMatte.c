/* ClearView writes opaque side masks without changing pipeline bindings.
   The mark follows only the current target; scene draws and clears reset it. */
static const GUID menuMatteTag={0x43405c27,0xeaf1,0x407d,{0x9c,0xa8,0x79,0xb1,0xce,0x82,0x09,0x13}};
static void ResetMenuMatte(ID3D11Resource *r){UINT value=0;if(r)ID3D11Resource_SetPrivateData(r,&menuMatteTag,sizeof(value),&value);}
static UINT CurrentMenuMatte(ID3D11DeviceContext *ctx){
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt)return 0;
    ID3D11Resource *r=0;ID3D11RenderTargetView_GetResource(rt,&r);ID3D11RenderTargetView_Release(rt);
    UINT value=0,size=sizeof(value);ID3D11Resource_GetPrivateData(r,&menuMatteTag,&size,&value);ID3D11Resource_Release(r);return value;
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
    ID3D11Resource *r=0;ID3D11RenderTargetView_GetResource(rt,&r);UINT yes=1;ID3D11Resource_SetPrivateData(r,&menuMatteTag,sizeof(yes),&yes);ID3D11Resource_Release(r);
    ID3D11RenderTargetView_Release(rt);ID3D11DeviceContext1_Release(advanced);return 1;
}
