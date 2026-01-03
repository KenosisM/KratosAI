#pragma once

#include <string>
#include <vector>

#include <GeneralStructures.h>

#include <Common/INI/INIConfig.h>

#include <Ext/EffectType/Effect/EffectData.h>
#include <Ext/Helper/MathEx.h>
#include "CounterData.h"

class CountTriggerEntity
{
public:
	bool Enable = false;

	Point2D Range = { 0, -1 };
	int TriggeredTimes = -1; // 触发次数

	// 触发计数操作
	int Num = 0;
	CounterAction Action = CounterAction::ADD;
	bool ResetNum = false;
	bool RemoveCounter = false;

	// 触发AE操作
	bool Attach = false;
	std::vector<std::string> AttachEffects{};
	std::vector<double> AttachChances{};
	bool AttachToSource = false;
	bool AttachToCounterSource = false;
	bool AttachFromCounterSource = false;

	bool Remove = false;
	std::vector<std::string> RemoveEffects{};
	std::vector<int> RemoveEffectsLevel{};
	std::vector<std::string> RemoveEffectsWithMarks{};
	bool RemoveEffectsSkipNext = false;
	bool RemoveToSource = false;
	bool RemoveToCounterSource = false;

	virtual void Read(INIBufferReader* reader, std::string title)
	{
		Range = reader->Get(title + "Range", Range);
		TriggeredTimes = reader->Get(title + "TriggeredTimes", TriggeredTimes);

		Num = reader->Get(title + "Num", Num);
		Action = reader->Get(title + "Action", Action);
		ResetNum = reader->Get(title + "ResetNum", ResetNum);
		RemoveCounter = reader->Get(title + "RemoveCounter", RemoveCounter);

		AttachEffects = reader->GetList(title + "AttachEffects", AttachEffects);
		ClearIfGetNone(AttachEffects);
		AttachChances = reader->GetChanceList(title + "AttachChances", AttachChances);
		Attach = !AttachEffects.empty();
		AttachToSource = reader->Get(title + "AttachToSource", AttachToSource);
		AttachToCounterSource = reader->Get(title + "AttachToCounterSource", AttachToCounterSource);
		AttachFromCounterSource = reader->Get(title + "AttachFromCounterSource", AttachFromCounterSource);
		if (AttachFromCounterSource)
		{
			AttachToSource = false;
			AttachToCounterSource = false;
		}
		else if (AttachToCounterSource)
		{
			AttachToSource = false;
		}

		RemoveEffects = reader->GetList(title + "RemoveEffects", RemoveEffects);
		ClearIfGetNone(RemoveEffects);
		RemoveEffectsLevel = reader->GetList(title + "RemoveEffectsLevel", RemoveEffectsLevel);
		RemoveEffectsWithMarks = reader->GetList(title + "RemoveEffectsWithMarks", RemoveEffectsWithMarks);
		ClearIfGetNone(RemoveEffectsWithMarks);
		Remove = !RemoveEffects.empty() || !RemoveEffectsWithMarks.empty();
		RemoveEffectsSkipNext = reader->Get(title + "RemoveEffectsSkipNext", RemoveEffectsSkipNext);
		RemoveToSource = reader->Get(title + "RemoveToSource", RemoveToSource);
		RemoveToCounterSource = reader->Get(title + "RemoveToCounterSource", RemoveToCounterSource);
		if (RemoveToCounterSource)
		{
			RemoveToSource = false;
		}

		Enable = Num != 0 || ResetNum || RemoveCounter || Attach || Remove;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Enable)
			.Process(this->Range)
			.Process(this->TriggeredTimes)

			.Process(this->Num)
			.Process(this->Action)
			.Process(this->ResetNum)
			.Process(this->RemoveCounter)

			.Process(this->Attach)
			.Process(this->AttachEffects)
			.Process(this->AttachChances)
			.Process(this->AttachToSource)
			.Process(this->AttachToCounterSource)
			.Process(this->AttachFromCounterSource)

			.Process(this->Remove)
			.Process(this->RemoveEffects)
			.Process(this->RemoveEffectsLevel)
			.Process(this->RemoveEffectsWithMarks)
			.Process(this->RemoveEffectsSkipNext)
			.Process(this->RemoveToSource)
			.Process(this->RemoveToCounterSource)

			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange)
	{
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const
	{
		return const_cast<CountTriggerEntity*>(this)->Serialize(stream);
	}
#pragma endregion
};

class CountTriggerData : public EffectData
{
public:
	EFFECT_DATA(CountTrigger);

	std::string Mark{};

	std::vector<CountTriggerEntity> Actions{}; // 触发效果列表

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "CountTrigger.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		Mark = reader->Get(title + "Mark", Mark);

		// 读取无序号的
		CountTriggerEntity defaultEntity;
		defaultEntity.Read(reader, title);
		if (defaultEntity.Enable)
		{
			Actions.push_back(defaultEntity);
		}
		// 读取有序号的
		for (int i = 0; i < 128; i++)
		{
			CountTriggerEntity entity{};
			entity.Read(reader, "CountTrigger" + std::to_string(i) + ".");
			if (entity.Enable)
			{
				Actions.push_back(entity);
			}
		}

		Enable = IsNotNone(Mark) && !Actions.empty();
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Mark)
			.Process(this->Actions)

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
		return const_cast<CountTriggerData*>(this)->Serialize(stream);
	}
#pragma endregion
};
