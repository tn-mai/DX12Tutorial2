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

#undef FLOAT
#undef FLOAT2
#undef FLOAT3
#undef FLOAT3A
#undef FLOAT4X4
#undef CBUFFER
#undef REGISTER

#endif // DX12TUTORIAL_RES_TERRAINCONSTANT_H_
