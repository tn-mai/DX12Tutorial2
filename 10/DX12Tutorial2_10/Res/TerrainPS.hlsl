/**
* @file TerrainPS.hlsl
*/
#include "TerrainConstant.h"

Texture2D texTerrain : register(t0);
SamplerState sampler0 : register(s0);

// 入力ピクセル点
struct DS_OUTPUT
{
  float4 vPosition  : SV_POSITION;
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
  float2 texcoord : TEXCOORD;
};

float3 EstimateNormal(float2 texcoord, float4 height4)
{
  float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  height4.x += HeightMapPS(texcoord + float2(0, offset.y));
  height4.y += HeightMapPS(texcoord + float2(0, -offset.y));
  height4.z += HeightMapPS(texcoord + float2(offset.x, 0));
  height4.w += HeightMapPS(texcoord + float2(-offset.x, 0));
  height4 = max(terrainSeaHeight, height4);
  height4.xw -= height4.yz;
  offset *= 2;
  const float3 vz = float3(0, height4.x, offset.y);
  const float3 vx = float3(-offset.x, height4.w, 0);
  return normalize(cross(vx, vz));
}

float4 main(DS_OUTPUT input) : SV_TARGET
{
  float3 norm = EstimateNormal(input.texcoord, input.height4);
  float3 viewvector = cbFrame.eye - input.worldPosition;
  //float3 color = float1(HeightMap(input.worldPosition.xz * (1.0 / 100.0))).xxx;
  //float3 color = norm.xyz * 0.5 + 0.5;
  float height = input.worldPosition.y * cbTerrain.reciprocalScale;
  height = (height - terrainSeaHeight) * (1.0 / (1 - terrainSeaHeight));
  float3 color = texTerrain.Sample(sampler0, float2(norm.y, height)).xyz;
  float3 diffuse = cbFrame.lightDiffuse * dot(-cbFrame.lightDir, norm.xyz);
  return float4((diffuse + cbFrame.lightAmbient) * color, 1);
}