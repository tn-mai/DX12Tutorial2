/**
* @file ProcedualTerrain.cpp
*/
#include "ProcedualTerrain.h"
#include "Texture.h"
#include "d3dx12.h"
#include "PSO.h"
#include "Texture.h"
#include "Graphics.h"
#include "GamePad.h"
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
bool ProcedualTerrain::Init(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12DescriptorHeap>& csuDescriptorHeap)
{
  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(vertexCount * sizeof(Vertex)),
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
  const XMVECTORF32 factor = { 100.0f / static_cast<float>(width - 1), 1, -100.0f / static_cast<float>(height - 1), 1 };
  const XMVECTORF32 offset = { -50, 0, 50, 0 };
  for (size_t z = 0; z < height; ++z) {
    for (size_t x = 0; x < width; ++x) {
      const XMVECTORF32 ipos = { static_cast<float>(x), 0, static_cast<float>(z), 1 };
      const XMVECTOR pos = ipos * factor + offset;
      XMStoreFloat3(&pVertex->position, pos);
      ++pVertex;
    }
  }
  vertexBuffer->Unmap(0, nullptr);
  vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = sizeof(Vertex);
  vertexBufferView.SizeInBytes = static_cast<UINT>(vertexCount * sizeof(Vertex));

  const int indexListSize = static_cast<int>(indexCount * sizeof(uint16_t));
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

  const size_t constantBufferSize = (sizeof(ConstantBuffer) + 255) & ~255;
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
  pConstantBuffer = static_cast<ConstantBuffer*>(constantBufferAddress);
  Update();

  constantBufferView.BufferLocation = constantBuffer->GetGPUVirtualAddress();
  constantBufferView.SizeInBytes = constantBufferSize;
  const UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  cbGPUAddress = CD3DX12_GPU_DESCRIPTOR_HANDLE(csuDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 10, handleSize);
  cbCPUAddress = CD3DX12_CPU_DESCRIPTOR_HANDLE(csuDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 10, handleSize);
  device->CreateConstantBufferView(&constantBufferView, cbCPUAddress);

  return true;
}

/**
* 更新
*/
void ProcedualTerrain::Update()
{
	if (!pConstantBuffer) {
		return;
	}
#if 0
	const GamePad& gamepad = GetGamePad(0);
	if (gamepad.buttons & GamePad::DPAD_LEFT) {
		rotEye.y -= 2.0f / 180.0f * 3.1415926f;
	}
	if (gamepad.buttons & GamePad::DPAD_RIGHT) {
		rotEye.y += 2.0f / 180.0f * 3.1415926f;
	}
	if (gamepad.buttons & GamePad::DPAD_UP) {
		rotEye.x -= 2.0f / 180.0f * 3.1415926f;
	}
	if (gamepad.buttons & GamePad::DPAD_DOWN) {
		rotEye.x += 2.0f / 180.0f * 3.1415926f;
	}
#endif
	Graphics::Graphics& graphics = Graphics::Graphics::Get();
	ConstantBuffer& constant = *pConstantBuffer;
    rotEye = { 3.14159265f / 3.0f, 0 };
	const XMMATRIX matEye = XMMatrixRotationX(rotEye.x) * XMMatrixRotationY(rotEye.y);
	const XMVECTOR eyePos = XMVector4Transform(XMVECTOR{ 0, 0, -70, 1 }, matEye);
	const XMVECTOR eyeUp = XMVector4Transform(XMVECTOR{ 0, 1, 0, 1 }, matEye);
	const XMMATRIX matView = XMMatrixLookAtLH(eyePos, XMVECTOR{ 0, 0, 0, 1 }, eyeUp);
	const XMMATRIX matProjection = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), graphics.viewport.Width / graphics.viewport.Height, 1.0f, 1000.0f);
	XMStoreFloat3(&constant.cbFrame.eye, eyePos);
	XMStoreFloat3(&constant.cbFrame.lightDir, XMVECTOR{1.0f / 1.41421356f, -1.0f / 1.41421356f, 0, 1});
	constant.cbFrame.lightDiffuse = XMFLOAT3A(0.8f, 0.8f, 0.7f);
	constant.cbFrame.lightSpecular = XMFLOAT3A(0.8f, 0.8f, 0.7f);
	constant.cbFrame.lightAmbient = XMFLOAT3A(0.1f, 0.05f, 0.1f);
    static float base = 0;
    base += 0.005f;
	constant.cbTerrain = { 25, 100, 100, base };
	XMStoreFloat4x4A(&constant.cbFrame.matViewProjection, XMMatrixTranspose(matView * matProjection));
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
  ID3D12DescriptorHeap* heapList[] = { graphics.csuDescriptorHeap.Get() };
  commandList->SetDescriptorHeaps(_countof(heapList), heapList);
  commandList->RSSetViewports(1, &graphics.viewport);
  commandList->RSSetScissorRects(1, &graphics.scissorRect);

  commandList->SetGraphicsRootDescriptorTable(cbTableIndex, cbGPUAddress);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
  commandList->IASetIndexBuffer(&indexBufferView);
  commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}
