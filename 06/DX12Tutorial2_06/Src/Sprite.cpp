/**
* @file Sprite.cpp
*/
#include "Sprite.h"
#include "Texture.h"
#include "PSO.h"
#include "Json.h"
#include "d3dx12.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace Sprite {

/**
* スプライト描画用頂点データ型.
*/
struct Vertex {
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
};

namespace /* unnamed */ {

XMFLOAT3 RotateZ(XMVECTOR c, float x, float y, float r)
{
	float fs, fc;
	XMScalarSinCos(&fs, &fc, r);
	const float rx = fc * x + fs * y;
	const float ry = -fs * x + fc * y;
	const XMVECTORF32 tmp{ rx, ry, 0.0f, 0.0f };
	XMFLOAT3 ret;
	XMStoreFloat3(&ret, XMVectorAdd(c, tmp));
	return ret;
}

/**
* ひとつのスプライトデータを頂点バッファに設定.
*
* @param sprite スプライトデータ.
* @param v      頂点データを描き込むアドレス.
* @param offset スクリーン左上座標.
*/
void AddVertex(const Sprite& sprite, const Cell* cell, const AnimationData& anm, Vertex* v, XMFLOAT2 offset)
{
	const XMVECTORF32 center{ offset.x + sprite.pos.x, offset.y - sprite.pos.y, sprite.pos.z, 0.0f };
	const XMFLOAT2 halfSize{ cell->ssize.x * 0.5f * sprite.scale.x * anm.scale.x, cell->ssize.y * 0.5f * sprite.scale.y * anm.scale.y };

	const XMVECTOR vcolor = XMVectorMultiply(XMLoadFloat4(&sprite.color), XMLoadFloat4(&anm.color));
	for (int i = 0; i < 4; ++i) {
		XMStoreFloat4(&v[i].color, vcolor);
	}
	const float rot = sprite.rotation + anm.rotation;
	v[0].position = RotateZ(center, -halfSize.x, halfSize.y, rot);
	v[0].texcoord.x = cell->uv.x;
	v[0].texcoord.y = cell->uv.y;

	v[1].position = RotateZ(center, halfSize.x, halfSize.y, rot);
	v[1].texcoord.x = cell->uv.x + cell->tsize.x;
	v[1].texcoord.y = cell->uv.y;

	v[2].position = RotateZ(center, halfSize.x, -halfSize.y, rot);
	v[2].texcoord.x = cell->uv.x + cell->tsize.x;
	v[2].texcoord.y = cell->uv.y + cell->tsize.y;

	v[3].position = RotateZ(center, -halfSize.x, -halfSize.y, rot);
	v[3].texcoord.x = cell->uv.x;
	v[3].texcoord.y = cell->uv.y + cell->tsize.y;
}

} // unnamed namedpace

/**
* コンストラクタ.
*
* @param al アニメーションリスト.
* @param p  スプライトの座標.
* @param rot 回転(ラジアン).
* @param s   拡大率.
* @param col 表示色.
*/
Sprite::Sprite(const AnimationList& al, DirectX::XMFLOAT3 p, float rot, DirectX::XMFLOAT2 s, DirectX::XMFLOAT4 col) :
	animeController(al),
	actController(),
	collisionId(-1),
	pos(p),
	rotation(rot),
	scale(s),
	color(col)
{
}

/**
* コンストラクタ.
*/
Renderer::Renderer() :
	maxSpriteCount(0),
	frameBufferCount(0),
	currentFrameIndex(-1)
{
}

