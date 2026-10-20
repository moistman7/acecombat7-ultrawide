#include "MenuEdges.h"
#include "MenuEdgesSlate.h"
static LONG menuEdgeExtension,menuSpan;
static int menuFit,menuBlackBars;
typedef struct {ID3D11DeviceContext *context;ID3D11GeometryShader *shader,*slateShader;ID3D11SamplerState *sampler,*sourceSampler;ID3D11Buffer *layout;float layoutSpan;UINT layoutWidth,layoutHeight;} MenuEdgeResources;
static MenuEdgeResources menuEdgeResources[32];
static LONG menuEdgeCount,menuEdgeHits;
/* The briefing intro movie and the hangar cutscene video use the same pillarboxed plate draw. Only the
   briefing renders 3D geometry behind it, which shows in the side bars; fill those, keep the video's black bars. */
static volatile ULONGLONG menuWorldTick;
static void NoteMenuWorldDraw(ID3D11DeviceContext *ctx,UINT count) {
    if(!menuEdgeExtension||count<5000)return;UINT width=0,height=0;if(PrimaryTarget(ctx,&width,&height)&&width*9>height*16){menuWorldTick=GetTickCount64();ResetCurrentMenuMatte(ctx);}
}
typedef struct {ID3D11GeometryShader *shader;ID3D11SamplerState *sampler;D3D11_RECT scissor[16];UINT scissorCount;int changedScissor;ID3D11Buffer *gsLayout;int changedLayout;int matte;} MenuEdgeState;
static int ApplyMenuEdges(ID3D11DeviceContext *ctx,MenuEdgeState *state) {
    if(!menuEdgeExtension&&!menuFit)return 0;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    /* The 3D UI plane (two materials), or the Slate composite of a 1920x1080 layer into the left 16:9 area. */
    int plane=hash==0x96e1f6f048250fecull||hash==0x87d48047026c963dull,slate=0,plate=0;
    if(!menuEdgeExtension&&!plane)return 0;
    if(!plane&&hash==0x2d58759ed62f1f84ull){ID3D11VertexShader *vs=0;ID3D11DeviceContext_VSGetShader(ctx,&vs,0,0);slate=GetShaderHash(vs)==0x47897216336c5479ull;if(vs)ID3D11VertexShader_Release(vs);}
    if(!plane&&hash==0x155ebdf3384010f4ull&&GetTickCount64()-menuWorldTick<250){ID3D11VertexShader *vs=0;ID3D11DeviceContext_VSGetShader(ctx,&vs,0,0);plate=GetShaderHash(vs)==0x8f3937c557a87704ull;if(vs)ID3D11VertexShader_Release(vs);}
    if(!plane&&!slate&&!plate)return 0;
#define EDGEREJECT(why) do{if((slate||plate)&&bailBudget>0){InterlockedDecrement(&bailBudget);UILog(slate?"MenuEdges slate rejected: " why "\r\n":"MenuEdges plate rejected: " why "\r\n");}return 0;}while(0)
    UINT width=0,height=0;if(!PrimaryTarget(ctx,&width,&height)||width*9<=height*16)return 0;
    if(slate){
        /* The composite is clipped to the left 16:9 area (0,0,~2563,height). That clip identifies it; the layer
           itself may be a render target (briefing card) or a plain image (mission title card). */
        D3D11_RECT clip[16];UINT clipCount=16;ID3D11DeviceContext_RSGetScissorRects(ctx,&clipCount,clip);float area=height*(16.0f/9.0f);
        if(!clipCount||clip[0].left!=0||clip[0].top!=0||clip[0].bottom<(LONG)height-1||clip[0].right<area-2||clip[0].right>area+6)return 0; /* Silent: most Slate draws land here. */
    }
    ID3D11ShaderResourceView *srv=0;ID3D11DeviceContext_PSGetShaderResources(ctx,0,1,&srv);if(!srv)EDGEREJECT("no srv");ID3D11Resource *resource=0;ID3D11ShaderResourceView_GetResource(srv,&resource);ID3D11ShaderResourceView_Release(srv);
    D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);int match=0;if(dim==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);match=desc.Width==1920&&desc.Height==1080&&(plate||slate||(desc.BindFlags&D3D11_BIND_RENDER_TARGET)!=0);}UINT backgroundComposition=MenuBackground(resource);UINT backgroundArt=MenuArt(resource);TraceMenuArt("consume",resource,0,hash,0,match,backgroundArt,backgroundArt);ID3D11Resource_Release(resource);if(!match)EDGEREJECT("texture");
    ID3D11GeometryShader *gs=0;ID3D11DeviceContext_GSGetShader(ctx,&gs,0,0);if(gs){ID3D11GeometryShader_Release(gs);EDGEREJECT("existing gs");}
    ID3D11SamplerState *original=0;ID3D11DeviceContext_PSGetSamplers(ctx,0,1,&original);if(!original)EDGEREJECT("no sampler");
    MenuEdgeResources *fix=0;for(LONG i=0;i<menuEdgeCount;++i)if(menuEdgeResources[i].context==ctx)fix=&menuEdgeResources[i];if(!fix&&menuEdgeCount<32){fix=&menuEdgeResources[menuEdgeCount++];fix->context=ctx;}if(!fix){ID3D11SamplerState_Release(original);return 0;}
    ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);
    if(plane&&!fix->shader)ID3D11Device_CreateGeometryShader(device,MenuEdges,sizeof(MenuEdges),0,&fix->shader);
    if(!plane&&!fix->slateShader)ID3D11Device_CreateGeometryShader(device,MenuEdgesSlate,sizeof(MenuEdgesSlate),0,&fix->slateShader);
    if(fix->sourceSampler!=original){if(fix->sampler)ID3D11SamplerState_Release(fix->sampler);if(fix->sourceSampler)ID3D11SamplerState_Release(fix->sourceSampler);fix->sourceSampler=original;ID3D11SamplerState_AddRef(original);fix->sampler=0;D3D11_SAMPLER_DESC desc;ID3D11SamplerState_GetDesc(original,&desc);desc.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;ID3D11Device_CreateSamplerState(device,&desc,&fix->sampler);}ID3D11Device_Release(device);
    ID3D11GeometryShader *shader=plane?fix->shader:fix->slateShader;
    if(!shader||!fix->sampler){ID3D11SamplerState_Release(original);EDGEREJECT("shader create");}
    state->matte=menuBlackBars&&(slate||plate||hash==0x87d48047026c963dull||backgroundArt||backgroundComposition||CurrentMenuMatte(ctx));
    state->shader=0;state->sampler=original;state->changedScissor=0;state->changedLayout=0;
    if(plane||slate||plate){
        /* Hangar menus (3D scene behind the UI) spread to the screen edges; other menus stay centered. */
        float spread=menuEdgeExtension&&menuSpan&&GetTickCount64()-menuWorldTick<250?1.0f:0.0f;
        float cover=menuEdgeExtension&&!menuBlackBars&&(hash==0x87d48047026c963dull||backgroundArt)?1.0f:0.0f;
        float panel=menuEdgeExtension&&!cover?1.0f:0.0f;
        float fitOnly=menuBlackBars?2.0f:menuEdgeExtension?0.0f:1.0f,layoutKey=spread+fitOnly*2+cover*4+panel*8;
        if(!fix->layout){ID3D11Device *owner=0;ID3D11DeviceContext_GetDevice(ctx,&owner);D3D11_BUFFER_DESC desc={16,D3D11_USAGE_DYNAMIC,D3D11_BIND_CONSTANT_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};ID3D11Device_CreateBuffer(owner,&desc,0,&fix->layout);ID3D11Device_Release(owner);fix->layoutSpan=-1;}
        if(!fix->layout){ID3D11SamplerState_Release(original);return 0;}
        D3D11_MAPPED_SUBRESOURCE map;if(fix->layoutSpan!=layoutKey||fix->layoutWidth!=width||fix->layoutHeight!=height){if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)fix->layout,0,D3D11_MAP_WRITE_DISCARD,0,&map))){ID3D11SamplerState_Release(original);return 0;}float *values=map.pData;values[0]=spread;values[1]=((float)width-(float)height*(16.0f/9.0f))/(float)width;values[2]=fitOnly;values[3]=cover?1.0f:panel?2.0f:0.0f;ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)fix->layout,0);fix->layoutSpan=layoutKey;fix->layoutWidth=width;fix->layoutHeight=height;}
        ID3D11DeviceContext_GSGetConstantBuffers(ctx,0,1,&state->gsLayout);ID3D11DeviceContext_GSSetConstantBuffers(ctx,0,1,&fix->layout);state->changedLayout=1;
    }
    if(slate){
        /* The composite is scissored to the 16:9 area; open it to the full target for this draw. */
        state->scissorCount=16;ID3D11DeviceContext_RSGetScissorRects(ctx,&state->scissorCount,state->scissor);
        D3D11_RECT full={0,0,(LONG)width,(LONG)height};ID3D11DeviceContext_RSSetScissorRects(ctx,1,&full);state->changedScissor=1;
    }
    ID3D11DeviceContext_GSSetShader(ctx,shader,0,0);ID3D11DeviceContext_PSSetSamplers(ctx,0,1,&fix->sampler);
    if((slate||plate)&&bailBudget>0){InterlockedDecrement(&bailBudget);UILog(slate?"MenuEdges slate applied\r\n":"MenuEdges plate applied\r\n");}
    if(InterlockedIncrement(&menuEdgeHits)==1)UILog(menuEdgeExtension?"Experimental UI-plane edge extension active (F7 toggles).\r\n":"Centered UI-plane fit active; side bars preserved.\r\n");return 1;
}
static void RestoreMenuEdges(ID3D11DeviceContext *ctx,MenuEdgeState *state) {
    if(state->matte)ApplyMenuMatte(ctx);
    ID3D11DeviceContext_GSSetShader(ctx,state->shader,0,0);ID3D11DeviceContext_PSSetSamplers(ctx,0,1,&state->sampler);ID3D11SamplerState_Release(state->sampler);
    if(state->changedScissor)ID3D11DeviceContext_RSSetScissorRects(ctx,state->scissorCount,state->scissor);
    if(state->changedLayout){ID3D11DeviceContext_GSSetConstantBuffers(ctx,0,1,&state->gsLayout);if(state->gsLayout)ID3D11Buffer_Release(state->gsLayout);}
}
