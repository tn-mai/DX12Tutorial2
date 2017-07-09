/**
* @file TerrainPS.hlsl
*/
#define TERRAIN_NOISE_BEGIN 4
#define TERRAIN_NOISE_END 5
#include "TerrainConstant.h"

Texture2D texTerrain : register(t0);
SamplerState sampler0 : register(s0);

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
  float3 norm = EstimateNormal(input.texcoord, input.height4);
  float3 viewvector = cbFrame.eye - input.worldPosition;
  //float3 color = float1(HeightMap(input.worldPosition.xz * (1.0 / 100.0))).xxx;
  //float3 color = float3(0.8, 0.8, 0.8);
  float3 color = texTerrain.Sample(sampler0, float2(acos(norm.y) * (1.0 / 3.14159265), input.worldPosition.y * cbTerrain.reciprocalScale)).xyz;
  float3 diffuse = cbFrame.lightDiffuse * dot(-cbFrame.lightDir, norm);
  return float4((diffuse + cbFrame.lightAmbient) * color, 1);
}