/**
* Rendererを初期化する.
*
* @param device           D3Dデバイス.
* @param frameBufferCount フレームバッファの数.
* @param maxSprite        描画できる最大スプライト数.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool Renderer::Init(ComPtr<ID3D12Device> device, int numFrameBuffer, int maxSprite, Resource::ResourceLoader& resourceLoader)
{
	maxSpriteCount = maxSprite;
	frameBufferCount = numFrameBuffer;

	frameResourceList.resize(numFrameBuffer);
	for (int i = 0; i < frameBufferCount; ++i) {
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameResourceList[i].commandAllocator)))) {
			return false;
		}
		if (FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(maxSpriteCount * sizeof(Vertex)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&frameResourceList[i].vertexBuffer)
		))) {
			return false;
		}
		frameResourceList[i].vertexBuffer->SetName(L"Sprite Vertex Buffer");
		CD3DX12_RANGE range(0, 0);
		if (FAILED(frameResourceList[i].vertexBuffer->Map(0, &range, &frameResourceList[i].vertexBufferGPUAddress))) {
			return false;
		}
		frameResourceList[i].vertexBufferView.BufferLocation = frameResourceList[i].vertexBuffer->GetGPUVirtualAddress();
		frameResourceList[i].vertexBufferView.StrideInBytes = sizeof(Vertex);
		frameResourceList[i].vertexBufferView.SizeInBytes = static_cast<UINT>(maxSpriteCount * sizeof(Vertex));
	}

	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameResourceList[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)))) {
		return false;
	}
	if (FAILED(commandList->Close())) {
		return false;
	}

	const int indexListSize = static_cast<int>(maxSpriteCount * 6 * sizeof(DWORD));
#if 1
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
	indexBuffer->SetName(L"Sprite Index Buffer");
	CD3DX12_RANGE range(0, 0);
	void* tmpIndexBufferAddress;
	if (FAILED(indexBuffer->Map(0, &range, &tmpIndexBufferAddress))) {
		return false;
	}
	DWORD* pIndexBuffer = static_cast<DWORD*>(tmpIndexBufferAddress);
	for (size_t i = 0; i < maxSpriteCount; ++i) {
		pIndexBuffer[i * 6 + 0] = i * 4 + 0;
		pIndexBuffer[i * 6 + 1] = i * 4 + 1;
		pIndexBuffer[i * 6 + 2] = i * 4 + 2;
		pIndexBuffer[i * 6 + 3] = i * 4 + 2;
		pIndexBuffer[i * 6 + 4] = i * 4 + 3;
		pIndexBuffer[i * 6 + 5] = i * 4 + 0;
	}
	indexBuffer->Unmap(0, nullptr);
#else
	std::vector<DWORD> indexList;
	indexList.resize(maxSpriteCount * 6);
	for (size_t i = 0; i < maxSpriteCount; ++i) {
		indexList[i * 6 + 0] = i * 4 + 0;
		indexList[i * 6 + 1] = i * 4 + 1;
		indexList[i * 6 + 2] = i * 4 + 2;
		indexList[i * 6 + 3] = i * 4 + 2;
		indexList[i * 6 + 4] = i * 4 + 3;
		indexList[i * 6 + 5] = i * 4 + 0;
	}

	D3D12_SUBRESOURCE_DATA subresource = { indexList.data(), indexListSize, indexListSize };
	if (!resourceLoader.Upload(indexBuffer, CD3DX12_RESOURCE_DESC::Buffer(indexListSize), subresource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"Sprite Index Buffer")) {
		return false;
	}
#endif
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indexListSize;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator)))) {
		return false;
	}
	bundleList.reserve(32);

	return true;
}

/**
* バンドルを作成.
*
* @param pso         バンドルに設定するPSO.
* @param texDescHeap バンドルに設定するテクスチャ用のデスクリプタヒープへのポインタ.
* @param texture     バンドルに設定するテクスチャ.
*
* @return 作成に成功したら0以上のIDを持つBundleIdオブジェクトを返す.
*         失敗したら空のBundleIdオブジェクトを返す.
*/
BundleId Renderer::CreateBundle(const PSO& pso, ID3D12DescriptorHeap* texDescHeap, const Resource::Texture& texture)
{
	ComPtr<ID3D12Device> device;
	if (FAILED(bundleAllocator->GetDevice(IID_PPV_ARGS(&device)))) {
		return BundleId();
	}
	ComPtr<ID3D12GraphicsCommandList> bundle;
	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.Get(), nullptr, IID_PPV_ARGS(&bundle)))) {
		return BundleId();
	}
	bundle->SetGraphicsRootSignature(pso.rootSignature.Get());
	bundle->SetPipelineState(pso.pso.Get());
	ID3D12DescriptorHeap* heapList[] = { texDescHeap };
	bundle->SetDescriptorHeaps(_countof(heapList), heapList);
	bundle->SetGraphicsRootDescriptorTable(0, texture.handle);
	bundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bundle->IASetIndexBuffer(&indexBufferView);
	if (FAILED(bundle->Close())) {
		return BundleId();
	}
	size_t id;
	auto itr = std::find_if(bundleList.begin(), bundleList.end(), [](const ComPtr<ID3D12GraphicsCommandList>& e) { return !e; });
	if (itr != bundleList.end()) {
		*itr = bundle;
		id = itr - bundleList.begin();
	} else {
		bundleList.push_back(bundle);
		id = bundleList.size() - 1;
	}
	return BundleId(new size_t{ id }, [this](size_t* p) { this->DestroyBundle(*p); delete p; });
}

/**
* バンドルを削除.
*
* @param bundleId 削除するバンドルのID.
*/
void Renderer::DestroyBundle(size_t bundleId)
{
	if (bundleId < bundleList.size()) {
		bundleList[bundleId].Reset();
	}
}

