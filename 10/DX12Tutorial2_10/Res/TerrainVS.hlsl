/**
* @file TerrainVS.hlsl
*/
#include "TerrainConstant.h"

struct VS_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
};

struct VS_INPUT
{
  float3 position : POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
  VS_OUTPUT output;
  output.worldPosition = input.position;
  float2 texcoord = (output.worldPosition.xz + float2(0, cbFrame.base)) * cbTerrain.reciprocalSize;
  output.worldPosition.y = HeightMapVS(texcoord);
  const float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  output.height4.x = HeightMapVS(texcoord + float2(0, offset.y));
  output.height4.y = HeightMapVS(texcoord + float2(0, -offset.y));
  output.height4.z = HeightMapVS(texcoord + float2(offset.x, 0));
  output.height4.w = HeightMapVS(texcoord + float2(-offset.x, 0));
  return output;
}