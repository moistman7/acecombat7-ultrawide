#include "Ultrawide.c"
#include "TestMenuVS.h"
static BYTE uiPixels[1920*1080*4],code[32768];
static BYTE footerBaseline[3440*4],descriptionBaseline[3440*4];
static BYTE overscanBaseline[3][3440*4];
static float viewConstants[540],primitiveConstants[40],materialConstants[28];
static void Require(int result,UINT reason){if(!result)ExitProcess(reason);}
void TestMain(void){
    Paths();WCHAR system[MAX_PATH];GetSystemDirectoryW(system,MAX_PATH);lstrcatW(system,L"\\d3d11.dll");HMODULE module=LoadLibraryW(system);CreateDeviceFn create=(CreateDeviceFn)GetProcAddress(module,"D3D11CreateDevice");ID3D11Device *device=0;ID3D11DeviceContext *ctx=0;Require(SUCCEEDED(create(0,D3D_DRIVER_TYPE_WARP,0,0,0,0,D3D11_SDK_VERSION,&device,0,&ctx)),1);Instrument(device,ctx);
    HANDLE file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\96e1f6f048250fec-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);Require(file!=INVALID_HANDLE_VALUE,2);DWORD size;ReadFile(file,code,sizeof(code),&size,0);CloseHandle(file);ID3D11PixelShader *ps=0;Require(SUCCEEDED(ID3D11Device_CreatePixelShader(device,code,size,0,&ps)),3);ID3D11DeviceContext_PSSetShader(ctx,ps,0,0);ID3D11VertexShader *vs=0;Require(SUCCEEDED(ID3D11Device_CreateVertexShader(device,TestMenuVS,sizeof(TestMenuVS),0,&vs)),4);ID3D11DeviceContext_VSSetShader(ctx,vs,0,0);ID3D11DeviceContext_IASetPrimitiveTopology(ctx,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for(UINT y=0;y<1080;++y)for(UINT x=0;x<1920;++x){UINT p=(y*1920+x)*4;uiPixels[p]=0;uiPixels[p+1]=32;uiPixels[p+2]=64;uiPixels[p+3]=255;if(y>=400&&y<680&&x>=640&&x<960){uiPixels[p+1]=255;uiPixels[p+2]=0;}if(((y>=820&&y<930)||(y>=990&&y<1075))&&x>=640&&x<1280&&(x/40)%2==0){uiPixels[p]=255;uiPixels[p+1]=uiPixels[p+2]=0;}if(y>=620&&y<960&&x>=1600&&x<1800){uiPixels[p]=0;uiPixels[p+1]=uiPixels[p+2]=255;}}
    D3D11_TEXTURE2D_DESC desc={1920,1080,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET,0,0};D3D11_SUBRESOURCE_DATA initial={uiPixels,1920*4,0};ID3D11Texture2D *source=0,*target=0,*staging=0;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,&initial,&source)),5);ID3D11ShaderResourceView *srv=0;Require(SUCCEEDED(ID3D11Device_CreateShaderResourceView(device,(ID3D11Resource*)source,0,&srv)),6);ID3D11DeviceContext_PSSetShaderResources(ctx,0,1,&srv);
    desc.Width=3440;desc.Height=1440;desc.BindFlags=D3D11_BIND_RENDER_TARGET;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&target)),7);ID3D11RenderTargetView *rt=0;Require(SUCCEEDED(ID3D11Device_CreateRenderTargetView(device,(ID3D11Resource*)target,0,&rt)),8);ID3D11DeviceContext_OMSetRenderTargets(ctx,1,&rt,0);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Require(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,0,&staging)),9);
    D3D11_SAMPLER_DESC samplerDesc={0};samplerDesc.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;samplerDesc.AddressU=samplerDesc.AddressV=samplerDesc.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;samplerDesc.MaxAnisotropy=1;samplerDesc.ComparisonFunc=D3D11_COMPARISON_NEVER;samplerDesc.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState *sampler=0;Require(SUCCEEDED(ID3D11Device_CreateSamplerState(device,&samplerDesc,&sampler)),10);ID3D11DeviceContext_PSSetSamplers(ctx,0,1,&sampler);
    D3D11_RASTERIZER_DESC raster={0};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_BACK; /* The game culls back faces; the edge shader must keep the winding. */raster.DepthClipEnable=TRUE;ID3D11RasterizerState *rs=0;Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),11);ID3D11DeviceContext_RSSetState(ctx,rs);D3D11_VIEWPORT vp={0,0,3440,1440,0,1};ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);
    materialConstants[16]=materialConstants[17]=materialConstants[18]=1;materialConstants[25]=1;ID3D11Buffer *buffers[3]={0};D3D11_BUFFER_DESC bd={sizeof(viewConstants),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};initial.pSysMem=viewConstants;Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&buffers[0])),12);bd.ByteWidth=sizeof(primitiveConstants);initial.pSysMem=primitiveConstants;Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&buffers[1])),13);bd.ByteWidth=sizeof(materialConstants);initial.pSysMem=materialConstants;Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&bd,&initial,&buffers[2])),14);ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,3,buffers);
    ID3D11Buffer *orderCB=0;float order[4]={0};D3D11_BUFFER_DESC orderDesc={16,D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};Require(SUCCEEDED(ID3D11Device_CreateBuffer(device,&orderDesc,0,&orderCB)),26);ID3D11DeviceContext_VSSetConstantBuffers(ctx,0,1,&orderCB);
    /* Both windings: the game culls back faces, so the edge shader must emit triangles with the source winding. */
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),27);ID3D11DeviceContext_RSSetState(ctx,rs);
    UINT baselineMin=0,baselineMax=0;for(int mode=0;mode<3;++mode){const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);menuEdgeExtension=mode!=0;menuSpan=mode==2;menuWorldTick=mode==2?GetTickCount64():0;MenuEdgeState state;int changed=ApplyMenuEdges(ctx,&state);Require(changed==(mode!=0),15);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);if(changed)RestoreMenuEdges(ctx,&state);ID3D11GeometryShader *actualGS=0;ID3D11DeviceContext_GSGetShader(ctx,&actualGS,0,0);Require(!actualGS,16);ID3D11SamplerState *actualSampler=0;ID3D11DeviceContext_PSGetSamplers(ctx,0,1,&actualSampler);Require(actualSampler==sampler,17);ID3D11SamplerState_Release(actualSampler);
        ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),18);BYTE *row=(BYTE*)map.pData+720*map.RowPitch;UINT min=3440,max=0,count=0;for(UINT x=0;x<3440;++x)if(row[x*4]<10&&row[x*4+1]>240&&row[x*4+2]<10){if(x<min)min=x;if(x>max)max=x;++count;}Require(count>400&&count<450,19);if(mode){Require(row[0]==0&&row[1]==32&&row[2]==64,20);Require(row[3439*4]==0&&row[3439*4+1]==32&&row[3439*4+2]==64,21);if(mode==1)Require(min==baselineMin&&max==baselineMax,22);else Require(min+440==baselineMin&&max+440==baselineMax,25); /* Span: the left half moves to the left edge, unscaled. */}else{Require(row[0]==255&&row[3439*4]==255,23);baselineMin=min;baselineMax=max;}for(UINT rowY=1000;rowY<=1240;rowY+=240){BYTE *miniRow=(BYTE*)map.pData+rowY*map.RowPitch;UINT miniMin=3440,miniCount=0;for(UINT x=0;x<3440;++x)if(miniRow[x*4]<10&&miniRow[x*4+1]>240&&miniRow[x*4+2]>240){if(x<miniMin)miniMin=x;++miniCount;}Require(miniCount>=266&&miniCount<=267,34);Require(miniMin==(mode==2?3013u:2573u),35);}BYTE *footer=(BYTE*)map.pData+1380*map.RowPitch;if(mode==0)BytesCopy(footerBaseline,footer,sizeof(footerBaseline));else for(UINT x=600*4;x<2800*4;++x)Require(footer[x]==footerBaseline[x],28);ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
    }}
    /* Perspective hangar plane: w varies per vertex. Footer glyphs must retain
       their positions and gaps in both source windings while span is active. */
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;order[1]=1;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),29);ID3D11DeviceContext_RSSetState(ctx,rs);
        for(int mode=0;mode<3;++mode){const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);menuEdgeExtension=mode!=0;menuSpan=mode==1;menuWorldTick=GetTickCount64();MenuEdgeState state;int changed=ApplyMenuEdges(ctx,&state);Require(changed==(mode!=0),30);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);if(changed)RestoreMenuEdges(ctx,&state);
            ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),31);BYTE *footer=(BYTE*)map.pData+1380*map.RowPitch;UINT glyphs=0,different=0;
            if(mode==2){BYTE *middle=(BYTE*)map.pData+720*map.RowPitch;Require(middle[0]==0&&middle[1]==32&&middle[3439*4]==0&&middle[3439*4+1]==32,56);}
            for(UINT x=600;x<2800;++x){if(footer[x*4]>240&&footer[x*4+1]<10)++glyphs;if(mode)for(UINT c=0;c<4;++c)if(footer[x*4+c]!=footerBaseline[x*4+c])++different;}
            BYTE *description=(BYTE*)map.pData+1260*map.RowPitch;if(mode){UINT descriptionDiff=0;for(UINT x=600*4;x<2800*4;++x)if(description[x]!=descriptionBaseline[x])++descriptionDiff;Require(descriptionDiff<16,36);}else BytesCopy(descriptionBaseline,description,sizeof(descriptionBaseline));Require(glyphs>200&&glyphs<600,32);if(mode)Require(different<16,33);else BytesCopy(footerBaseline,footer,sizeof(footerBaseline));ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
        }
    }
    /* Actual hangar dimensions: flat and 4.08% taller than the viewport.
       Extend the sides while preserving center pixels at multiple heights. */
    menuFit=menuSpan=0;menuBlackBars=1;
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;order[1]=0;order[2]=1.0408f;order[3]=0;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),57);ID3D11DeviceContext_RSSetState(ctx,rs);
        for(int enabled=0;enabled<2;++enabled){menuEdgeExtension=enabled;const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);MenuEdgeState actual;int applied=ApplyMenuEdges(ctx,&actual);Require(applied==enabled,58);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);if(applied)RestoreMenuEdges(ctx,&actual);
            ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),59);
            const UINT rows[3]={0,720,1380};for(UINT r=0;r<3;++r){BYTE *row=(BYTE*)map.pData+rows[r]*map.RowPitch;if(!enabled)BytesCopy(overscanBaseline[r],row,sizeof(overscanBaseline[r]));else{UINT different=0;for(UINT x=400*4;x<3040*4;++x)if(row[x]!=overscanBaseline[r][x])++different;Require(different<64,60);Require(row[0]==0&&row[1]==32&&row[3439*4]==0&&row[3439*4+1]==32,61);}}
            ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
        }
    }
    /* Captured hangar depth, with scale moving across the old 2.02 gate.
       Every step must extend sides and preserve the center, in both windings. */
    menuBlackBars=1;menuSpan=0;
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;order[1]=2;order[3]=0;raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),62);ID3D11DeviceContext_RSSetState(ctx,rs);
      for(UINT step=0;step<4;++step){const float scales[4]={.93f,.98f,1.0f,1.0408f};order[2]=scales[step];ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);
        for(int enabled=0;enabled<2;++enabled){menuEdgeExtension=enabled;menuFit=0;const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);MenuEdgeState transition;int applied=ApplyMenuEdges(ctx,&transition);Require(applied==enabled,63);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);if(applied)RestoreMenuEdges(ctx,&transition);
          ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),64);BYTE *row=(BYTE*)map.pData+720*map.RowPitch;
          if(!enabled)BytesCopy(footerBaseline,row,sizeof(footerBaseline));else {Require(row[0]==0&&row[1]==32&&row[3439*4]==0&&row[3439*4+1]==32,65);for(UINT x=600*4;x<2800*4;++x)Require(row[x]==footerBaseline[x],66);}ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
        }
      }
    }
    /* Title plane inset on all four sides: fit to height, retain pillarboxes. */
    menuFit=1;menuEdgeExtension=1;menuBlackBars=1;menuSpan=0;
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;order[1]=0;order[2]=.93f;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),37);ID3D11DeviceContext_RSSetState(ctx,rs);
        const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);MenuEdgeState fitState;Require(ApplyMenuEdges(ctx,&fitState),38);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);RestoreMenuEdges(ctx,&fitState);
        ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),39);
        BYTE *top=(BYTE*)map.pData+1720*4,*bottom=(BYTE*)map.pData+1439*map.RowPitch+1720*4,*row=(BYTE*)map.pData+720*map.RowPitch;Require(top[0]==0&&top[1]==32&&bottom[0]==0&&bottom[1]==32,40);Require(row[0]==255&&row[439*4]==255&&row[3000*4]==255&&row[3439*4]==255,41);Require(row[440*4]==0&&row[2999*4]==0,42);
        UINT glyphs=0;for(UINT x=0;x<3440;++x)if(row[x*4]<10&&row[x*4+1]>240&&row[x*4+2]<10)++glyphs;Require(glyphs>=426&&glyphs<=427,43);ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
    }
    menuBlackBars=0;
    /* Background-only material: cover both edges with artwork and use the
       same scale on both axes. UI tests above still require unchanged glyphs. */
    file=CreateFileW(L"D:\\SteamLibrary\\steamapps\\common\\ACE COMBAT 7\\Ace7NativeUI\\87d48047026c963d-ps.cso",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);Require(file!=INVALID_HANDLE_VALUE,48);ReadFile(file,code,sizeof(code),&size,0);CloseHandle(file);ID3D11PixelShader *backgroundPS=0;Require(SUCCEEDED(ID3D11Device_CreatePixelShader(device,code,size,0,&backgroundPS)),49);ID3D11DeviceContext_PSSetShader(ctx,backgroundPS,0,0);
    for(int policy=0;policy<2;++policy){menuBlackBars=policy;
    for(int material=0;material<2;++material){UINT known=(UINT)material;ID3D11Texture2D_SetPrivateData(source,&menuArtTag,sizeof(known),&known);ID3D11DeviceContext_PSSetShader(ctx,material?ps:backgroundPS,0,0);
    menuEdgeExtension=1;menuFit=menuSpan=0;order[1]=order[2]=order[3]=0;
    for(int winding=0;winding<2;++winding){order[0]=(float)winding;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),50);ID3D11DeviceContext_RSSetState(ctx,rs);
        const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);MenuEdgeState background;Require(ApplyMenuEdges(ctx,&background),51);((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);RestoreMenuEdges(ctx,&background);
        ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),52);
        BYTE *row=(BYTE*)map.pData+720*map.RowPitch;if(policy)Require(row[0]==0&&row[1]==0&&row[2]==0&&row[3439*4]==0&&row[3439*4+1]==0,53);else Require(row[0]==0&&row[1]==32&&row[3439*4]==0&&row[3439*4+1]==32,53);UINT count=0;for(UINT x=0;x<3440;++x)if(row[x*4]<10&&row[x*4+1]>240&&row[x*4+2]<10)++count;Require(policy?(count>=426&&count<=427):(count>=573&&count<=574),54);
        /* The 280-source-pixel marker height scales by the same 3440/1920. */
        UINT vertical=0;for(UINT y=0;y<1440;++y){BYTE *pixel=(BYTE*)map.pData+y*map.RowPitch+1400*4;if(pixel[0]<10&&pixel[1]>240&&pixel[2]<10)++vertical;}Require(policy?(vertical>=373&&vertical<=374):(vertical>=501&&vertical<=502),55);ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
    }
    }}menuBlackBars=0;UINT notArt=0;ID3D11Texture2D_SetPrivateData(source,&menuArtTag,sizeof(notArt),&notArt);
    ID3D11DeviceContext_PSSetShader(ctx,ps,0,0);
    /* A new 3D scene discards menu matte state before map/hangar UI. */
    menuBlackBars=menuEdgeExtension=1;Require(ApplyMenuMatte(ctx)&&CurrentMenuMatte(ctx),67);NoteMenuWorldDraw(ctx,6000);Require(!CurrentMenuMatte(ctx),68);
    order[0]=order[1]=order[3]=0;order[2]=.93f;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);
    MenuEdgeState mapState;Require(ApplyMenuEdges(ctx,&mapState)&&!mapState.matte,69);RestoreMenuEdges(ctx,&mapState);menuBlackBars=0;
    /* Inactive and sliding planes must retain their exact source placement.
       A previous fit recentered these planes and exposed hidden submenu text. */
    menuFit=menuSpan=0;order[1]=order[2]=0;
    for(int winding=0;winding<2;++winding){
        order[0]=(float)winding;raster.FrontCounterClockwise=winding;ID3D11RasterizerState_Release(rs);Require(SUCCEEDED(ID3D11Device_CreateRasterizerState(device,&raster,&rs)),44);ID3D11DeviceContext_RSSetState(ctx,rs);
        for(int hidden=0;hidden<2;++hidden){order[3]=hidden?3.0f:.2f;ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)orderCB,0,0,order,0,0);
            for(int enabled=0;enabled<2;++enabled){
                menuEdgeExtension=enabled;const float white[4]={1,1,1,1};ID3D11DeviceContext_ClearRenderTargetView(ctx,rt,white);MenuEdgeState shifted;int applied=ApplyMenuEdges(ctx,&shifted);Require(applied==enabled,45);
                ((void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT))Original(ctx,contexts,contextCount)[13])(ctx,6,0);if(applied)RestoreMenuEdges(ctx,&shifted);
                ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)staging,(ID3D11Resource*)target);D3D11_MAPPED_SUBRESOURCE map;Require(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)staging,0,D3D11_MAP_READ,0,&map)),46);
                BYTE *row=(BYTE*)map.pData+720*map.RowPitch;if(!enabled)BytesCopy(footerBaseline,row,sizeof(footerBaseline));else for(UINT x=0;x<sizeof(footerBaseline);++x)Require(row[x]==footerBaseline[x],47);
                ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)staging,0);
            }
        }
    }
    ID3D11DeviceContext_GSSetShader(ctx,menuEdgeResources[0].shader,0,0);MenuEdgeState state;Require(!ApplyMenuEdges(ctx,&state),24);ID3D11DeviceContext_GSSetShader(ctx,0,0,0);ExitProcess(0);
}
