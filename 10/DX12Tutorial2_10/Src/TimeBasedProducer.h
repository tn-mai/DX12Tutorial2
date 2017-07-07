/**
* @file TimeBasedProducer.h
*/
#ifndef DX12TUTORIAL_SRC_TIMEBASEDPRODUCER_H_
#define DX12TUTORIAL_SRC_TIMEBASEDPRODUCER_H_
#include <DirectXMath.h>
#include <vector>
#include <list>
#include <functional>
#include <stdint.h>

namespace EventProducer {

/**
* 時間ベースのイベント制御クラス.
*/
class TimeBasedProducer
{
public:
	typedef std::function<void(uint32_t type, uint32_t action, const DirectX::XMFLOAT2& pos)> GenSpriteFunc; ///< スプライト発生関数型.
	typedef std::function<uint32_t(const char*)> NameToIndexFunc;
	static const uint32_t unknownName = 0xffffffffU;

	TimeBasedProducer() = default;
	TimeBasedProducer(const TimeBasedProducer&) = default;
	~TimeBasedProducer() = default;
	TimeBasedProducer& operator=(const TimeBasedProducer&) = default;

	bool LoadScheduleFromJsonFile(const wchar_t* filename, NameToIndexFunc actionFunc, NameToIndexFunc enemyFunc);
	void Update(double delta, GenSpriteFunc func);
	bool IsFinish() const;

private:

	/// 編隊に追加する敵データ型.
	struct EnemyEntryData {
		uint32_t enemyType; ///< 敵の種類.
		uint32_t action; ///< 動作方法.
		DirectX::XMFLOAT2 offset; ///< 編隊の中心からの相対位置.
		float interval; ///< 編隊に追加されるまでの待機時間.
	};
	typedef std::vector<EnemyEntryData> FormationList; ///< 敵編隊データ型.

	/// 敵出現イベント型.
	struct Event {
		float time; ///< イベント発動予定時刻.
		DirectX::XMFLOAT2 pos; ///< イベント発動位置.
		FormationList::const_iterator begin; ///< イベントの編隊データの先頭.
		FormationList::const_iterator end; ///< イベントの編隊データの終端.
	};
	typedef std::vector<Event> EventList; ///< イベントリスト型.

	/// 発動中イベント型.
	struct ActiveEvent {
		double time; ///< イベント発動からの経過時間.
		FormationList::const_iterator current; ///< これから処理する敵データの位置.
		EventList::const_iterator event; ///< 実行するイベントデータの位置.
	};

	FormationList formationList; ///< 敵編隊データ.
	EventList schedule; ///< イベントリスト.
	std::list<ActiveEvent> activeEventList; ///< 発動中のイベントのリスト.
	EventList::const_iterator itrCurrentEvent = schedule.end(); ///< これから処理するイベントの位置.
	double time = 0; ///< 総経過時間.
};

} // namespace EventProducer

#endif // DX12TUTORIAL_SRC_TIMEBASEDPRODUCER_H_