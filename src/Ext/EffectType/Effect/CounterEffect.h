#pragma once

#include <string>
#include <vector>

#include <GeneralDefinitions.h>
#include <AnimClass.h>

#include "../EffectScript.h"
#include "CounterData.h"


/// @brief EffectScript
/// GameObject
///		|__ AttachEffect
///				|__ AttachEffectScript#0
///						|__ EffectScript#0
///						|__ EffectScript#1
///				|__ AttachEffectScript#1
///						|__ EffectScript#0
///						|__ EffectScript#1
///						|__ EffectScript#2
class CounterEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(Counter);

	virtual void Clean() override
	{
		EffectScript::Clean();

		_count = 0;
	}

	virtual void OnStart() override;

	virtual void OnUpdate() override;
	virtual void OnWarpUpdate() override;

	void ModifyCount(CounterData delta);

	// 计数器
	int CountNum = 0;

#pragma region Save/Load
	template <typename T>
	bool Serialize(T& stream) {
		return stream
			.Process(this->CountNum)
			.Process(this->_count)
			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectScript::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectScript::Save(stream);
		return const_cast<CounterEffect*>(this)->Serialize(stream);
	}
#pragma endregion
private:
	void Watch();
	bool CanActive(int Counters, int level, Condition condition);

	// 计数器触发次数
	int _count = 0;
};
