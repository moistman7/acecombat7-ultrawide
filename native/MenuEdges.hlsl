// Fill the unused sides around the game's 16:9 UI plane. Register order matches the
// game's widget material; the sampler is replaced with a U-clamped copy.
// Normal (flat plane): fit to the full screen height without distortion and extend
// the edge columns to the screen edges.
// Span (menus over a 3D scene, e.g. the hangar): split the plane at its horizontal
// center, move the left half to the left screen edge and the right half to the right
// edge, and fill the gap with the center column. Keep the footer together: tilted
// hangar planes reserve 25% for descriptions; flat briefing planes reserve only
// the bottom 10% for controls, leaving their lower-right minimap in the side group.
// The quad is rebuilt from its first triangle in texture space (attributes and clip-space
// positions are affine there, so this is exact under perspective); the second triangle is
// dropped. Emitted triangles keep the source winding because the game culls back faces.
cbuffer MenuLayout : register(b0) { float4 menuLayout; } // x: span, y: tilted shift, z: fit only (preserve side bars)
struct Vertex {
    float4 tex10:TEXCOORD10;
    float4 tex11:TEXCOORD11;
    float4 uv:TEXCOORD0;
    float4 position:SV_Position;
};
static Vertex corner[3];static float2 t0,t1,t2;static float tDet;
static float fitScale,fitX,fitY;static bool sourceFront;
Vertex At(float u,float v) {
    float2 d=float2(u,v)-t0,a=t1-t0,b=t2-t0;
    float l1=(d.x*b.y-d.y*b.x)/tDet,l2=(a.x*d.y-a.y*d.x)/tDet,l0=1-l1-l2;
    Vertex o;
    o.tex10=corner[0].tex10*l0+corner[1].tex10*l1+corner[2].tex10*l2;
    o.tex11=corner[0].tex11*l0+corner[1].tex11*l1+corner[2].tex11*l2;
    o.uv=corner[0].uv*l0+corner[1].uv*l1+corner[2].uv*l2;
    o.position=corner[0].position*l0+corner[1].position*l1+corner[2].position*l2;
    return o;
}
// Place a vertex: fitted screen position plus a horizontal shift, or a fixed screen x.
Vertex Place(Vertex o,float shift,float fixedX,bool useFixed) {
    float x=(o.position.x/o.position.w-fitX)*fitScale,y=(o.position.y/o.position.w-fitY)*fitScale;
    o.position.x=(useFixed?fixedX:x+shift)*o.position.w;o.position.y=y*o.position.w;
    return o;
}
float Cross(Vertex a,Vertex b,Vertex c) {
    float2 p=a.position.xy/a.position.w,q=b.position.xy/b.position.w,r=c.position.xy/c.position.w;
    return (q.x-p.x)*(r.y-p.y)-(q.y-p.y)*(r.x-p.x);
}
void Quad(Vertex a,Vertex b,Vertex c,Vertex d,inout TriangleStream<Vertex> output) {
    // a b on one row, c d on the other; swap columns if the winding does not match the source.
    if((Cross(a,b,c)>0)!=sourceFront){Vertex s=a;a=b;b=s;s=c;c=d;d=s;}
    output.Append(a);output.Append(b);output.Append(c);output.Append(d);output.RestartStrip();
}
[maxvertexcount(24)]
void main(triangle Vertex input[3],uint id:SV_PrimitiveID,inout TriangleStream<Vertex> output) {
    float3 xs=float3(input[0].position.x/input[0].position.w,input[1].position.x/input[1].position.w,input[2].position.x/input[2].position.w);
    float3 ys=float3(input[0].position.y/input[0].position.w,input[1].position.y/input[1].position.w,input[2].position.y/input[2].position.w);
    float left=min(xs.x,min(xs.y,xs.z)),right=max(xs.x,max(xs.y,xs.z));
    float top=max(ys.x,max(ys.y,ys.z)),bottom=min(ys.x,min(ys.y,ys.z));
    float minU=min(input[0].uv.x,min(input[1].uv.x,input[2].uv.x)),maxU=max(input[0].uv.x,max(input[1].uv.x,input[2].uv.x));
    float minV=min(input[0].uv.y,min(input[1].uv.y,input[2].uv.y)),maxV=max(input[0].uv.y,max(input[1].uv.y,input[2].uv.y));
    float span=right-left,height=top-bottom;
    t0=input[0].uv.xy;t1=input[1].uv.xy;t2=input[2].uv.xy;tDet=(t1.x-t0.x)*(t2.y-t0.y)-(t1.y-t0.y)*(t2.x-t0.x);
    float w0=input[0].position.w;
    bool flat=abs(input[1].position.w-w0)<1e-4*abs(w0)&&abs(input[2].position.w-w0)<1e-4*abs(w0);
    bool spread=menuLayout.x>.5;
    // The full UI plane only: a large quad that does not already cover the screen width.
    bool plane=maxU>minU&&maxV>minV&&abs(tDet)>1e-6&&span>.4&&height>1.2;
    // Hidden and sliding menu planes share this material. Fitting an offscreen
    // plane recenters it over the active menu and reveals inactive submenu text.
    bool centered=abs(left+right)<.02&&abs(top+bottom)<.02;
    // Captured hangar planes have clip w about 90.037 (menu planes use
    // 48/64/480/640). Keep their side extension active while scale animates
    // through the ordinary fit boundary, instead of switching at height 2.02.
    bool hangar=menuLayout.z>1.5&&flat&&centered&&w0>88&&w0<92&&span<1.98&&height>1.2&&height<2.2;
    bool fit=!hangar&&flat&&centered&&span<1.98&&height>1.6&&height<2.02;
    // Captured hangar UI is flat, with NDC height 2.0816. Preserve that
    // overscan and its text size; extend its sides without fitting to height.
    bool overscan=hangar||(flat&&centered&&height>=2.02&&height<2.2&&span<1.98);
    bool panel=menuLayout.w>1.5&&((!flat&&abs(left+right)<.2)||overscan);
    if(!plane||!(fit||spread||panel)){[unroll]for(uint i=0;i<3;++i)output.Append(input[i]);output.RestartStrip();return;}
    if(id&1)return; // The first triangle already covered the whole quad.
    corner[0]=input[0];corner[1]=input[1];corner[2]=input[2];
    sourceFront=Cross(input[0],input[1],input[2])>0;
    fitScale=fit?2/height:1;fitX=fit?(left+right)*.5:0;fitY=fit?(top+bottom)*.5:0;
    float s=fit?span*fitScale*.5:0;
    // Which texture column sits on the left of the screen.
    Vertex a=At(minU,minV),b=At(maxU,minV);bool uLeft=a.position.x/a.position.w<b.position.x/b.position.w;
    float leftU=uLeft?minU:maxU,rightU=uLeft?maxU:minU;
    // Captured 87d48047 is the background artwork alone. Cover the viewport
    // uniformly instead of dragging its outermost texel across either side.
    // The text-bearing 96e1f6f0 layer never enters this branch.
    if(menuLayout.w>.5&&menuLayout.w<1.5&&fit){
        fitScale/=s;
        Quad(Place(At(leftU,minV),0,0,false),Place(At(rightU,minV),0,0,false),Place(At(leftU,maxV),0,0,false),Place(At(rightU,maxV),0,0,false),output);
        return;
    }
    if(menuLayout.z>.5&&(menuLayout.z<1.5||fit)){
        Quad(Place(At(leftU,minV),0,0,false),Place(At(rightU,minV),0,0,false),Place(At(leftU,maxV),0,0,false),Place(At(rightU,maxV),0,0,false),output);
        return;
    }
    if(spread){
        float d=fit?1-s:menuLayout.y,midU=(minU+maxU)*.5;
        Vertex v0=At(midU,minV),v1=At(midU,maxV);
        bool vTop=v0.position.y/v0.position.w>v1.position.y/v1.position.w;
        float topV=vTop?minV:maxV,bottomV=vTop?maxV:minV,cutV=lerp(topV,bottomV,flat?.90:.75);
        Vertex lt=At(leftU,topV),lb=At(leftU,cutV),mt=At(midU,topV),mb=At(midU,cutV),rt=At(rightU,topV),rb=At(rightU,cutV);
        Quad(Place(lt,-d,0,false),Place(mt,-d,0,false),Place(lb,-d,0,false),Place(mb,-d,0,false),output);
        Quad(Place(mt,-d,0,false),Place(mt,d,0,false),Place(mb,-d,0,false),Place(mb,d,0,false),output);
        Quad(Place(mt,d,0,false),Place(rt,d,0,false),Place(mb,d,0,false),Place(rb,d,0,false),output);
        // Preserve the original continuous texture mapping for full-width footer text.
        lt=At(leftU,cutV);lb=At(leftU,bottomV);rt=At(rightU,cutV);rb=At(rightU,bottomV);
        Quad(Place(lt,0,-1,true),Place(lt,0,0,false),Place(lb,0,-1,true),Place(lb,0,0,false),output);
        Quad(Place(lt,0,0,false),Place(rt,0,0,false),Place(lb,0,0,false),Place(rb,0,0,false),output);
        Quad(Place(rt,0,0,false),Place(rt,0,1,true),Place(rb,0,0,false),Place(rb,0,1,true),output);
    } else {
        Vertex lt=At(leftU,minV),lb=At(leftU,maxV),rt=At(rightU,minV),rb=At(rightU,maxV);
        Quad(Place(lt,0,-1,true),Place(lt,0,0,false),Place(lb,0,-1,true),Place(lb,0,0,false),output);
        Quad(Place(lt,0,0,false),Place(rt,0,0,false),Place(lb,0,0,false),Place(rb,0,0,false),output);
        Quad(Place(rt,0,0,false),Place(rt,0,1,true),Place(rb,0,0,false),Place(rb,0,1,true),output);
    }
}
