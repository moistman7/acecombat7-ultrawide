// Same fit-and-extend as MenuEdges.hlsl, for the Slate composite that draws a
// 1920x1080 UI layer into the left 16:9 area of the screen (briefing cards,
// mission title card) and for the pillarboxed briefing movie plate. The layer is
// recentered and its edge pixels extended.
// Register order matches the Slate vertex shader output signature. The Slate pixel
// shader samples at extra.xy*extra.zw, so that is the coordinate adjusted here.
cbuffer MenuLayout : register(b0) { float4 menuLayout; }
struct Vertex {
    float4 position:SV_Position;
    float4 color:COLOR0;
    float4 original:ORIGINAL_POSITION;
    float2 uv:TEXCOORD0;
    float4 extra:TEXCOORD1;
};
[maxvertexcount(3)]
void main(triangle Vertex input[3],inout TriangleStream<Vertex> output) {
    float3 xs=float3(input[0].position.x/input[0].position.w,input[1].position.x/input[1].position.w,input[2].position.x/input[2].position.w);
    float3 ys=float3(input[0].position.y/input[0].position.w,input[1].position.y/input[1].position.w,input[2].position.y/input[2].position.w);
    float left=min(xs.x,min(xs.y,xs.z)),right=max(xs.x,max(xs.y,xs.z));
    float top=max(ys.x,max(ys.y,ys.z)),bottom=min(ys.x,min(ys.y,ys.z));
    float3 us=float3(input[0].extra.x*input[0].extra.z,input[1].extra.x*input[1].extra.z,input[2].extra.x*input[2].extra.z);
    float3 ts=float3(input[0].uv.x,input[1].uv.x,input[2].uv.x);
    float minU=min(us.x,min(us.y,us.z)),maxU=max(us.x,max(us.y,us.z));
    float minT=min(ts.x,min(ts.y,ts.z)),maxT=max(ts.x,max(ts.y,ts.z));
    float span=right-left,height=top-bottom;
    bool expand=span>.4&&span<1.98&&height>1.6&&height<2.02&&maxU>minU;
    float scale=expand?2/height:1,centerX=(left+right)*.5,centerY=(top+bottom)*.5;
    float scaledLeft=(left-centerX)*scale,scaledSpan=span*scale;
    [unroll]for(uint i=0;i<3;++i){
        Vertex v=input[i];
        if(expand){
            float x=(v.position.x/v.position.w-centerX)*scale,y=(v.position.y/v.position.w-centerY)*scale;
            float newX=menuLayout.z>1.5?x:(x-scaledLeft)*2/scaledSpan-1,delta=(newX-x)/scaledSpan;
            if(v.extra.z!=0)v.extra.x+=delta*(maxU-minU)/v.extra.z;
            v.uv.x+=delta*(maxT-minT);
            v.position.x=newX*v.position.w;v.position.y=y*v.position.w;
        }
        output.Append(v);
    }
    output.RestartStrip();
}
