/**
* @file TerrainPS.hlsl
*/

Texture2D texTerrain : register(t0);
SamplerState sampler0 : register(s0);

struct TerrainData
{
  float2 reciprocalSize;
  float scale;
  float reciprocalScale;
  float base;
};

struct PerFrameData
{
  float4x4 matViewProjection;
  float3 eye;
  float3 lightDir;
  float3 lightDiffuse;
  float3 lightAmbient;
};

cbuffer Constant : register(b0)
{
  TerrainData cbTerrain;
  PerFrameData cbFrame;
}

struct DS_OUTPUT
{
  float4 vPosition  : SV_POSITION;
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
  float2 texcoord : TEXCOORD;
};

float4 Random4(float2 st)
{
  const float3 scale = { 12.9898f, 78.233f, 43758.5453123f };
  const float4 tmp = (st.xyxy + float4(0, 0, 1, 1)) * scale.xyxy;
  return frac(sin(tmp.xzxz + tmp.yyww) * scale.z);
}

float Noise(float2 st)
{
  const float2 i = floor(st);
  const float2 f = frac(st);
  const float2 u = f * f * (3.0f - 2.0f * f);
  const float2 ratio = { 1.0f - u.x, u.x };
  float4 abcd = Random4(i);
  abcd.zw -= abcd.xy;
  abcd.zw *= u.y;
  return dot(abcd.xyzw, ratio.xyxy);
}

float HeightMap(float2 texcoord)
{
  const float freq = 3.0;
  float value = 0;
  //value += Noise(texcoord * freq) * 0.5;
  //value += Noise(texcoord * freq * 2.01) * 0.25;
  //value += Noise(texcoord * freq * 2.01 * 2.02) * 0.125;
  //value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03) * 0.0625;
  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03 * 2.01) * 0.03125;
  return value;
}

float3 EstimateNormal(float2 texcoord, float4 height4)
{
  float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  height4.x += HeightMap(texcoord + float2(0, offset.y));
  height4.y += HeightMap(texcoord + float2(0, -offset.y));
  height4.z += HeightMap(texcoord + float2(offset.x, 0));
  height4.w += HeightMap(texcoord + float2(-offset.x, 0));
  height4 = max(0, height4 * 2 - 1) * cbTerrain.scale;
  height4.xw -= height4.yz;
  offset *= cbTerrain.scale * 2;
  const float3 vz = float3(0, height4.x, offset.y);
  const float3 vx = float3(-offset.x, height4.w, 0);
  return normalize(cross(vx, vz));
}

float4 main(DS_OUTPUT input) : SV_TARGET
{
  float3 norm = input.worldPosition.y == 0 ? float3(0, 1, 0) : EstimateNormal(input.texcoord, input.height4);
  float3 viewvector = cbFrame.eye - input.worldPosition;
  //float3 color = float1(HeightMap(input.worldPosition.xz * (1.0 / 100.0))).xxx;
  //float3 color = float3(0.8, 0.8, 0.8);
  float3 color = texTerrain.Sample(sampler0, float2(input.worldPosition.y * cbTerrain.reciprocalScale, acos(norm.y) * (1.0 / 3.14159265))).xyz;
  float3 diffuse = cbFrame.lightDiffuse * dot(-cbFrame.lightDir, norm);
  return float4((diffuse + cbFrame.lightAmbient) * color, 1);
}