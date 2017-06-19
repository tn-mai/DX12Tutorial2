/**
* @file TimeBasedProducer.cpp
*/
#include "TimeBasedProducer.h"
#include "Json.h"
#include "File.h"
#include <string>
#include <Windows.h>
#include <unordered_map>
#include <algorithm>

using namespace DirectX;

namespace EventProducer {

/**
* スケジュールをJSONファイルから読み込む.
*
* @param filename    ファイル名.
* @param actionFunc  アクション名をインデックスに変換する関数.
* @param enemyFunc   敵の名前をインデックスに変換する関数.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool TimeBasedProducer::LoadScheduleFromJsonFile(const wchar_t* filename, NameToIndexFunc actionFunc, NameToIndexFunc enemyFunc)
{
	File::BufferType buffer;
	if (!File::Read(filename, buffer)) {
		return false;
	}
	const Json::Result result = Json::Parse(buffer.data(), buffer.data() + buffer.size());
	if (!result.error.empty()) {
		OutputDebugStringA(result.error.c_str());
		return false;
	}

	const std::wstring strError = std::wstring(L"ERROR in ") + filename + L": ";
	struct Range {
		size_t begin;
		size_t end;
	};
	std::unordered_map<Json::String, Range> rangeList;

	const Json::Object& json = result.value.AsObject();
	const Json::Object::const_iterator itrFormation = json.find("formation");
	if (itrFormation == json.end()) {
		OutputDebugStringW((strError + L": formation要素がありません\n").data());
		return false;
	}
	const Json::Array& formations = itrFormation->second.AsArray();
	rangeList.reserve(formations.size());
	for (const Json::Value& e : formations) {
		const Json::Object& obj = e.AsObject();
		const Json::Object::const_iterator itrName = obj.find("name");
		if (itrName == obj.end()) {
			OutputDebugStringW((strError + L"formationデータにname要素がありません\n").data());
			continue;
		}
		const Json::Object::const_iterator itrList = obj.find("list");
		if (itrList == obj.end()) {
			OutputDebugStringW((strError + L"formationデータにlist要素がありません\n").data());
			continue;
		}
		Range range;
		range.begin = formationList.size();
		formationList.reserve(formationList.size() + itrList->second.AsArray().size());
		for (const Json::Value& e : itrList->second.AsArray()) {
			const Json::Object& data = e.AsObject();
			static const char* const dataNameList[] = { "type", "action", "offset", "interval" };
			Json::Object::const_iterator itr[_countof(dataNameList)];
			bool hasFailure = false;
			for (size_t i = 0; i < _countof(dataNameList); ++i) {
				itr[i] = data.find(dataNameList[i]);
				if (itr[i] == data.end()) {
					const std::wstring tmp(dataNameList[i], dataNameList[i] + strlen(dataNameList[i]));
					OutputDebugStringW((strError + L"formationデータに" + tmp + L"要素がありません\n").data());
					hasFailure = true;
				}
			}
			if (hasFailure) {
				continue;
			}
			const Json::Array& offset = itr[2]->second.AsArray();
			if (offset.size() < 2) {
				OutputDebugStringW((strError + L"formationデータのoffset要素数が不足しています\n").data());
				continue;
			}
			const Json::String& type = itr[0]->second.AsString();
			uint32_t typeId = enemyFunc(type.data());
			if (typeId == unknownName) {
				OutputDebugStringW((strError + std::wstring(type.begin(), type.end()) + L"は未知の敵です\n").data());
				typeId = 0;
			}
			const Json::String& action = itr[1]->second.AsString();
			uint32_t actionId = actionFunc(action.data());
			if (actionId == unknownName) {
				OutputDebugStringW((strError + std::wstring(action.begin(), action.end()) + L"は未知のアクションです\n").data());
				actionId = 0;
			}
			formationList.push_back({
				typeId,
				actionId,
				XMFLOAT2(offset[0].AsNumber<float>(), offset[1].AsNumber<float>()),
				itr[3]->second.AsNumber<float>()
			});
		}
		range.end = formationList.size();
		std::sort(formationList.begin() + range.begin, formationList.begin() + range.end,
			[](const EnemyEntryData& lhs, const EnemyEntryData& rhs) { return lhs.interval < rhs.interval; }
		);
		rangeList.insert(std::make_pair(itrName->second.AsString(), range));
	}

	const Json::Object::const_iterator itrSchedule = json.find("schedule");
	if (itrSchedule == json.end()) {
		OutputDebugStringW((strError + L": schedule要素がありません\n").data());
		return false;
	}
	const Json::Array& eventList = itrSchedule->second.AsArray();
	schedule.reserve(eventList.size());
	for (const Json::Value& e : eventList) {
		const Json::Object& data = e.AsObject();
		static const char* const dataNameList[] = { "time", "event", "position" };
		Json::Object::const_iterator itr[_countof(dataNameList)];
		bool hasFailure = false;
		for (size_t i = 0; i < _countof(dataNameList); ++i) {
			itr[i] = data.find(dataNameList[i]);
			if (itr[i] == data.end()) {
				const std::wstring tmp(dataNameList[i], dataNameList[i] + strlen(dataNameList[i]));
				OutputDebugStringW((strError + L"scheduleデータに" + tmp + L"要素がありません\n").data());
				hasFailure = true;
			}
		}
		if (hasFailure) {
			continue;
		}
		const Json::Array& pos = itr[2]->second.AsArray();
		if (pos.size() < 2) {
			OutputDebugStringW((strError + L"scheduleデータのposition要素数が不足しています\n").data());
			continue;
		}
		const Json::String& eventName = itr[1]->second.AsString();
		auto itrRange = rangeList.find(eventName);
		if (itrRange == rangeList.end()) {
			const std::wstring tmp(eventName.begin(), eventName.end());
			OutputDebugStringW((strError + tmp + L"はformationに存在しないイベントです\n").data());
			continue;
		}
		schedule.push_back({
			itr[0]->second.AsNumber<float>(),
			XMFLOAT2(pos[0].AsNumber<float>(), pos[1].AsNumber<float>()),
			formationList.begin() + itrRange->second.begin,
			formationList.begin() + itrRange->second.end
		});
	}
	std::sort(schedule.begin(), schedule.end(), [](const Event& lhs, const Event& rhs) { return lhs.time < rhs.time; });
	activeEventList.clear();
	itrCurrentEvent = schedule.begin();
	time = 0;
	return true;
}

/**
* 状態を更新する.
*
* @param delta  経過時間.
* @param func   スプライトを追加する関数.
*/
void TimeBasedProducer::Update(double delta, GenSpriteFunc func)
{
	time += delta;

	while (itrCurrentEvent != schedule.end()) {
		if (itrCurrentEvent->time > time) {
			break;
		}
		activeEventList.push_back({ 0, itrCurrentEvent->begin, itrCurrentEvent });
		++itrCurrentEvent;
	}

	for (auto itr = activeEventList.begin(); itr != activeEventList.end();) {
		while (itr->current != itr->event->end) {
			if (itr->current->interval > itr->time) {
				break;
			}
			XMFLOAT2 pos;
			XMStoreFloat2(&pos, XMLoadFloat2(&itr->event->pos) + XMLoadFloat2(&itr->current->offset));
			func(itr->current->enemyType, itr->current->action, pos);
			++itr->current;
		}
		itr->time += delta;
		if (itr->current == itr->event->end) {
			itr = activeEventList.erase(itr);
		} else {
			++itr;
		}
	}
}

} // namespace EventProducer
