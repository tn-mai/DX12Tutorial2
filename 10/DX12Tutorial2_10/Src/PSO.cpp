/**
* @file PSO.cpp
*/
#include "PSO.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <vector>
#include <Windows.h>

using Microsoft::WRL::ComPtr;

namespace /* unnamed */ {

std::vector<PSO> psoList;

/// 頂点データ型のレイアウト.
const D3D12_INPUT_ELEMENT_DESC vertexLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/// 地形用の頂点データ型のレイアウト.
const D3D12_INPUT_ELEMENT_DESC terrainVertexLayout[] = {
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

bool LoadShader(const wchar_t* filename, const char* target, ID3DBlob** blob);
bool CreatePSO(PSO& pso, ID3D12Device* device, bool warp, const wchar_t* vs, const wchar_t* ps, const wchar_t* hs, const wchar_t* ds, const D3D12_BLEND_DESC& blendDesc);

/**
* シェーダを読み込む.
*
* @param filename シェーダファイル名.
* @param target   対象とするシェーダバージョン.
* @param blob     読み込んだシェーダを格納するBlobインターフェイスポインタのアドレス.
*
* @retval true 読み込み成功.
* @retval false 読み込み失敗.
*/
bool LoadShader(const wchar_t* filename, const char* target, ID3DBlob** blob)
{
	ComPtr<ID3DBlob> errorBuffer;
	if (FAILED(D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", target, true ? D3DCOMPILE_OPTIMIZATION_LEVEL3 : D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, blob, &errorBuffer))) {
		if (errorBuffer) {
			OutputDebugStringA(static_cast<char*>(errorBuffer->GetBufferPointer()));
		}
		return false;
	}
	return true;
}

/**
* ルートシグネチャとPSOを作成する.
*
* @param pso 作成するPSOオブジェクト.
* @param vs  作成するPSOに設定する頂点シェーダファイル名.
* @param ps  作成するPSOに設定するピクセルシェーダファイル名.
*
* @retval true  作成成功.
* @retval false 作成失敗.
*/
bool CreatePSO(PSO& pso, ID3D12Device* device, bool warp, const wchar_t* vs, const wchar_t* ps, const wchar_t* hs, const wchar_t* ds, const D3D12_BLEND_DESC& blendDesc)
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	if (!LoadShader(vs, "vs_5_0", &vertexShaderBlob)) {
		return false;
	}
	ComPtr<ID3DBlob> pixelShaderBlob;
	if (!LoadShader(ps, "ps_5_0", &pixelShaderBlob)) {
		return false;
	}
    ComPtr<ID3DBlob> hullShaderBlob;
    if (hs && !LoadShader(hs, "hs_5_0", &hullShaderBlob)) {
      return false;
    }
    ComPtr<ID3DBlob> domainShaderBlob;
    if (ds && !LoadShader(ds, "ds_5_0", &domainShaderBlob)) {
      return false;
    }

	// ルートシグネチャを作成.
	// ルートパラメータのShaderVisibilityは適切に設定する必要がある.
	// ルートシグネチャが正しく設定されていない場合でも、シグネチャの作成には成功することがある.
	// しかしその場合、PSO作成時にエラーが発生する.
	{
		D3D12_DESCRIPTOR_RANGE descRange0[] = { CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0) };
		D3D12_DESCRIPTOR_RANGE descRange1[] = { CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0) };
		D3D12_STATIC_SAMPLER_DESC staticSampler[] = { CD3DX12_STATIC_SAMPLER_DESC(0) };
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		D3D12_ROOT_SIGNATURE_DESC rsDesc;
		if (hs && ds) {
			rootParameters[0].InitAsDescriptorTable(_countof(descRange0), descRange0);
			rootParameters[1].InitAsDescriptorTable(_countof(descRange1), descRange1);
			staticSampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			rsDesc = { 2, rootParameters, _countof(staticSampler), staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
		} else {
			rootParameters[0].InitAsDescriptorTable(_countof(descRange0), descRange0);
			rootParameters[1].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
			rootParameters[2].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
			rsDesc = { 3, rootParameters, _countof(staticSampler), staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
		}
		ComPtr<ID3DBlob> signatureBlob;
        HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, nullptr);
        if (FAILED(hr)) {
			return false;
		}
		if (FAILED(device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&pso.rootSignature)))) {
			return false;
		}
	}

	// パイプラインステートオブジェクト(PSO)を作成.
	// PSOは、レンダリングパイプラインの状態を素早く、一括して変更できるように導入された.
	// PSOによって、多くのステートに対してそれぞれ状態変更コマンドを送らずとも、単にPSOを切り替えるコマンドを送るだけで済む.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = pso.rootSignature.Get();
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	if (hs && ds) {
		psoDesc.HS = { hullShaderBlob->GetBufferPointer(), hullShaderBlob->GetBufferSize() };
		psoDesc.DS = { domainShaderBlob->GetBufferPointer(), domainShaderBlob->GetBufferSize() };
	}
    psoDesc.BlendState = blendDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	if (hs && ds) {
//		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		psoDesc.InputLayout.pInputElementDescs = terrainVertexLayout;
		psoDesc.InputLayout.NumElements = sizeof(terrainVertexLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	} else {
		psoDesc.InputLayout.pInputElementDescs = vertexLayout;
		psoDesc.InputLayout.NumElements = sizeof(vertexLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc = { 1, 0 };
	if (warp) {
//		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
	}
	if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.pso)))) {
		return false;
	}
	return true;
}

}

/**
* PSOを作成する.
*
* @retval true  作成成功.
* @retval false 作成失敗.
*/
bool CreatePSOList(ID3D12Device* device, bool warp)
{
	CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);

	psoList.resize(countof_PSOType);
	if (!CreatePSO(psoList[PSOType_Simple], device, warp, L"Res/VertexShader.hlsl", L"Res/PixelShader.hlsl", nullptr, nullptr, blendDesc)) {
		return false;
	}
	if (!CreatePSO(psoList[PSOType_NoiseTexture], device, warp, L"Res/VertexShader.hlsl", L"Res/NoiseTexture.hlsl", nullptr, nullptr, blendDesc)) {
		return false;
	}
	if (!CreatePSO(psoList[PSOType_Terrain], device, warp, L"Res/TerrainVS.hlsl", L"Res/TerrainPS.hlsl", L"Res/TerrainHS.hlsl", L"Res/TerrainDS.hlsl", blendDesc)) {
		return false;
	}
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	if (!CreatePSO(psoList[PSOType_Sprite], device, warp, L"Res/VertexShader.hlsl", L"Res/PixelShader.hlsl", nullptr, nullptr, blendDesc)) {
		return false;
	}
	return true;
}

/**
* PSOを取得する.
*
* @param type 取得するPSOの種類.
*
* @return typeに対応するPSO.
*/
const PSO& GetPSO(PSOType type)
{
	return psoList[type];
}
