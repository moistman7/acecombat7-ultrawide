struct Vertex {float4 position:SV_Position;float2 uv:TEXCOORD0;};
Vertex VS(uint id:SV_VertexID){Vertex o;o.uv=float2((id<<1)&2,id&2);o.position=float4(o.uv.x*2-1,1-o.uv.y*2,0,1);return o;}
Texture2D image:register(t3);SamplerState imageSampler:register(s0);
float4 PS(Vertex input):SV_Target {return image.Sample(imageSampler,input.uv)+float4(0,.01,.02,0);}
