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
  FLOAT2 reciprocalSize; ///< sizeX, sizeZ�̋t��.
  FLOAT scale;///< Y�����̊g�嗦.
  FLOAT reciprocalScale;///< Y�����̊g�嗦�̋t��.
};

struct PerFrameData
{
  FLOAT4X4 matViewProjection; ///< View/Projection�s��.
  FLOAT3A eye; ///< ���_���W.
  FLOAT3A lightDir; ///< ���s�����̌���.
  FLOAT3A lightDiffuse; ///< ���s�����̖��邳.
  FLOAT3 lightAmbient; ///< �����̖��邳.
  float base; ///< �n�C�g�}�b�v�̎Q�ƈʒu.
};

#undef FLOAT
#undef FLOAT2
#undef FLOAT3
#undef FLOAT3A
#undef FLOAT4X4

#endif // DX12TUTORIAL_RES_TERRAINCONSTANT_H_
