/**
* @file ProcedualTerrain.h
*/
#ifndef DX12TUTORIAL_PROCEDUALTERRAIN_H_
#define DX12TUTORIAL_PROCEDUALTERRAIN_H_
#include "Texture.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <stdint.h>
#include <DirectXMath.h>

/**
*
*/
class ProcedualTerrain
{
public:
  static const size_t sizeX = 100;
  static const size_t sizeY = 100;
  static const size_t width = 8;
  static const size_t height = 8;
  static const size_t vertexCount = width * height;
  static const size_t indexCount = (width - 1) * (height - 1) * 4;

  ProcedualTerrain() = default;
  ~ProcedualTerrain() = default;
  ProcedualTerrain(const ProcedualTerrain&) = default;
  ProcedualTerrain& operator=(const ProcedualTerrain&) = default;

  bool Init(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& csuDescriptorHeap);
  void Update(double delta);
  void Draw(ID3D12GraphicsCommandList* commandList, uint32_t cbTableIndex) const;

private:
  /**
  * 定数バッファ.
  */
  struct ConstantBuffer
  {
    struct TerrainData
    {
      DirectX::XMFLOAT2 reciprocalSize;
      float scale;
      float reciprocalScale;
      float base;
    };

    struct PerFrameData
    {
      DirectX::XMFLOAT4X4A matViewProjection;
      DirectX::XMFLOAT3A eye;
      DirectX::XMFLOAT3A lightDir;
      DirectX::XMFLOAT3A lightDiffuse;
      DirectX::XMFLOAT3A lightAmbient;
    };

    TerrainData cbTerrain;
    PerFrameData cbFrame;
  };

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
  D3D12_INDEX_BUFFER_VIEW indexBufferView;
  Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferView;
  D3D12_CPU_DESCRIPTOR_HANDLE cbCPUAddress;
  D3D12_GPU_DESCRIPTOR_HANDLE cbGPUAddress;
  ConstantBuffer* pConstantBuffer = nullptr;
  Resource::Texture texTerrain;
  DirectX::XMFLOAT2 rotEye;
  float offsetZ;
  float base;
};

#endif // DX12TUTORIAL_PROCEDUALTERRAIN_H_