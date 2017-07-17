/**
* @file TerrainDS.hlsl
*/
#include "TerrainConstant.h"

// 入力制御点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  float4 height4: HEIGHT;
};

// 入力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[4] : SV_TessFactor;
  float InsideTessFactor[2] : SV_InsideTessFactor;
};

// 出力制御点
struct DS_OUTPUT
{
  float4 vPosition  : SV_POSITION;
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
  float2 texcoord : TEXCOORD;
};

#define NUM_CONTROL_POINTS 4

[domain("quad")]
DS_OUTPUT main(
  HS_CONSTANT_DATA_OUTPUT input,
  float2 domain : SV_DomainLocation,
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  DS_OUTPUT output;
  output.worldPosition = lerp(
    lerp(patch[0].worldPosition, patch[1].worldPosition, domain.x),
    lerp(patch[2].worldPosition, patch[3].worldPosition, domain.x),
    domain.y);
  output.texcoord = (output.worldPosition.xz + float2(0, cbFrame.base)) * cbTerrain.reciprocalSize;
  const float h = HeightMapDS(output.texcoord);
  output.worldPosition.y = max(terrainSeaHeight, (output.worldPosition.y + h)) * cbTerrain.scale;
  output.vPosition = mul(float4(output.worldPosition, 1), cbFrame.matViewProjection);

  output.height4 = lerp(
    lerp(patch[0].height4, patch[1].height4, domain.x),
    lerp(patch[2].height4, patch[3].height4, domain.x),
    domain.y);
  const float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  output.height4.x += HeightMapDS(output.texcoord + float2(0, offset.y));
  output.height4.y += HeightMapDS(output.texcoord + float2(0, -offset.y));
  output.height4.z += HeightMapDS(output.texcoord + float2(offset.x, 0));
  output.height4.w += HeightMapDS(output.texcoord + float2(-offset.x, 0));
  return output;
}
