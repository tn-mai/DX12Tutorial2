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
* ���ԃx�[�X�̃C�x���g����N���X.
*/
class TimeBasedProducer
{
public:
	typedef std::function<void(uint32_t type, uint32_t action, const DirectX::XMFLOAT2& pos)> GenSpriteFunc; ///< �X�v���C�g�����֐��^.
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

	/// �ґ��ɒǉ�����G�f�[�^�^.
	struct EnemyEntryData {
		uint32_t enemyType; ///< �G�̎��.
		uint32_t action; ///< ������@.
		DirectX::XMFLOAT2 offset; ///< �ґ��̒��S����̑��Έʒu.
		float interval; ///< �ґ��ɒǉ������܂ł̑ҋ@����.
	};
	typedef std::vector<EnemyEntryData> FormationList; ///< �G�ґ��f�[�^�^.

	/// �G�o���C�x���g�^.
	struct Event {
		float time; ///< �C�x���g�����\�莞��.
		DirectX::XMFLOAT2 pos; ///< �C�x���g�����ʒu.
		FormationList::const_iterator begin; ///< �C�x���g�̕ґ��f�[�^�̐擪.
		FormationList::const_iterator end; ///< �C�x���g�̕ґ��f�[�^�̏I�[.
	};
	typedef std::vector<Event> EventList; ///< �C�x���g���X�g�^.

	/// �������C�x���g�^.
	struct ActiveEvent {
		double time; ///< �C�x���g��������̌o�ߎ���.
		FormationList::const_iterator current; ///< ���ꂩ�珈������G�f�[�^�̈ʒu.
		EventList::const_iterator event; ///< ���s����C�x���g�f�[�^�̈ʒu.
	};

	FormationList formationList; ///< �G�ґ��f�[�^.
	EventList schedule; ///< �C�x���g���X�g.
	std::list<ActiveEvent> activeEventList; ///< �������̃C�x���g�̃��X�g.
	EventList::const_iterator itrCurrentEvent = schedule.end(); ///< ���ꂩ�珈������C�x���g�̈ʒu.
	double time = 0; ///< ���o�ߎ���.
};

} // namespace EventProducer

#endif // DX12TUTORIAL_SRC_TIMEBASEDPRODUCER_H_