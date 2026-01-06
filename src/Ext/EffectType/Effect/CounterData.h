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

class CounterEntity
{
public:
	bool Enable = false;
	Point2D RemoveWhenNum = { 0, -1 };

	virtual void Read(INIBufferReader* reader, std::string title)
	{
		RemoveWhenNum = reader->Get(title + "RemoveWhenNum", RemoveWhenNum);
		Enable = RemoveWhenNum.Y > 0 && RemoveWhenNum.Y >= RemoveWhenNum.X;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Enable)
			.Process(this->RemoveWhenNum)

			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange)
	{
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const
	{
		return const_cast<CounterEntity*>(this)->Serialize(stream);
	}
#pragma endregion
};

enum class CounterType : int
{
	Number = 0,
	HP = 1,
	MaxHP = 2,
};

class CounterData : public EffectData
{
public:
	EFFECT_DATA(Counter);

	std::string Mark{};

	double Num = 0;
	CounterType NumType = CounterType::Number;
	bool NumFromSource = false;

	double Min = 0;
	double Max = -1;
	CounterAction Action = CounterAction::ADD;
	bool AttachIfNotFound = true;

	std::vector<CounterEntity> RemoveWhenNums{}; // 触发效果列表

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "Counter.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		Mark = reader->Get(title + "Mark", Mark);

		// 初始数字特殊格式
		std::string numStr{ "" };
		numStr = reader->Get(title + "Num", numStr);
		if (IsNotNone(numStr))
		{
			if (std::regex_match(numStr, INIReader::Number))
			{
				int buffer = 0;
				const char* pFmt = "%d";
				if (sscanf_s(numStr.c_str(), pFmt, &buffer) == 1)
				{
					Num = buffer;
					NumType = CounterType::Number;
				}
			}
			else
			{
				const char v = *uppercase(numStr).substr(0, 1).c_str();
				switch (v)
				{
				case 'H': // HP
					Num = 0;
					NumType = CounterType::HP;
					break;
				case 'M': // MAXHP
					Num = 0;
					NumType = CounterType::MaxHP;
					break;
				}
			}
		}
		NumFromSource = reader->Get(title + "NumFromSource", NumFromSource);

		Min = reader->Get(title + "Min", Min);
		Max = reader->Get(title + "Max", Max);
		Action = reader->Get(title + "Action", Action);
		AttachIfNotFound = reader->Get(title + "AttachIfNotFound", AttachIfNotFound);

		// 读取无序号的
		CounterEntity defaultEntity;
		defaultEntity.Read(reader, title);
		if (defaultEntity.Enable)
		{
			RemoveWhenNums.push_back(defaultEntity);
		}
		// 读取有序号的
		for (int i = 0; i < 128; i++)
		{
			CounterEntity entity{};
			entity.Read(reader, "Counter" + std::to_string(i) + ".");
			if (entity.Enable)
			{
				RemoveWhenNums.push_back(entity);
			}
		}

		Enable = IsNotNone(Mark);
	}

	bool CanAttach()
	{
		return AttachIfNotFound && (Action == CounterAction::ADD || Action == CounterAction::INIT);
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Mark)
			.Process(this->Num)
			.Process(this->NumType)
			.Process(this->NumFromSource)
			.Process(this->Min)
			.Process(this->Max)
			.Process(this->Action)
			.Process(this->RemoveWhenNums)

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
