/**
* @file Animation.cpp
*/
#include "Animation.h"
#include "Json.h"
#include <windows.h>
#include <map>
#include <vector>
#include <string>

/**
* コンストラクタ.
*
* @param al 設定するアニメーションリスト.
*/
AnimationController::AnimationController(const AnimationList& al)
	: list(al)
	, seqIndex(0)
	, cellIndex(0)
	, time(0.0f)
{
}

/**
* アニメーションシーケンスのインデックスを設定する.
*
* @param idx 設定するシーケンスインデックス.
*/
void AnimationController::SetSeqIndex(uint32_t idx)
{
	if (idx >= list.list.size()) {
		return;
	}
	seqIndex = idx;
	cellIndex = 0;
	time = 0.0f;
}

/**
* アニメーションの状態を更新する.
*
* @param delta 経過時間.
*/
void AnimationController::Update(double delta)
{
	if (seqIndex >= list.list.size() || list.list[seqIndex].empty()) {
		return;
	}

	time += delta;
	for (;;) {
		const float targetTime = list.list[seqIndex][cellIndex].time;
		if (targetTime <= 0.0f || time < targetTime) {
			break;
		}
		time -= targetTime;
		++cellIndex;
		if (cellIndex >= list.list[seqIndex].size()) {
			cellIndex = 0;
		}
	}
}

/**
* アニメーションデータを取得する.
*
* @return アニメーションデータ.
*/
const AnimationData& AnimationController::GetData() const
{
	if (seqIndex >= list.list.size() || list.list[seqIndex].empty()) {
		static const AnimationData dummy{};
		return dummy;
	}
	return list.list[seqIndex][cellIndex];
}

/**
* リスト内のアニメーションシーケンスの数を取得する.
*
* @return アニメーションシーケンスの数.
*/
size_t AnimationController::GetSeqCount() const
{
	return list.list.size();
}

/**
* アニメーションが終了しているか調べる.
*
* @retval true  終了している.
* @retval false 再生中.
*/
bool AnimationController::IsFinished() const
{
	if (seqIndex >= list.list.size() || list.list[seqIndex].empty()) {
		return true;
	}
	return list.list[seqIndex][cellIndex].time < 0;
}

/**
* ファイルからアニメーションリストを読み込む.
*
* @param list     読み込み先オブジェクト.
* @param filename ファイル名.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*
* [
*   {
*     "name" : animation list name string
*     "list" :
*     [
*       [
*         {
*           "cell" : cell index,
*           "time" : duration time,
*           "rotation" : rotation radian,
*           "scale" : [x, y],
*           "color" : [r, g, b, a]
*         },
*         ...
*       ],
*       ...
*     ]
*   },
*   ...
* ]
*/
AnimationFile LoadAnimationFromJsonFile(const wchar_t* filename)
{
	struct HandleHolder {
		explicit HandleHolder(HANDLE h) : handle(h) {}
		~HandleHolder() { if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); } }
		HANDLE handle;
		operator HANDLE() { return handle; }
		operator HANDLE() const { return handle; }
	};

	HandleHolder h(CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (h == INVALID_HANDLE_VALUE) {
		return {};
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(h, &size)) {
		return {};
	}
	if (size.QuadPart > std::numeric_limits<size_t>::max()) {
		return {};
	}
	std::vector<char> buffer;
	buffer.resize(static_cast<size_t>(size.QuadPart));
	DWORD readBytes;
	if (!ReadFile(h, &buffer[0], buffer.size(), &readBytes, nullptr)) {
		return {};
	}
	const Json::Result result = Json::Parse(buffer.data(), buffer.data() + buffer.size());
	if (!result.error.empty()) {
		OutputDebugStringA(result.error.c_str());
		return {};
	}
	AnimationFile af;
	const Json::Array& json = result.value.AsArray();
	for (const Json::Value& e : json) {
		const Json::Object& object = e.AsObject();
		AnimationList al;
		const Json::Object::const_iterator itrName = object.find("name");
		if (itrName != object.end()) {
			al.name = itrName->second.AsString();
		}
		const Json::Object::const_iterator itrList = object.find("list");
		if (itrList == object.end()) {
			break;
		}
		for (const Json::Value& seq : itrList->second.AsArray()) {
			AnimationSequence as;
			for (const Json::Value& data : seq.AsArray()) {
				const Json::Object& obj = data.AsObject();
				AnimationData ad;
				ad.cellIndex = static_cast<uint32_t>(obj.find("cell")->second.AsNumber());
				ad.time = static_cast<float>(obj.find("time")->second.AsNumber());
				ad.rotation = static_cast<float>(obj.find("rotation")->second.AsNumber());
				{
					auto itr = obj.find("scale");
					if (itr == obj.end()) {
						return af;
					}
					const Json::Array& scale = itr->second.AsArray();
					if (scale.size() < 2) {
						return af;
					}
					ad.scale.x = static_cast<float>(scale[0].AsNumber());
					ad.scale.y = static_cast<float>(scale[1].AsNumber());
				}
				{
					auto itr = obj.find("color");
					if (itr == obj.end()) {
						return af;
					}
					const Json::Array& color = itr->second.AsArray();
					if (color.size() < 4) {
						return af;
					}
					ad.color.x = static_cast<float>(color[0].AsNumber());
					ad.color.y = static_cast<float>(color[1].AsNumber());
					ad.color.z = static_cast<float>(color[2].AsNumber());
					ad.color.w = static_cast<float>(color[3].AsNumber());
				}
				as.push_back(ad);
			}
			al.list.push_back(as);
		}
		af.push_back(al);
	}

	return af;
}

/**
* アニメーションリストを取得する.
*
* @return アニメーションリスト.
*/
const AnimationList& GetAnimationList()
{
	static AnimationFile af;
	if (af.empty()) {
		af = LoadAnimationFromJsonFile(L"Res/Anm/Animation.json");
	}
	return af[0];
}
