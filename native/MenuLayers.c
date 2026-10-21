static void TraceMenuArt(const char*,ID3D11Resource*,ID3D11Resource*,uint64_t,UINT,int,UINT,UINT);
/* Identify the captured background artwork at upload, then propagate that
   identity only through single-quad Slate copies. Never classify UI by PS alone. */
static const GUID menuArtTag={0x4ebdafb1,0x39d4,0x4a9f,{0x8e,0x13,0x72,0xf1,0x8c,0x2d,0x09,0x51}};
static UINT MenuArt(ID3D11Resource *resource) {
    UINT value=0,size=sizeof(value);if(resource)ID3D11Resource_GetPrivateData(resource,&menuArtTag,&size,&value);return value;
}
/* Composition lineage is distinct from pure-art identity: decorations may
   invalidate cover scaling while the composition still needs menu mattes. */
static const GUID menuBackgroundTag={0x375d21c3,0xa66e,0x40f1,{0xa1,0xb2,0x48,0x95,0x83,0xd3,0x29,0x72}};
static UINT MenuBackground(ID3D11Resource *r){UINT value=0,size=sizeof(value);if(r)ID3D11Resource_GetPrivateData(r,&menuBackgroundTag,&size,&value);return value;}
static void SetMenuBackground(ID3D11Resource *r,UINT value){if(r)ID3D11Resource_SetPrivateData(r,&menuBackgroundTag,sizeof(value),&value);}
/* Some engine upload/copy paths bypass initial data and UpdateSubresource.
   Probe only exact-size BC1 sources at a matched full-layer copy, once per
   resource, and cap all fallback readbacks for the process. */
