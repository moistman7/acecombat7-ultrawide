// ---- Created with 3Dmigoto v1.3.14 on Thu Feb 21 15:57:31 2019
cbuffer cb0 : register(b0)
{
  float4 cb0[4];
}




// 3Dmigoto declarations
#define cmp -
cbuffer NativeFix : register(b13) { float4 fix; }
Texture2D<float4> StereoParams : register(t125);


void main(
  float4 v0 : ATTRIBUTE0,
  float2 v1 : ATTRIBUTE1,
  float2 v2 : ATTRIBUTE2,
  float4 v3 : ATTRIBUTE3,
  uint2 v4 : ATTRIBUTE4,
  out float4 o0 : SV_POSITION0,
  out float4 o1 : COLOR0,
  out float4 o2 : ORIGINAL_POSITION0,
  out float4 o3 : TEXCOORD0,
  out float4 o4 : TEXCOORD1)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = cb0[1].xyzw * v2.yyyy;
  r0.xyzw = v2.xxxx * cb0[0].xyzw + r0.xyzw;
  o0.xyzw = cb0[3].xyzw + r0.xyzw;
  r0.xyz = max(float3(6.10351999e-005,6.10351999e-005,6.10351999e-005), v3.xyz);
  r1.xyz = r0.xyz * float3(0.947867274,0.947867274,0.947867274) + float3(0.0521326996,0.0521326996,0.0521326996);
  r1.xyz = log2(r1.xyz);
  r1.xyz = float3(2.4000001,2.4000001,2.4000001) * r1.xyz;
  r1.xyz = exp2(r1.xyz);
  r2.xyz = cmp(float3(0.0404499993,0.0404499993,0.0404499993) < r0.xyz);
  r0.xyz = float3(0.0773993805,0.0773993805,0.0773993805) * r0.xyz;
  o1.xyz = r2.xyz ? r1.xyz : r0.xyz;

  o1.w = v3.w;
  o2.xy = v2.xy;
  o2.zw = float2(0,1);
  o3.xy = v1.xy;
  o4.xyzw = v0.xyzw;
  o0.x = o0.x * fix.x + fix.y * o0.w;
  return;
}
