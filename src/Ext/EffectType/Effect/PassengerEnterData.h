#pragma once

#include <string>
#include <vector>

#include <GeneralStructures.h>

#include <Common/INI/INIConfig.h>

#include <Ext/EffectType/Effect/EffectData.h>

class PassengerEnterData : public EffectData
{
public:
	EFFECT_DATA(PassengerEnter);

	std::vector<std::string> AttachEffects{};
	bool SourceIsPassenger = true;

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "PassengerEnter.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		AttachEffects = reader->GetList(title + "AttachEffects", AttachEffects);
		SourceIsPassenger = reader->Get(title + "SourceIsPassenger", SourceIsPassenger);

		Enable = !AttachEffects.empty();
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->AttachEffects)
			.Process(this->SourceIsPassenger)
			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectData::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectData::Save(stream);
		return const_cast<PassengerEnterData*>(this)->Serialize(stream);
	}
#pragma endregion
};