static const GUID menuProbeTag={0x7444797e,0x53df,0x4164,{0x80,0xda,0x8a,0x39,0xb3,0x47,0x67,0x1f}};
static LONG menuProbeCount;
static UINT DetectMenuArt(ID3D11DeviceContext *ctx,ID3D11Resource *resource) {
    if(MenuArt(resource))return 1;
    UINT checked=0,size=sizeof(checked);ID3D11Resource_GetPrivateData(resource,&menuProbeTag,&size,&checked);if(checked||ID3D11DeviceContext_GetType(ctx)!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return 0;
    D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);if(dim!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return 0;
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);if(desc.Width!=1920||desc.Height!=1080||desc.Format<70||desc.Format>72||desc.SampleDesc.Count!=1)return 0;
    checked=1;ID3D11Resource_SetPrivateData(resource,&menuProbeTag,sizeof(checked),&checked);if(InterlockedIncrement(&menuProbeCount)>16)return 0;
    desc.MipLevels=desc.ArraySize=1;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);ID3D11Texture2D *copy=0;UINT known=0;
    if(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&copy))){
        ID3D11DeviceContext_CopySubresourceRegion(ctx,(ID3D11Resource*)copy,0,0,0,0,resource,0,0);D3D11_MAPPED_SUBRESOURCE map;
        if(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)copy,0,D3D11_MAP_READ,0,&map))){MenuTextureUpload(resource,0,0,map.pData,map.RowPitch);known=MenuArt(resource);ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)copy,0);}
        ID3D11Texture2D_Release(copy);
    }
    ID3D11Device_Release(device);return known;
}
static void MenuTextureUpload(ID3D11Resource *resource,UINT sub,const D3D11_BOX *box,const void *data,UINT pitch) {
    if(sub||!data||pitch<3840)return;
    D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);if(dim!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return;
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);
    if(desc.Width!=1920||desc.Height!=1080||desc.Format<70||desc.Format>72)return;
    UINT known=0;
    if(!box||(!box->left&&!box->top&&!box->front&&box->right==1920&&box->bottom==1080&&box->back==1)){
        uint64_t hash=0xcbf29ce484222325ull;const BYTE *bytes=data;
        for(UINT y=0;y<270;++y)for(UINT x=0;x<3840;++x){hash*=0x100000001b3ull;hash^=bytes[(SIZE_T)y*pitch+x];}
        known=hash==0xde80c0a319352b37ull;
    }
    if(known&&!MenuArt(resource))UILog("Known menu background artwork identified from upload.\r\n");
    TraceMenuArt("upload",0,resource,0,0,box==0,MenuArt(resource),known);
    ID3D11Resource_SetPrivateData(resource,&menuArtTag,sizeof(known),&known);SetMenuBackground(resource,known);
}
static HRESULT STDMETHODCALLTYPE CreateMenuTexture(ID3D11Device *device,const D3D11_TEXTURE2D_DESC *desc,const D3D11_SUBRESOURCE_DATA *data,ID3D11Texture2D **out) {
    void **v=Original(device,devices,deviceCount);
    HRESULT hr=((HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,const D3D11_TEXTURE2D_DESC*,const D3D11_SUBRESOURCE_DATA*,ID3D11Texture2D**))v[5])(device,desc,data,out);
    if(SUCCEEDED(hr)&&out&&*out&&data)MenuTextureUpload((ID3D11Resource*)*out,0,0,data->pSysMem,data->SysMemPitch);
    return hr;
}
static int FullMenuCopy(ID3D11DeviceContext *ctx,UINT first,INT base) {
    if(base<0)return 0;
    ID3D11VertexShader *vs=0;ID3D11DeviceContext_VSGetShader(ctx,&vs,0,0);uint64_t hash=GetShaderHash(vs);if(vs)ID3D11VertexShader_Release(vs);if(hash!=0x47897216336c5479ull)return 0;
    ID3D11Buffer *cb=0,*vb=0,*ib=0;UINT stride=0,offset=0,io=0;DXGI_FORMAT format;
    ID3D11DeviceContext_VSGetConstantBuffers(ctx,0,1,&cb);ID3D11DeviceContext_IAGetVertexBuffers(ctx,0,1,&vb,&stride,&offset);ID3D11DeviceContext_IAGetIndexBuffer(ctx,&ib,&format,&io);
    BufferMirror *cm=Mirror((ID3D11Resource*)cb,1),*vm=Mirror((ID3D11Resource*)vb,1),*im=Mirror((ID3D11Resource*)ib,1);InitialSnapshot(ctx,cm);InitialSnapshot(ctx,vm);InitialSnapshot(ctx,im);if(cb)ID3D11Buffer_Release(cb);if(vb)ID3D11Buffer_Release(vb);if(ib)ID3D11Buffer_Release(ib);
    UINT indexSize=format==DXGI_FORMAT_R16_UINT?2:format==DXGI_FORMAT_R32_UINT?4:0;
    if(!cm||!vm||!im||!cm->valid||!vm->valid||!im->valid||cm->size<64||stride!=40||!indexSize||(uint64_t)io+((uint64_t)first+6)*indexSize>im->size)return 0;
    float left=100,right=-100,bottom=100,top=-100,*m=(float*)cm->data;
    for(UINT i=0;i<6;++i){BYTE *ip=im->data+io+(first+i)*indexSize;UINT index=indexSize==2?*(USHORT*)ip:*(UINT*)ip;uint64_t at=(uint64_t)offset+((uint64_t)(UINT)base+index)*stride;if(at+stride>vm->size)return 0;float *p=(float*)(vm->data+at),x=p[6]*m[0]+p[7]*m[4]+m[12],y=p[6]*m[1]+p[7]*m[5]+m[13];if(x<left)left=x;if(x>right)right=x;if(y<bottom)bottom=y;if(y>top)top=y;}
    D3D11_VIEWPORT vp;UINT n=1;ID3D11DeviceContext_RSGetViewports(ctx,&n,&vp);
    D3D11_RECT clip;UINT clips=1;ID3D11DeviceContext_RSGetScissorRects(ctx,&clips,&clip);
    return clips==1&&clip.left<=0&&clip.top<=0&&clip.right>=1920&&clip.bottom>=1080&&n==1&&vp.TopLeftX==0&&vp.TopLeftY==0&&vp.Width==1920&&vp.Height==1080&&left>=-1.02f&&left<=-.98f&&right>=.98f&&right<=1.02f&&bottom>=-1.02f&&bottom<=-.98f&&top>=.98f&&top<=1.02f;
}
static void NoteMenuLayer(ID3D11DeviceContext *ctx,UINT count,UINT first,INT base) {
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt)return;
    ID3D11Resource *target=0;ID3D11RenderTargetView_GetResource(rt,&target);ID3D11RenderTargetView_Release(rt);
    D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(target,&dim);if(dim!=D3D11_RESOURCE_DIMENSION_TEXTURE2D){ID3D11Resource_Release(target);return;}
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)target,&desc);if(desc.Width!=1920||desc.Height!=1080){ID3D11Resource_Release(target);return;}
    UINT known=0;ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    int full=count==6&&hash==0x2d58759ed62f1f84ull&&FullMenuCopy(ctx,first,base);
    ID3D11ShaderResourceView *srv=0;ID3D11Resource *source=0;ID3D11DeviceContext_PSGetShaderResources(ctx,0,1,&srv);
    if(srv){ID3D11ShaderResourceView_GetResource(srv,&source);ID3D11ShaderResourceView_Release(srv);}
    if(full&&source)known=DetectMenuArt(ctx,source);
    SetMenuBackground(target,full?known||(source&&MenuBackground(source)):MenuBackground(target));
    TraceMenuArt("draw",source,target,hash,count,full,MenuArt(target),known);
    if(known||MenuArt(target))ID3D11Resource_SetPrivateData(target,&menuArtTag,sizeof(known),&known);
    if(source)ID3D11Resource_Release(source);
    ID3D11Resource_Release(target);
}
static void STDMETHODCALLTYPE ClearMenuTarget(ID3D11DeviceContext *ctx,ID3D11RenderTargetView *rt,const FLOAT color[4]) {
    if(rt){ID3D11Resource *target=0;ID3D11RenderTargetView_GetResource(rt,&target);SetMenuBackground(target,0);ResetMenuMatte(target);TraceMenuArt("clear",0,target,0,0,0,MenuArt(target),0);if(MenuArt(target)){UINT no=0;ID3D11Resource_SetPrivateData(target,&menuArtTag,sizeof(no),&no);}ID3D11Resource_Release(target);}
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11RenderTargetView*,const FLOAT*))v[50])(ctx,rt,color);
}
static void CopyMenuTag(ID3D11Resource *dst,UINT dstSub,UINT x,UINT y,UINT z,ID3D11Resource *src,UINT srcSub,const D3D11_BOX *box) {
    UINT pure=MenuArt(src),known=pure||MenuBackground(src),old=MenuArt(dst);
    UINT keep=0;D3D11_RESOURCE_DIMENSION sd,dd;ID3D11Resource_GetType(src,&sd);ID3D11Resource_GetType(dst,&dd);
    if(known&&!dstSub&&!srcSub&&!x&&!y&&!z&&sd==D3D11_RESOURCE_DIMENSION_TEXTURE2D&&dd==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC a,b;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)src,&a);ID3D11Texture2D_GetDesc((ID3D11Texture2D*)dst,&b);keep=a.Width==1920&&a.Height==1080&&b.Width==a.Width&&b.Height==a.Height&&(!box||(!box->left&&!box->top&&!box->front&&box->right==a.Width&&box->bottom==a.Height&&box->back==1));}
    SetMenuBackground(dst,keep);keep=keep&&pure;
    TraceMenuArt("copy",src,dst,0,0,keep,old,keep);
    if(known||old)ID3D11Resource_SetPrivateData(dst,&menuArtTag,sizeof(keep),&keep);
}
static void STDMETHODCALLTYPE CopyMenuResource(ID3D11DeviceContext *ctx,ID3D11Resource *dst,ID3D11Resource *src) {
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,ID3D11Resource*))v[47])(ctx,dst,src);CopyMenuTag(dst,0,0,0,0,src,0,0);CopyMenuMatte(ctx,dst,0,0,0,0,src,0,0);
}
static void STDMETHODCALLTYPE CopyMenuRegion(ID3D11DeviceContext *ctx,ID3D11Resource *dst,UINT dstSub,UINT x,UINT y,UINT z,ID3D11Resource *src,UINT srcSub,const D3D11_BOX *box) {
    void **v=Original(ctx,contexts,contextCount);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT,UINT,UINT,UINT,ID3D11Resource*,UINT,const D3D11_BOX*))v[46])(ctx,dst,dstSub,x,y,z,src,srcSub,box);CopyMenuTag(dst,dstSub,x,y,z,src,srcSub,box);CopyMenuMatte(ctx,dst,dstSub,x,y,z,src,srcSub,box);
}
