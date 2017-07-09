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
#else
#define FLOAT float
#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT3A float3
#define FLOAT4X4 float4x4
#endif // _WIN32

struct TerrainData
{
  FLOAT2 reciprocalSize; ///< sizeX, sizeZの逆数.
  FLOAT scale;///< Y方向の拡大率.
  FLOAT reciprocalScale;///< Y方向の拡大率の逆数.
};

struct PerFrameData
{
  FLOAT4X4 matViewProjection; ///< View/Projection行列.
  FLOAT3A eye; ///< 視点座標.
  FLOAT3A lightDir; ///< 平行光源の向き.
  FLOAT3A lightDiffuse; ///< 平行光源の明るさ.
  FLOAT3 lightAmbient; ///< 環境光の明るさ.
  float base; ///< ハイトマップの参照位置.
};

#undef FLOAT
#undef FLOAT2
#undef FLOAT3
#undef FLOAT3A
#undef FLOAT4X4

#endif // DX12TUTORIAL_RES_TERRAINCONSTANT_H_
