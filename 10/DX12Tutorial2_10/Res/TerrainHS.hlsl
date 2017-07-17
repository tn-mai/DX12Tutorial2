/**
* @file TerrainHS.hlsl
*/
#include "TerrainConstant.h"

// 入力制御点
struct VS_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
};

// 出力制御点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
};

// 出力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[4] : SV_TessFactor;
  float InsideTessFactor[2] : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 4

float CalcTessFactor(float3 p)
{
  float d = distance(p, cbFrame.eye);
  float s = saturate((d - 16.0f) * (1.0f / (256.0f - 16.0f)));
  return pow(2, (lerp(6, 0, s)));
}

// パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONSTANT_DATA_OUTPUT output;
#if 0
  float4 isLand = step(terrainSeaHeight * 0.5, float4(ip[0].worldPosition.y, ip[1].worldPosition.y, ip[2].worldPosition.y, ip[3].worldPosition.y));
  float factor = 1.0 + any(isLand) * 3.0;
  output.EdgeTessFactor[0] =
  output.EdgeTessFactor[1] =
  output.EdgeTessFactor[2] =
  output.EdgeTessFactor[3] =
  output.InsideTessFactor[0] = output.InsideTessFactor[1] = factor;
#elif 1
  const float4 h = { ip[0].worldPosition.y, ip[1].worldPosition.y, ip[2].worldPosition.y, ip[3].worldPosition.y };
  const float4 edge = min(4, max(0, h.xxyz + h.zyww - terrainSeaHeight) * 0.5 * (1.0 / (1 - terrainSeaHeight)) * 12 + 1);
  const float2 center = (edge.xy + edge.zw) * 0.5;
  output.EdgeTessFactor[0] = edge.x;
  output.EdgeTessFactor[1] = edge.y;
  output.EdgeTessFactor[2] = edge.z;
  output.EdgeTessFactor[3] = edge.w;
  output.InsideTessFactor[0] = center.x;
  output.InsideTessFactor[1] = center.y;
#else
  output.EdgeTessFactor[0] = 4.0f;
  output.EdgeTessFactor[1] = 4.0f;
  output.EdgeTessFactor[2] = 4.0f;
  output.EdgeTessFactor[3] = 4.0f;
  output.InsideTessFactor[0] = 4.0f;
  output.InsideTessFactor[1] = 4.0f;
#endif
  return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONTROL_POINT_OUTPUT output;
  output.worldPosition = ip[i].worldPosition;
  output.height4 = ip[i].height4;
  return output;
}
