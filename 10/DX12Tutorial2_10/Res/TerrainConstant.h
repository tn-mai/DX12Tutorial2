/**
* @file TerrainConstant.h
*/
#ifndef DX12TUTORIAL_RES_TERRAINCONSTANT_H_
#define DX12TUTORIAL_RES_TERRAINCONSTANT_H_
#ifdef _WIN32
#include <DirectXMath.h>
#endif // _WIN32

#ifdef _WIN32
#define FLOAT float
#define FLOAT2 DirectX::XMFLOAT2
#define FLOAT3 DirectX::XMFLOAT3
#define FLOAT3A DirectX::XMFLOAT3A
#define FLOAT4X4 DirectX::XMFLOAT4X4A
#define CBUFFER struct
#define REGISTER(r)
#else
#define FLOAT float
#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT3A float3
#define FLOAT4X4 float4x4
#define CBUFFER cbuffer
#define REGISTER(r) : register(r)
#endif // _WIN32

/// 地形用の変化しないデータ.
struct TerrainData
{
  FLOAT2 reciprocalSize; ///< sizeX, sizeZの逆数.
  FLOAT scale;///< Y方向の拡大率.
  FLOAT reciprocalScale;///< Y方向の拡大率の逆数.
};

/// 地形用の毎フレーム変化するデータ.
struct PerFrameData
{
  FLOAT4X4 matViewProjection; ///< View/Projection行列.
  FLOAT3A eye; ///< 視点座標.
  FLOAT3A lightDir; ///< 平行光源の向き.
  FLOAT3A lightDiffuse; ///< 平行光源の明るさ.
  FLOAT3 lightAmbient; ///< 環境光の明るさ.
  float base; ///< ハイトマップの参照位置.
};

/// 地形用の定数バッファ.
CBUFFER TerrainConstant REGISTER(b0)
{
	TerrainData cbTerrain;
	PerFrameData cbFrame;
};

static const FLOAT terrainSeaHeight = FLOAT(0.5);

#undef FLOAT
#undef FLOAT2
#undef FLOAT3
#undef FLOAT3A
#undef FLOAT4X4
#undef CBUFFER
#undef REGISTER

#ifndef _WIN32

/// 座標に対応するノイズ値を取得する.
float Noise(float2 st)
{
  const float2 i = floor(st);
  const float2 f = frac(st);
  const float2 u = f * f * (3.0f - 2.0f * f);
  const float2 ratio = { 1.0f - u.x, u.x };
  const float3 scale = { 12.9898f, 78.233f, 43758.5453123f };
  const float4 tmp = (i.xyxy + float4(0, 0, 1, 1)) * scale.xyxy;
  float4 abcd = frac(sin(tmp.xzxz + tmp.yyww) * scale.z);
  abcd.zw -= abcd.xy;
  abcd.zw *= u.y;
  return dot(abcd.xyzw, ratio.xyxy);
}

/// 座標に対応する高さを取得する.
float HeightMapVS(float2 texcoord)
{
  const float freq = 3.0;
  float value = 0;
  value += Noise(texcoord * freq) * 0.5;
  value += Noise(texcoord * freq * 2.01) * 0.25;
  return value;
}

/// 座標に対応する高さを取得する.
float HeightMapDS(float2 texcoord)
{
  const float freq = 3.0;
  float value = 0;
//  value += Noise(texcoord * freq * 2.01) * 0.25;
  value += Noise(texcoord * freq * 2.01 * 2.02) * 0.125;
  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03) * 0.0625;
  return value;
}
/// 座標に対応する高さを取得する.
float HeightMapPS(float2 texcoord)
{
  const float freq = 3.0;
  float value = 0;
//  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03) * 0.0625;
  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03 * 2.01) * 0.03125 * 0.5;
//  value += Noise(texcoord * freq * 2.01 * 2.02 * 2.03 * 2.01 * 2.02 * 2.03) * 0.03125 * 0.0625;
  return value;
}

#endif // _WIN32

#endif // DX12TUTORIAL_RES_TERRAINCONSTANT_H_