/**
* スプライトの描画開始.
*
* @param frameIndex 現在のフレームバッファのインデックス.
*
* @retval true 描画可能な状態になった.
* @retval false 描画可能な状態への遷移に失敗.
*/
bool Renderer::Begin(int frameIndex)
{
	if (currentFrameIndex >= 0) {
		return false;
	}

	FrameResource& fr = frameResourceList[frameIndex];
	if (FAILED(fr.commandAllocator->Reset())) {
		return false;
	}
	if (FAILED(commandList->Reset(fr.commandAllocator.Get(), nullptr))) {
		return false;
	}

	currentFrameIndex = frameIndex;
	spriteCount = 0;
	return true;
}

/**
* スプライトを描画.
*
* @param spriteList 描画するスプライトのリスト.
* @param pso        描画に使用するPSO.
* @param texture    描画に使用するテクスチャ.
* @param info       描画情報.
*
* @retval true  コマンドリスト作成成功.
* @retval false コマンドリスト作成失敗.
*/
bool Renderer::Draw(const std::vector<Sprite>& spriteList, const Cell* cellList, const BundleId& bundleId, RenderingInfo& info)
{
	if (spriteList.empty()) {
		return true;
	}
	return Draw(&*spriteList.begin(), (&*spriteList.begin()) + spriteList.size(), cellList, bundleId, info);
}

/**
*
*/
bool Renderer::IsValidDrawStatus(const BundleId& bundleId)
{
	if (!bundleId || static_cast<size_t>(*bundleId) >= bundleList.size() || !bundleList[*bundleId]) {
		return false;
	}
	if (currentFrameIndex < 0) {
		return false;
	}
	return true;
}

/**
* スプライト描画ループの設定をする.
*
* @param cellList 描画に使用するセルリスト.
* @param bundleId 描画に使用するバンドルID.
* @param info     描画情報.
*
* @return 描画ループ用パラメータ.
*/
Renderer::DrawParamters Renderer::SetupDraw(const Cell* cellList, const BundleId& bundleId, RenderingInfo& info)
{
	FrameResource& fr = frameResourceList[currentFrameIndex];
	ID3D12DescriptorHeap* heapList[] = { info.texDescHeap };
	commandList->SetDescriptorHeaps(_countof(heapList), heapList);
	commandList->ExecuteBundle(bundleList[*bundleId].Get());
	commandList->SetGraphicsRoot32BitConstants(1, 16, &info.matViewProjection, 0);
	commandList->IASetVertexBuffers(0, 1, &fr.vertexBufferView);
	commandList->OMSetRenderTargets(1, &info.rtvHandle, FALSE, &info.dsvHandle);
	commandList->RSSetViewports(1, &info.viewport);
	commandList->RSSetScissorRects(1, &info.scissorRect);

	return DrawParamters{
		info,
		cellList,
		{ -(info.viewport.Width * 0.5f), info.viewport.Height * 0.5f },
		(fr.vertexBufferView.SizeInBytes / fr.vertexBufferView.StrideInBytes / 4) - spriteCount,
		0,
		static_cast<Vertex*>(fr.vertexBufferGPUAddress) + (spriteCount * 4)
	};
}

/**
* スプライトを描画する.
*
* @param param 　描画ループ用パラメータ.
* @param sprite 描画するスプライト.
*
* @retval true  描画を継続する.
* @retval false 描画を終了する.
*/
bool Renderer::Draw(DrawParamters& param, const Sprite& sprite)
{
	if (sprite.scale.x == 0 || sprite.scale.y == 0) {
		return true;
	}
	const Cell* cell = param.cellList + sprite.GetCellIndex();
	AddVertex(sprite, cell, sprite.animeController.GetData(), param.v, param.offset);
	++param.numSprite;
	if (param.numSprite >= param.remainingSprite) {
		return false;
	}
	param.v += 4;
	return true;
}

/**
* スプライト描画ループを終了する.
*
* @param param 描画ループ用パラメータ.
*/
void Renderer::TeardownDraw(const DrawParamters& param)
{
	commandList->DrawIndexedInstanced(param.numSprite * 6, 1, 0, spriteCount * 4, 0);
	spriteCount += param.numSprite;
}

/**
*
*/
bool Renderer::Draw(const Sprite* first, const Sprite* last, const Cell* cellList, const BundleId& bundleId, RenderingInfo& info)
{
	if (!IsValidDrawStatus(bundleId)) {
		return false;
	}
	if (first == last) {
		return true;
	}
	DrawParamters param = SetupDraw(cellList, bundleId, info);
	for (const Sprite* sprite = first; sprite != last; ++sprite) {
		if (!Draw(param, *sprite)) {
			break;
		}
	}
	TeardownDraw(param);
	return true;
}

/**
* スプライトの描画終了.
*
* @retval true  コマンドリスト作成成功.
* @retval false コマンドリスト作成失敗.
*/
bool Renderer::End()
{
	if (currentFrameIndex < 0) {
		return false;
	}
	currentFrameIndex = -1;
	if (FAILED(commandList->Close())) {
		return false;
	}
	return true;
}

