/**
* @file ProcedualTerrain.cpp
*/
#include "ProcedualTerrain.h"
#include "d3dx12.h"
#include "PSO.h"
#include "Graphics.h"
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

/**
* 地形描画用頂点データ型.
*/
struct Vertex {
  XMFLOAT3 position;
};

/**
* 手続き的地形を初期化する.
*
* @param commandAllocator コマンドリスト作成用のアロケータ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool ProcedualTerrain::Init(const ComPtr<ID3D12DescriptorHeap>& csuDescriptorHeap)
{
  ComPtr<ID3D12Device> device;
  if (FAILED(csuDescriptorHeap->GetDevice(IID_PPV_ARGS(&device)))) {
    return false;
  }

  const UINT vertexBufferSize = static_cast<UINT>(vertexCount * sizeof(Vertex));
  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&vertexBuffer)
  ))) {
    return false;
  }
  vertexBuffer->SetName(L"ProcedualTerrain Vertex Buffer");
  CD3DX12_RANGE range(0, 0);
  void* vertexBufferAddress;
  if (FAILED(vertexBuffer->Map(0, &range, &vertexBufferAddress))) {
    return false;
  }
  Vertex* pVertex = static_cast<Vertex*>(vertexBufferAddress);

  // 四角形を等分した頂点データを作成する.
  const XMVECTOR factor = { static_cast<float>(sizeX) / static_cast<float>(width - 1), 1, -static_cast<float>(sizeZ) / static_cast<float>(height - 1), 1 };
  const XMVECTOR offset = { -static_cast<float>(sizeX) * 0.5f, 0, static_cast<float>(sizeZ) * 0.5f, 0 };
  for (size_t z = 0; z < height; ++z) {
    for (size_t x = 0; x < width; ++x) {
      const XMVECTOR ipos = { static_cast<float>(x), 0, static_cast<float>(z), 1 };
      const XMVECTOR pos = ipos * factor + offset;
      XMStoreFloat3(&pVertex->position, pos);
      ++pVertex;
    }
  }
  vertexBuffer->Unmap(0, nullptr);
  vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = sizeof(Vertex);
  vertexBufferView.SizeInBytes = vertexBufferSize;

  const UINT indexListSize = static_cast<UINT>(indexCount * sizeof(uint16_t));
  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(indexListSize),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&indexBuffer)
  ))) {
    return false;
  }
  indexBuffer->SetName(L"ProcedualTerrain Index Buffer");
  void* indexBufferAddress;
  if (FAILED(indexBuffer->Map(0, &range, &indexBufferAddress))) {
    return false;
  }
  uint16_t* pIndexBuffer = static_cast<uint16_t*>(indexBufferAddress);
  for (size_t z = 0; z < height - 1; ++z) {
    for (size_t x = 0; x < width - 1; ++x) {
      pIndexBuffer[0] = static_cast<uint16_t>(x + z * width);
      pIndexBuffer[1] = static_cast<uint16_t>((x + 1) + z * width);
      pIndexBuffer[2] = static_cast<uint16_t>(x + (z + 1) * width);
      pIndexBuffer[3] = static_cast<uint16_t>((x + 1) + (z + 1) * width);
	  pIndexBuffer += 4;
    }
  }
  indexBuffer->Unmap(0, nullptr);
  indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
  indexBufferView.Format = DXGI_FORMAT_R16_UINT;
  indexBufferView.SizeInBytes = indexListSize;

  const size_t constantBufferSize = (sizeof(TerrainConstant) + 255) & ~255;
  if (FAILED(device->CreateCommittedResource(
	  &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	  D3D12_HEAP_FLAG_NONE,
	  &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
	  D3D12_RESOURCE_STATE_GENERIC_READ,
	  nullptr,
	  IID_PPV_ARGS(&constantBuffer)
  ))) {
	  return false;
  }
  indexBuffer->SetName(L"ProcedualTerrain Constant Buffer");
  void* constantBufferAddress;
  if (FAILED(constantBuffer->Map(0, &range, &constantBufferAddress))) {
	  return false;
  }
  pConstantBuffer = static_cast<TerrainConstant*>(constantBufferAddress);
  constantBufferView.BufferLocation = constantBuffer->GetGPUVirtualAddress();
  constantBufferView.SizeInBytes = constantBufferSize;
  const UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  cbGPUAddress = CD3DX12_GPU_DESCRIPTOR_HANDLE(csuDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 10, handleSize);
  const D3D12_CPU_DESCRIPTOR_HANDLE cbCPUAddress = CD3DX12_CPU_DESCRIPTOR_HANDLE(csuDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 10, handleSize);
  device->CreateConstantBufferView(&constantBufferView, cbCPUAddress);

  Graphics::Graphics& graphics = Graphics::Graphics::Get();
  Resource::ResourceLoader loader;
  loader.Begin(graphics.csuDescriptorHeap);
  if (!loader.LoadFromFile(texTerrain, 11, L"Res/Terrain.png")) {
    return false;
  }
  ID3D12CommandList* ppCommandLists[] = { loader.End() };
  graphics.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  graphics.WaitForGpu();

  offsetZ = 0;
  base = 0;
  Update(0);

  return true;
}

/**
* 更新.
*
* @param delta 経過時間.
*/
void ProcedualTerrain::Update(double delta)
{
	if (!pConstantBuffer) {
		return;
	}
    const float limitOffsetZ = static_cast<float>(sizeZ) / static_cast<float>(height - 1);
    offsetZ += 4.0f * static_cast<float>(delta);
    if (offsetZ >= limitOffsetZ) {
      const float i = std::floor(offsetZ * (1.0f / limitOffsetZ));
      offsetZ = std::fmod(offsetZ, limitOffsetZ);
      base += i * limitOffsetZ;
    }
    Graphics::Graphics& graphics = Graphics::Graphics::Get();
	const XMMATRIX matEyeRot = XMMatrixRotationX(XMConvertToRadians(75.0f));
    const XMMATRIX matEye = matEyeRot * XMMatrixTranslation(0, 0, offsetZ - limitOffsetZ);
    const XMVECTOR eyePos = XMVector4Transform(XMVECTOR{ 0, 0, -75, 1 }, matEye);
    const XMVECTOR eyeForcus = XMVector4Transform(XMVECTOR{ 0, 0, 0, 1 }, matEye);
    const XMVECTOR eyeUp = XMVector4Transform(XMVECTOR{ 0, 1, 0, 1 }, matEyeRot);
	const XMMATRIX matView = XMMatrixLookAtLH(eyePos, eyeForcus, eyeUp);
	const XMMATRIX matProjection = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), graphics.viewport.Width / graphics.viewport.Height, 1.0f, 1000.0f);
	XMStoreFloat3(&pConstantBuffer->cbFrame.eye, eyePos);
    XMStoreFloat3(&pConstantBuffer->cbFrame.lightDir, XMVector4Transform(XMVECTOR{0, -1, 0}, XMMatrixRotationRollPitchYaw(XMConvertToRadians(30), XMConvertToRadians(30), 0)));
	pConstantBuffer->cbFrame.lightDiffuse = XMFLOAT3A(1.0f, 1.0f, 0.95f);
	pConstantBuffer->cbFrame.lightAmbient = XMFLOAT3A(0.1f, 0.05f, 0.15f);
    pConstantBuffer->cbFrame.base = base;
	pConstantBuffer->cbTerrain = { XMFLOAT2(1.0f / static_cast<float>(sizeX), 1.0f / static_cast<float>(sizeZ)), 30.0f, 1.0f / 30.0f };
	XMStoreFloat4x4A(&pConstantBuffer->cbFrame.matViewProjection, XMMatrixTranspose(matView * matProjection));
}

/**
* 描画.
*
* @param commandList  描画コマンド発行先のコマンドリスト.
* @param cbTableIndex 定数バッファを割り当てるルートデスクリプタテーブルのインデックス.
*/
void ProcedualTerrain::Draw(ID3D12GraphicsCommandList* commandList, uint32_t cbTableIndex) const
{
  Graphics::Graphics& graphics = Graphics::Graphics::Get();
  const PSO& pso = GetPSO(PSOType_Terrain);
  commandList->SetGraphicsRootSignature(pso.rootSignature.Get());
  commandList->SetPipelineState(pso.pso.Get());
  commandList->RSSetViewports(1, &graphics.viewport);
  commandList->RSSetScissorRects(1, &graphics.scissorRect);
  ID3D12DescriptorHeap* heapList[] = { graphics.csuDescriptorHeap.Get() };
  commandList->SetDescriptorHeaps(_countof(heapList), heapList);
  commandList->SetGraphicsRootDescriptorTable(0, texTerrain.handle);
  commandList->SetGraphicsRootDescriptorTable(1, cbGPUAddress);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
  commandList->IASetIndexBuffer(&indexBufferView);
  commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}
