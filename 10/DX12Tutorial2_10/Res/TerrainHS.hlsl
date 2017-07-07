/**
* @file TerrainHS.hlsl
*/

struct TerrainData
{
  float scale;
  float width;
  float depth;
  float base;
};

struct PerFrameData
{
  float4x4 matViewProjection;
  float3 eye;
  float3 lightDir;
  float3 lightDiffuse;
  float3 lightSpecular;
  float3 lightAmbient;
};

cbuffer Constant : register(b0)
{
  TerrainData cbTerrain;
  PerFrameData cbFrame;
}

// 入力制御点
struct VS_OUTPUT
{
  float3 worldPosition : WORLDPOS;
  uint skirt : SKIRT;
};

// 出力制御点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 worldPosition : WORLDPOS;
};

// 出力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[4] : SV_TessFactor;
  float InsideTessFactor[2] : SV_InsideTessFactor;
  uint skirt : SKIRT;
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
  output.skirt = ip[0].skirt;
#if 0
  if (output.skirt == 0) {
    output.EdgeTessFactor[0] = 1.0f;
    output.EdgeTessFactor[1] = 1.0f;
    output.EdgeTessFactor[2] = 1.0f;
    output.EdgeTessFactor[3] = 1.0f;
    output.InsideTessFactor[0] = 1.0f;
    output.InsideTessFactor[1] = 1.0f;
  } else if (output.skirt < 5) {
    const float3 e3 = 0.5f * (ip[2].worldPosition + ip[3].worldPosition);
    output.EdgeTessFactor[0] = 1.0f;
    output.EdgeTessFactor[1] = 1.0f;
    output.EdgeTessFactor[2] = 1.0f;
    output.EdgeTessFactor[3] = CalcTessFactor(e3);
    output.InsideTessFactor[0] = 1.0f;
    output.InsideTessFactor[1] = 1.0f;
  } else {
    const float3 e0 = 0.5f * (ip[0].worldPosition + ip[2].worldPosition);
    const float3 e1 = 0.5f * (ip[0].worldPosition + ip[1].worldPosition);
    const float3 e2 = 0.5f * (ip[1].worldPosition + ip[3].worldPosition);
    const float3 e3 = 0.5f * (ip[2].worldPosition + ip[3].worldPosition);
    const float3 center = 0.5f * (e1 + e3);
    output.EdgeTessFactor[0] = CalcTessFactor(e0);
    output.EdgeTessFactor[1] = CalcTessFactor(e1);
    output.EdgeTessFactor[2] = CalcTessFactor(e2);
    output.EdgeTessFactor[3] = CalcTessFactor(e3);
    output.InsideTessFactor[0] = output.InsideTessFactor[1] = CalcTessFactor(center);
  }
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
[partitioning("fractional_even")]
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
  return output;
}