/**
* コマンドリストを取得する.
*
* @return ID3D12GraphicsCommandListインターフェイスへのポインタ.
*/
ID3D12GraphicsCommandList* Renderer::GetCommandList()
{
	return commandList.Get();
}

/**
* Fileインターフェイスの実装クラス.
*/
class FileImpl : public File
{
public:
	FileImpl() {}
	virtual ~FileImpl() {}
	virtual const CellList* Get(uint32_t no) const {
		if (no >= clList.size()) {
			return nullptr;
		}
		return &clList[no];
	}
	virtual size_t Size() const { return clList.size(); }

	std::vector<CellList> clList;
};

/**
* ファイルからセルリストを読み込む.
*
* @param filename ファイル名.
*
* @return 読み込んだセルリスト.
*         読み込み失敗の場合はnullptrを返す.
*
* JSONフォーマットは次のとおり:
* <pre>
* [
*   {
*     "name" : "セルリスト名",
*     "texsize" : [w, h],
*     "list" : [
*       {
*         "uv" : [u, v],
*         "tsize" : [tw, th],
*         "ssize" : [sw, sh]
*       },
*       ...
*     ]
*   },
*   ...
* ]
* </pre>
*/
FilePtr LoadFromJsonFile(const wchar_t* filename)
{
	struct HandleHolder {
		explicit HandleHolder(HANDLE h) : handle(h) {}
		~HandleHolder() { if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); } }
		HANDLE handle;
		operator HANDLE() { return handle; }
		operator HANDLE() const { return handle; }
	};

	std::shared_ptr<FileImpl> af(new FileImpl);

	HandleHolder h(CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (h == INVALID_HANDLE_VALUE) {
		return af;
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(h, &size)) {
		return af;
	}
	if (size.QuadPart > std::numeric_limits<size_t>::max()) {
		return af;
	}
	std::vector<char> buffer;
	buffer.resize(static_cast<size_t>(size.QuadPart));
	DWORD readBytes;
	if (!ReadFile(h, buffer.data(), buffer.size(), &readBytes, nullptr)) {
		return af;
	}
	const Json::Result result = Json::Parse(buffer.data(), buffer.data() + buffer.size());
	if (!result.error.empty()) {
		OutputDebugStringA(result.error.c_str());
		return af;
	}
	const Json::Array& json = result.value.AsArray();

	for (const Json::Value& e : json) {
		const Json::Object& object = e.AsObject();
		CellList al;
		const Json::Object::const_iterator itrName = object.find("name");
		if (itrName != object.end()) {
			al.name = itrName->second.AsString();
		}
		const Json::Object::const_iterator itrTexSize = object.find("texsize");
		if (itrTexSize == object.end()) {
			break;
		}
		const Json::Array& ts = itrTexSize->second.AsArray();
		if (ts.size() < 2) {
			break;
		}
		const XMVECTOR texsize = XMVectorReciprocal({ static_cast<float>(ts[0].AsNumber()), static_cast<float>(ts[1].AsNumber()) });
		const Json::Object::const_iterator itrList = object.find("list");
		if (itrList == object.end()) {
			break;
		}
		for (const Json::Value& data : itrList->second.AsArray()) {
			const Json::Object& obj = data.AsObject();
			Cell cell;
			const Json::Array& uv = obj.find("uv")->second.AsArray();
			cell.uv.x = uv.size() > 0 ? static_cast<float>(uv[0].AsNumber()) : 0.0f;
			cell.uv.y = uv.size() > 1 ? static_cast<float>(uv[1].AsNumber()) : 0.0f;
			XMStoreFloat2(&cell.uv, XMVectorMultiply(XMLoadFloat2(&cell.uv), texsize));
			const Json::Array& tsize = obj.find("tsize")->second.AsArray();
			cell.tsize.x = tsize.size() > 0 ? static_cast<float>(tsize[0].AsNumber()) : 0.0f;
			cell.tsize.y = tsize.size() > 1 ? static_cast<float>(tsize[1].AsNumber()) : 0.0f;
			XMStoreFloat2(&cell.tsize, XMVectorMultiply(XMLoadFloat2(&cell.tsize), texsize));
			const Json::Array& ssize = obj.find("ssize")->second.AsArray();
			cell.ssize.x = ssize.size() > 0 ? static_cast<float>(ssize[0].AsNumber()) : 0.0f;
			cell.ssize.y = ssize.size() > 1 ? static_cast<float>(ssize[1].AsNumber()) : 0.0f;
			al.list.push_back(cell);
		}
		af->clList.push_back(al);
	}

	return af;
}

} // namespace Sprite
