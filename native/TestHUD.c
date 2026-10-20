#include "Ultrawide.c"

#include "TestHUDVS.h"

static BYTE hudPixels[2560*1440*4],scenePixels[3440*1440*4],shaderCode[32768];
static float viewData[516],materialData[84];
static void Require(int result,UINT code){if(!result)ExitProcess(code);}
void TestMain(void) {
    Paths();WCHAR path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);lstrcatW(path,L"\\d3d11.dll");HMODULE module=LoadLibraryW(path);Require(module!=0,1);
    CreateDeviceFn create=(CreateDeviceFn)GetProcAddress(module,"D3D11CreateDevice");ID3D11Device *device=0;ID3D11DeviceContext *ctx=0;
    Require(SUCCEEDED(create(0,D3D_DRIVER_TYPE_WARP,0,0,0,0,D3D11_SDK_VERSION,&device,0,&ctx)),2);Instrument(device,ctx);
    HANDLE file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\eb4ce6dce4ada70a-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);Require(file!=INVALID_HANDLE_VALUE,3);DWORD size;ReadFile(file,shaderCode,sizeof(shaderCode),&size,0);CloseHandle(file);
    ID3D11PixelShader *originalPS=0;Require(SUCCEEDED(ID3D11Device_CreatePixelShader(device,shaderCode,size,0,&originalPS)),4);
    ID3D11VertexShader *vs=0;Require(SUCCEEDED(ID3D11Device_CreateVertexShader(device,TestHUDVS,sizeof(TestHUDVS),0,&vs)),5);ID3D11DeviceContext_VSSetShader(ctx,vs,0,0);ID3D11DeviceContext_IASetPrimitiveTopology(ctx,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for(UINT y=0;y<1440;++y)for(UINT x=0;x<2560;++x){UINT p=(y*2560+x)*4;int channel=-1;if(y>=936&&y<1224){if(x>=128&&x<512)channel=0;if(x>=2048&&x<2432)channel=2;}if(y>=576&&y<864&&x>=1216&&x<1344)channel=1;if(channel>=0){hudPixels[p+(UINT)channel]=255;hudPixels[p+3]=255;}}
    D3D11_TEXTURE2D_DESC desc={2560,1440,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE,0,0};D3D11_SUBRESOURCE_DATA initial={hudPixels,2560*4,0};ID3D11Texture2D *hudTexture=0,*sceneTexture=0,*target=0,*readback=0;ID3D11ShaderResourceView *srvs[2]={0};
    Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,&initial,&hudTexture)),6);Require(SUCCEEDED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)hudTexture,0,&srvs[1])),7);
    desc.Width=3440;initial.pSysMem=scenePixels;initial.SysMemPitch=3440*4;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,&initial,&sceneTexture)),8);Require(SUCCEEDED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)sceneTexture,0,&srvs[0])),9);ID3D11DeviceContext_PSSetShaderResources(ctx,0,2,srvs);
    desc.BindFlags=D3D11_BIND_RENDER_TARGET;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&target)),10);ID3D11RenderTargetView *rt=0;Require(SUCCEEDED(ID3D11Device_CreateRenderTargetView(device,(ID3D11Resource*)target,0,&rt)),11);ID3D11DeviceContext_OMSetRenderTargets(ctx,1,&rt,0);
    desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&readback)),12);
    D3D11_SAMPLER_DESC sampler={0};sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=D3D11_FLOAT32_MAX;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D11_COMPARISON_NEVER;ID3D11SamplerState *sampleState=0;Require(SUCCEEDED(ID3D11Device_CreateSamplerState(device,&sampler,&sampleState)),13);ID3D11DeviceContext_PSSetSamplers(ctx,0,1,&sampleState);
    D3D11_RASTERIZER_DESC raster={0};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;ID3D11RasterizerState *rs=0;Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),14);ID3D11DeviceContext_RSSetState(ctx,rs);D3D11_DEPTH_STENCIL_DESC depthDesc={0};depthDesc.DepthEnable=FALSE;depthDesc.DepthFunc=D3D11_COMPARISON_ALWAYS;ID3D11DepthStencilState *depthState=0;Require(SUCCEEDED(ID3D11Device_CreateDepthStencilState(device,&depthDesc,&depthState)),29);ID3D11DeviceContext_OMSetDepthStencilState(ctx,depthState,0);D3D11_VIEWPORT vp={0,0,3440,1440,0,1};ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);
    materialData[76]=materialData[77]=materialData[80]=materialData[81]=2;materialData[78]=materialData[79]=materialData[82]=materialData[83]=3;
    viewData[508]=3440;viewData[509]=1440;viewData[510]=1.0f/3440;viewData[511]=1.0f/1440;viewData[512]=3440;viewData[513]=1440;viewData[514]=1.0f/3440;viewData[515]=1.0f/1440;
    D3D11_BUFFER_DESC bd={sizeof(materialData),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};initial.pSysMem=materialData;ID3D11Buffer *buffers[2]={0};Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&buffers[0])),15);bd.ByteWidth=sizeof(viewData);initial.pSysMem=viewData;Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&buffers[1])),16);ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,2,buffers);
    Require(hudPixels[(1000*2560+128)*4]==255,31);ID3D11ShaderResourceView *boundSRV[2]={0};ID3D11DeviceContext_PSGetShaderResources(ctx,0,2,boundSRV);Require(boundSRV[1]==srvs[1],32);if(boundSRV[0])ID3D11ShaderResourceView_Release(boundSRV[0]);if(boundSRV[1])ID3D11ShaderResourceView_Release(boundSRV[1]);for(int mode=0;mode<2;++mode){hudMode=mode;ID3D11DeviceContext_PSSetShader(ctx,originalPS,0,0);HUDState state;Require(ApplyHUD(ctx,&state),17);Require(state.replacedShader==mode,18);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,3,0);RestoreHUD(ctx,&state);ID3D11PixelShader *restored=0;ID3D11DeviceContext_PSGetShader(ctx,&restored,0,0);Require(restored==originalPS,19);ID3D11PixelShader_Release(restored);
        ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)readback,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)readback,0,D3D11_MAP_READ,0,&map)),20);
        UINT min[3]={3440,3440,3440},max[3]={0},counts[3]={0};for(UINT channel=0;channel<3;++channel){UINT row=channel==1?720:1000;BYTE *pixels=(BYTE*)map.pData+row*map.RowPitch;for(UINT x=0;x<3440;++x)if(pixels[x*4+channel]>200){if(x<min[channel])min[channel]=x;if(x>max[channel])max[channel]=x;++counts[channel];}}
        ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)readback,0);Require(counts[0]==384&&counts[2]==384&&counts[1]==128,21+(UINT)mode);Require(min[0]==(mode?128u:568u)&&max[0]==(mode?511u:951u),23+(UINT)mode);Require(min[2]==(mode?2928u:2488u)&&max[2]==(mode?3311u:2871u),25+(UINT)mode);Require(min[1]==1656&&max[1]==1783,27+(UINT)mode);
    }
    /* Exercise the actual final-blend variants over a bright scene. The old
       fixture only tested opaque glyph positions, so it missed pale panels. */
    for(UINT p=0;p<2560*1440;++p){hudPixels[p*4]=0;hudPixels[p*4+1]=32;hudPixels[p*4+2]=0;hudPixels[p*4+3]=128;}
    for(UINT p=0;p<3440*1440;++p){scenePixels[p*4]=scenePixels[p*4+1]=scenePixels[p*4+2]=128;scenePixels[p*4+3]=255;}
    ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)hudTexture,0,0,hudPixels,2560*4,0);
    ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)sceneTexture,0,0,scenePixels,3440*4,0);
    BYTE zero[4]={0};desc.Width=desc.Height=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=0;initial.pSysMem=zero;initial.SysMemPitch=4;
    ID3D11Texture2D *blank=0;ID3D11ShaderResourceView *blankView=0;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,&initial,&blank)),40);Require(SUCCEEDED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)blank,0,&blankView)),41);
    ID3D11ShaderResourceView *finalViews[5]={srvs[0],srvs[1],blankView,blankView,blankView};ID3D11DeviceContext_PSSetShaderResources(ctx,0,5,finalViews);
    ID3D11SamplerState *finalSamplers[5]={sampleState,sampleState,sampleState,sampleState,sampleState};ID3D11DeviceContext_PSSetSamplers(ctx,0,5,finalSamplers);
    for(UINT i=0;i<84;++i)materialData[i]=0;
    materialData[76]=.05f;materialData[77]=.67f;materialData[78]=.21f;materialData[79]=.95f;
    materialData[80]=.83f;materialData[81]=.09f;materialData[82]=.95f;materialData[83]=.34f;
    /* Final shaders have 26 float4 constants, unlike the intermediate's 21. */
    static float finalMaterial[104];BytesCopy(finalMaterial,materialData,sizeof(materialData));finalMaterial[92]=-1;finalMaterial[93]=1;finalMaterial[95]=1;finalMaterial[96]=1;
    bd.ByteWidth=sizeof(finalMaterial);initial.pSysMem=finalMaterial;ID3D11Buffer *finalCB=0;Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&finalCB)),42);ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,1,&finalCB);
    const WCHAR *finalPaths[2]={L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\e718d83b50978835-ps.cso",L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\1de2e550b4a941b3-ps.cso"};
    for(UINT variant=0;variant<2;++variant){file=CreateFileW(finalPaths[variant],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);Require(file!=INVALID_HANDLE_VALUE,43);ReadFile(file,shaderCode,sizeof(shaderCode),&size,0);CloseHandle(file);ID3D11PixelShader *finalPS=0;Require(SUCCEEDED(ID3D11Device_CreatePixelShader(device,shaderCode,size,0,&finalPS)),44);
        for(int mode=0;mode<2;++mode){hudMode=mode;ID3D11DeviceContext_PSSetShader(ctx,finalPS,0,0);HUDState state;Require(ApplyHUD(ctx,&state),45);Require(state.replacedMaterial==mode,46);
            ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,3,0);RestoreHUD(ctx,&state);
            ID3D11Buffer *restoredCB=0;ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,1,&restoredCB);Require(restoredCB==finalCB,47);ID3D11Buffer_Release(restoredCB);
            ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)readback,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)readback,0,D3D11_MAP_READ,0,&map)),48);
            BYTE *radar=(BYTE*)map.pData+1000*map.RowPitch+(mode?333:773)*4;BYTE *portrait=(BYTE*)map.pData+300*map.RowPitch+(mode?3158:2718)*4;BYTE *outside=(BYTE*)map.pData+720*map.RowPitch+1720*4;
            Require(radar[0]>=62&&radar[0]<=65,49);if(variant)Require(portrait[0]>=62&&portrait[0]<=65,50);Require(outside[0]==128,51);ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)readback,0);
        }ID3D11PixelShader_Release(finalPS);
    }
    ExitProcess(0);
}
