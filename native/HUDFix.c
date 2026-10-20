#include "ScreenHUD.h"
static LONG hudMode;
typedef struct {ID3D11DeviceContext *context;ID3D11Buffer *buffer;UINT size;ID3D11PixelShader *screenPS;ID3D11Buffer *screenCB,*materialCB;UINT materialSize;} HUDResources;
static HUDResources hudResources[32];
static LONG hudCount,hudHits;
static int IsHUDFinalBlend(uint64_t hash) {return hash==0xe718d83b50978835ull||hash==0x1de2e550b4a941b3ull;}
typedef struct {ID3D11Buffer *original,*layout;ID3D11PixelShader *shader;int replacedShader;UINT slot;ID3D11Buffer *material;int replacedMaterial;} HUDState;
/* Screen mode moves the radar and gauges to the screen edges, but the final HUD blend (e718) still darkens a
   backing rectangle (cb0[19], 16:9 HUD coordinates) at the old position, leaving the moved radar washed out.
   Move that rectangle with its group: same edge offset ScreenHUD.hlsl applies. */
static void MoveHUDBacking(ID3D11DeviceContext *ctx,HUDResources *fix,HUDState *state) {
    ID3D11Buffer *material=0;ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,1,&material);if(!material)return;
    BufferMirror *mirror=Mirror((ID3D11Resource*)material,1);InitialSnapshot(ctx,mirror);
    if(!mirror||!mirror->valid||mirror->size<416){ID3D11Buffer_Release(material);return;}
    if(fix->materialSize!=mirror->size){if(fix->materialCB)ID3D11Buffer_Release(fix->materialCB);fix->materialCB=0;fix->materialSize=mirror->size;ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);D3D11_BUFFER_DESC desc={mirror->size,D3D11_USAGE_DYNAMIC,D3D11_BIND_CONSTANT_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};ID3D11Device_CreateBuffer(device,&desc,0,&fix->materialCB);ID3D11Device_Release(device);}
    D3D11_MAPPED_SUBRESOURCE map;if(!fix->materialCB||FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)fix->materialCB,0,D3D11_MAP_WRITE_DISCARD,0,&map))){ID3D11Buffer_Release(material);return;}
    BytesCopy(map.pData,mirror->data,mirror->size);
    float virtualWidth=(float)hudHeight*(16.0f/9.0f),offset=((float)hudWidth-virtualWidth)*.5f/virtualWidth;
    /* The portrait-enabled final blend also tests cb0[20]. Disabled rectangles
       have reversed/equal bounds and must remain disabled. */
    for(UINT i=19;i<=20;++i){float *rect=(float*)mirror->data+i*4,*moved=(float*)map.pData+i*4;
        if(!(rect[2]>rect[0]&&rect[3]>rect[1]))continue;
        float center=(rect[0]+rect[2])*.5f;
        if(center<.4f){moved[0]-=offset;moved[2]-=offset;}else if(center>.6f){moved[0]+=offset;moved[2]+=offset;}
    }
    ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)fix->materialCB,0);
    state->material=material;state->replacedMaterial=1;ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,1,&fix->materialCB);
}
static int ApplyHUD(ID3D11DeviceContext *ctx,HUDState *state) {
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(!IsHUDFinalBlend(hash)&&hash!=0xeb4ce6dce4ada70aull&&hash!=0x727b30bff010ee9cull)return 0;
    /* Cutscenes (e.g. the takeoff "Press to skip") composite the HUD with 727b, which reads the same view constants from slot 0. */
    UINT slot=hash==0x727b30bff010ee9cull?0:1;
    ID3D11Buffer *original=0;ID3D11DeviceContext_PSGetConstantBuffers(ctx,slot,1,&original);if(!original)return 0;
    BufferMirror *mirror=Mirror((ID3D11Resource*)original,1);InitialSnapshot(ctx,mirror);
    if(!mirror||!mirror->valid||mirror->size<2064){ID3D11Buffer_Release(original);return 0;}
    float *view=(float*)mirror->data;
    if(view[508]<1||view[509]<1||view[510]<=0){ID3D11Buffer_Release(original);return 0;}
    HUDResources *fix=0;for(LONG i=0;i<hudCount;++i)if(hudResources[i].context==ctx)fix=&hudResources[i];
    if(!fix&&hudCount<32){fix=&hudResources[hudCount++];fix->context=ctx;}
    if(!fix){ID3D11Buffer_Release(original);return 0;}
    if(fix->size!=mirror->size){if(fix->buffer)ID3D11Buffer_Release(fix->buffer);fix->buffer=0;fix->size=mirror->size;ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);D3D11_BUFFER_DESC desc={mirror->size,D3D11_USAGE_DYNAMIC,D3D11_BIND_CONSTANT_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};ID3D11Device_CreateBuffer(device,&desc,0,&fix->buffer);ID3D11Device_Release(device);}
    if(!fix->buffer){ID3D11Buffer_Release(original);return 0;}
    D3D11_MAPPED_SUBRESOURCE map;if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)fix->buffer,0,D3D11_MAP_WRITE_DISCARD,0,&map))){ID3D11Buffer_Release(original);return 0;}
    BytesCopy(map.pData,view,mirror->size);float *patched=map.pData;
    float ratio=view[508]/(view[509]*(16.0f/9.0f));float shift=((float)hudWidth*1080/hudHeight-1920)/3840;patched[504]+=shift/(view[510]*ratio);
    ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)fix->buffer,0);state->original=original;state->replacedShader=0;state->slot=slot;state->replacedMaterial=0;
    ID3D11DeviceContext_PSSetConstantBuffers(ctx,slot,1,&fix->buffer);
    if(hudMode==1&&hash==0xeb4ce6dce4ada70aull&&hudWidth*9>hudHeight*16){
        ID3D11Device *device=0;ID3D11DeviceContext_GetDevice(ctx,&device);
        if(!fix->screenPS)ID3D11Device_CreatePixelShader(device,ScreenHUD,sizeof(ScreenHUD),0,&fix->screenPS);
        if(!fix->screenCB){D3D11_BUFFER_DESC desc={16,D3D11_USAGE_DYNAMIC,D3D11_BIND_CONSTANT_BUFFER,D3D11_CPU_ACCESS_WRITE,0,0};ID3D11Device_CreateBuffer(device,&desc,0,&fix->screenCB);}ID3D11Device_Release(device);
        if(fix->screenPS&&fix->screenCB&&SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)fix->screenCB,0,D3D11_MAP_WRITE_DISCARD,0,&map))){float *layout=map.pData;layout[0]=(float)hudWidth;layout[1]=(float)hudHeight;layout[2]=((float)hudWidth-(float)hudHeight*(16.0f/9.0f))*.5f;layout[3]=(float)hudHeight*(16.0f/9.0f)*.25f;ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)fix->screenCB,0);ID3D11DeviceContext_PSGetShader(ctx,&state->shader,0,0);ID3D11DeviceContext_PSGetConstantBuffers(ctx,13,1,&state->layout);ID3D11DeviceContext_PSSetShader(ctx,fix->screenPS,0,0);ID3D11DeviceContext_PSSetConstantBuffers(ctx,13,1,&fix->screenCB);state->replacedShader=1;}
    }
    if(hudMode==1&&IsHUDFinalBlend(hash)&&hudWidth*9>hudHeight*16)MoveHUDBacking(ctx,fix,state);
    if(InterlockedIncrement(&hudHits)==1)UILog("Native HUD correction active; F6 toggles 16:9 / Screen.\r\n");
    return 1;
}
static void RestoreHUD(ID3D11DeviceContext *ctx,HUDState *state) {if(state->replacedMaterial){ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,1,&state->material);ID3D11Buffer_Release(state->material);}if(state->replacedShader){ID3D11DeviceContext_PSSetShader(ctx,state->shader,0,0);if(state->shader)ID3D11PixelShader_Release(state->shader);ID3D11DeviceContext_PSSetConstantBuffers(ctx,13,1,&state->layout);if(state->layout)ID3D11Buffer_Release(state->layout);}ID3D11DeviceContext_PSSetConstantBuffers(ctx,state->slot,1,&state->original);ID3D11Buffer_Release(state->original);}
