#pragma once

#include "../EffectScript.h"
#include "PassengerEnterData.h"

class PassengerEnterEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(PassengerEnter);

	virtual void Clean() override
	{
		EffectScript::Clean();

		_transporter = nullptr;
	}

	virtual void OnUpdate() override;
	virtual void OnWarpUpdate() override;

#pragma region Save/Load
	template <typename T>
	bool Serialize(T& stream) {
		return stream
			.Process(this->_transporter)
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
		return const_cast<PassengerEnterEffect*>(this)->Serialize(stream);
	}
#pragma endregion
private:
	TechnoClass* _transporter = nullptr;
};
