static LONG sceneCapture;
static ID3D11DeviceContext *sceneCaptureContext;
static LONG hudCaptureMask;
static LONG64 hudCaptureSerial;
static ID3D11DeviceContext *hudCaptureContext;
static WCHAR hudCapturePrefix[80];
static void InspectHUDComposite(ID3D11DeviceContext *ctx) {
    if(!diagnostics||!hudCaptureMask||hudCaptureContext)return;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    LONG bit=hash==0xeb4ce6dce4ada70aull?1:IsHUDFinalBlend(hash)?2:0;
    if(!bit||!(InterlockedAnd(&hudCaptureMask,~bit)&bit))return;
    WCHAR name[100],hex[17];lstrcpyW(hudCapturePrefix,L"hud-");HexName(hex,(uint64_t)hudCaptureSerial);lstrcatW(hudCapturePrefix,hex);lstrcatW(hudCapturePrefix,L"-");HexName(hex,hash);lstrcatW(hudCapturePrefix,hex);
    UINT info[3]={(UINT)hudMode,(UINT)hudWidth,(UINT)hudHeight};lstrcpyW(name,hudCapturePrefix);lstrcatW(name,L"-layout.bin");SaveData(name,info,sizeof(info));
    ID3D11Buffer *cb[2]={0};ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,2,cb);
    for(UINT i=0;i<2;++i)if(cb[i]){lstrcpyW(name,hudCapturePrefix);lstrcatW(name,i?L"-cb1.bin":L"-cb0.bin");DumpBuffer(ctx,cb[i],name);ID3D11Buffer_Release(cb[i]);}
    ID3D11ShaderResourceView *srvs[5]={0};ID3D11DeviceContext_PSGetShaderResources(ctx,0,5,srvs);
    for(UINT i=0;i<5;++i)if(srvs[i]){ID3D11Resource *res=0;ID3D11ShaderResourceView_GetResource(srvs[i],&res);lstrcpyW(name,hudCapturePrefix);WCHAR suffix[4]={L'-',L't',(WCHAR)(L'0'+i),0};lstrcatW(name,suffix);CaptureTexture(ctx,res,name);ID3D11Resource_Release(res);ID3D11ShaderResourceView_Release(srvs[i]);}
    lstrcpyW(name,hudCapturePrefix);lstrcatW(name,L"-before");CaptureTarget(ctx,name);hudCaptureContext=ctx;
}
static uint64_t primaryTraceHashes[32];
static LONG primaryTraceCount;
static int primaryTraceEnabled;
static LONG primaryTraceDraws;
static void TracePrimaryShader(ID3D11DeviceContext *ctx) {
    if(!primaryTraceEnabled||primaryTraceCount>=32||primaryTraceDraws++>=512)return;UINT width=0,height=0;if(!PrimaryTarget(ctx,&width,&height))return;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);if(!hash)return;
    for(LONG i=0;i<primaryTraceCount;++i)if(primaryTraceHashes[i]==hash)return;primaryTraceHashes[primaryTraceCount++]=hash;
    WCHAR prefix[80],name[100],hex[17];HexName(hex,hash);lstrcpyW(prefix,L"primary-");lstrcatW(prefix,hex);
    UINT sizes[6]={width,height,0,0,0,0};ID3D11ShaderResourceView *srvs[2]={0};ID3D11DeviceContext_PSGetShaderResources(ctx,0,2,srvs);
    for(UINT i=0;i<2;++i)if(srvs[i]){ID3D11Resource *resource=0;ID3D11ShaderResourceView_GetResource(srvs[i],&resource);D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);if(dim==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);sizes[2+i*2]=desc.Width;sizes[3+i*2]=desc.Height;}ID3D11Resource_Release(resource);ID3D11ShaderResourceView_Release(srvs[i]);}
    lstrcpyW(name,prefix);lstrcatW(name,L"-sizes.bin");SaveData(name,sizes,sizeof(sizes));D3D11_VIEWPORT vp[16];UINT count=16;ID3D11DeviceContext_RSGetViewports(ctx,&count,vp);lstrcpyW(name,prefix);lstrcatW(name,L"-viewports.bin");SaveData(name,vp,count*sizeof(D3D11_VIEWPORT));
    ID3D11Buffer *buffers[2]={0};ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,2,buffers);for(UINT i=0;i<2;++i)if(buffers[i]){lstrcpyW(name,prefix);lstrcatW(name,i?L"-cb1.bin":L"-cb0.bin");DumpBuffer(ctx,buffers[i],name);ID3D11Buffer_Release(buffers[i]);}
    if(primaryTraceCount==1)UILog("Primary render-target shader trace started (maximum 32 unique shaders).\r\n");
}
static void InspectSceneComposite(ID3D11DeviceContext *ctx) {
    InspectHUDComposite(ctx);
    if(diagnostics)TracePrimaryShader(ctx);
    if(!diagnostics||sceneCapture!=1)return;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(hash!=0x727b30bff010ee9cull||InterlockedCompareExchange(&sceneCapture,0,1)!=1)return;
    ID3D11Buffer *cb=0;ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,1,&cb);if(cb){DumpBuffer(ctx,cb,L"scene-compositor-cb.bin");ID3D11Buffer_Release(cb);}
    ID3D11ShaderResourceView *textures[2]={0};ID3D11DeviceContext_PSGetShaderResources(ctx,0,2,textures);
    for(UINT i=0;i<2;++i)if(textures[i]){ID3D11Resource *resource=0;ID3D11ShaderResourceView_GetResource(textures[i],&resource);CaptureTexture(ctx,resource,i?L"scene-t1":L"scene-t0");ID3D11Resource_Release(resource);ID3D11ShaderResourceView_Release(textures[i]);}
    CaptureTarget(ctx,L"scene-before");sceneCaptureContext=ctx;
}
static void CaptureSceneAfter(ID3D11DeviceContext *ctx) {
    if(hudCaptureContext==ctx){WCHAR name[100];lstrcpyW(name,hudCapturePrefix);lstrcatW(name,L"-after");CaptureTarget(ctx,name);hudCaptureContext=0;UILog("HUD compositor textures/constants/before/after dump saved.\r\n");}
    if(sceneCaptureContext==ctx){CaptureTarget(ctx,L"scene-after");sceneCaptureContext=0;UILog("Scene compositor texture capture saved.\r\n");}
}
