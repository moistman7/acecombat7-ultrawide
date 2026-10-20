// Native replacement for the game's HUD compositor. Move the outer groups
// through texture coordinates without stretching their glyphs or gauges.
cbuffer Material : register(b0) { float4 material[21]; }
cbuffer View : register(b1) { float4 view[129]; }
cbuffer Layout : register(b13) { float4 layout; } // width,height,centerOffset,edgeWidth
Texture2D<float4> sceneTexture : register(t0);
Texture2D<float4> hudTexture : register(t1);
SamplerState gameSampler : register(s0);
float4 main(float2 uv : TEXCOORD0) : SV_Target {
    float2 mapped=(uv*view[128].xy-view[126].xy)*view[127].zw;
    if(view[127].y>1) mapped.x*=view[127].x/(view[127].y*(16.0/9.0));
    bool gap=false;
    if(layout.z>0) {
        float virtualWidth=layout.y*(16.0/9.0);
        float screenX=uv.x*layout.x;
        if(screenX<layout.w) mapped.x+=layout.z/virtualWidth;
        else if(screenX>layout.x-layout.w) mapped.x-=layout.z/virtualWidth;
        else gap=mapped.x<.25 || mapped.x>.75;
    }
    bool rectA=all(mapped>=material[20].xy)&&all(mapped<=material[20].zw);
    bool rectB=all(mapped>=material[19].xy)&&all(mapped<=material[19].zw);
    float4 hud=gap?float4(0,0,0,0):hudTexture.Sample(gameSampler,mapped);
    float4 scene=sceneTexture.Sample(gameSampler,uv);
    float4 masked=hud+(1-hud.a)*(hud.a*scene);
    float4 normal=scene+(1-scene.a)*hud;
    return (rectA||rectB)&&!gap?masked:normal;
}
