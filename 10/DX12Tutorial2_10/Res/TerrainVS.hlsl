/**
* @file TerrainVS.hlsl
*/

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

struct VS_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
};

struct VS_INPUT
{
  float3 position : POSITION;
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
  static const float freq = 3.0;
  float value = 0;
  value += Noise(texcoord * freq) * 0.5;
  value += Noise(texcoord * freq * 2.01) * 0.25;
  //value += Noise(texcoord * freq * 2.01 * 2.02) * 0.125;
  //value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03) * 0.0625;
  //value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03 * 2.01) * 0.03125;
  return value;
}

VS_OUTPUT main(VS_INPUT input)
{
  VS_OUTPUT output;
  output.worldPosition = input.position;
  float2 texcoord = (output.worldPosition.xz + float2(0, cbTerrain.base)) * cbTerrain.reciprocalSize;
  output.worldPosition.y = HeightMap(texcoord);
  const float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  output.height4.x = HeightMap(texcoord + float2(0, offset.y));
  output.height4.y = HeightMap(texcoord + float2(0, -offset.y));
  output.height4.z = HeightMap(texcoord + float2(offset.x, 0));
  output.height4.w = HeightMap(texcoord + float2(-offset.x, 0));
  return output;
}