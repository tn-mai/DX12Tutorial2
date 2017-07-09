/**
* @file TerrainDS.hlsl
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
  //value += Noise(texcoord * freq) * 0.5;
  //value += Noise(texcoord * freq * 2.01) * 0.25;
  value += Noise(texcoord * freq * 2.01 * 2.02) * 0.125;
  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03) * 0.0625;
  //value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03 * 2.01) * 0.03125;
  return value;
}

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
  output.texcoord = (output.worldPosition.xz + float2(0, cbTerrain.base)) * cbTerrain.reciprocalSize;
  const float h = HeightMap(output.texcoord);
  output.worldPosition.y = max(0, 1 - (output.worldPosition.y + h) * 2) * cbTerrain.scale;
  output.vPosition = mul(float4(output.worldPosition, 1), cbFrame.matViewProjection);

  output.height4 = lerp(
    lerp(patch[0].height4, patch[1].height4, domain.x),
    lerp(patch[2].height4, patch[3].height4, domain.x),
    domain.y);
  const float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  output.height4.x += HeightMap(output.texcoord + float2(0, offset.y));
  output.height4.y += HeightMap(output.texcoord + float2(0, -offset.y));
  output.height4.z += HeightMap(output.texcoord + float2(offset.x, 0));
  output.height4.w += HeightMap(output.texcoord + float2(-offset.x, 0));
  return output;
}
