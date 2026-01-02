#pragma once

#include <string>
#include <vector>

#include <GeneralStructures.h>

#include <Common/INI/INIConfig.h>

#include <Ext/EffectType/Effect/EffectData.h>
#include <Ext/Helper/MathEx.h>
#include <Ext/EffectType/Effect/StackData.h>

enum class CounterAction : int
{
	INIT = 0,
	ADD = 1,
	SUB = 2,
	MUL = 3,
};

template <>
inline bool Parser<CounterAction>::TryParse(const char* pValue, CounterAction* outValue)
{
	switch (toupper(static_cast<unsigned char>(*pValue)))
	{
	case 'I':
		if (outValue)
		{
			*outValue = CounterAction::INIT;
		}
		return true;
	case 'A':
		if (outValue)
		{
			*outValue = CounterAction::ADD;
		}
		return true;
	case 'S':
		if (outValue)
		{
			*outValue = CounterAction::SUB;
		}
		return true;
	case 'M':
		if (outValue)
		{
			*outValue = CounterAction::MUL;
		}
		return true;
	default:
		if (outValue)
		{
			*outValue = CounterAction::ADD;
		}
		return true;
	}
}

class CounterData : public EffectData
{
public:
	EFFECT_DATA(Counter);

	std::string Mark{};
	int Num = 0;
	CounterAction Action = CounterAction::ADD;
	bool RemoveWhenZero = true;
	bool ResetNum = false;

	Condition Condition = Condition::EQ;
	int Level = 0;

	bool Attach = false;
	std::vector<std::string> AttachEffects{};
	std::vector<double> AttachChances{};
	bool AttachToSource = false;

	bool Remove = false;
	std::vector<std::string> RemoveEffects{};
	std::vector<int> RemoveEffectsLevel{};
	std::vector<std::string> RemoveEffectsWithMarks{};
	bool RemoveEffectsSkipNext = false;
	bool RemoveToSource = false;

	std::vector<int> RemoveLevel{};
	bool RemoveAll = true;
	bool RemoveSkipNext = false;

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "Counter.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		Mark = reader->Get(title + "Mark", Mark);
		Num = reader->Get(title + "Num", Num);
		Action = reader->Get(title + "Action", Action);
		RemoveWhenZero = reader->Get(title + "RemoveWhenZero", RemoveWhenZero);
		ResetNum = reader->Get(title + "ResetNum", ResetNum);

		Condition = reader->Get(title + "Condition", Condition);
		Level = reader->Get(title + "Level", Level);

		AttachEffects = reader->GetList(title + "AttachEffects", AttachEffects);
		ClearIfGetNone(AttachEffects);
		AttachChances = reader->GetChanceList(title + "AttachChances", AttachChances);
		Attach = !AttachEffects.empty();
		AttachToSource = reader->Get(title + "AttachToSource", AttachToSource);

		RemoveEffects = reader->GetList(title + "RemoveEffects", RemoveEffects);
		ClearIfGetNone(RemoveEffects);
		RemoveEffectsLevel = reader->GetList(title + "RemoveEffectsLevel", RemoveEffectsLevel);
		RemoveEffectsWithMarks = reader->GetList(title + "RemoveEffectsWithMarks", RemoveEffectsWithMarks);
		ClearIfGetNone(RemoveEffectsWithMarks);
		Remove = !RemoveEffects.empty() || !RemoveEffectsWithMarks.empty();
		RemoveEffectsSkipNext = reader->Get(title + "RemoveEffectsSkipNext", RemoveEffectsSkipNext);
		RemoveToSource = reader->Get(title + "RemoveToSource", RemoveToSource);

		RemoveLevel = reader->GetList(title + "RemoveLevel", RemoveLevel);
		RemoveAll = reader->Get(title + "RemoveAll", RemoveAll);
		RemoveSkipNext = reader->Get(title + "RemoveSkipNext", RemoveSkipNext);

		Enable = !Mark.empty();
	}

	bool CanAttach()
	{
		return Action == CounterAction::ADD || Action == CounterAction::INIT;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Mark)
			.Process(this->Num)
			.Process(this->Action)
			.Process(this->RemoveWhenZero)
			.Process(this->ResetNum)

			.Process(this->Condition)
			.Process(this->Level)

			.Process(this->Attach)
			.Process(this->AttachEffects)
			.Process(this->AttachChances)
			.Process(this->AttachToSource)

			.Process(this->Remove)
			.Process(this->RemoveEffects)
			.Process(this->RemoveEffectsLevel)
			.Process(this->RemoveEffectsWithMarks)
			.Process(this->RemoveEffectsSkipNext)
			.Process(this->RemoveToSource)

			.Process(this->RemoveLevel)
			.Process(this->RemoveAll)
			.Process(this->RemoveSkipNext)
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
		return const_cast<CounterData*>(this)->Serialize(stream);
	}
#pragma endregion
};
