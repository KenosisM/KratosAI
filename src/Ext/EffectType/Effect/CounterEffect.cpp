#include "CounterEffect.h"

#include <Ext/Helper/Finder.h>
#include <Ext/Helper/FLH.h>
#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Status.h>

bool CounterEffect::CanActive(int num, int level, Condition condition)
{
	if (level >= 0)
	{
		switch (condition)
		{
		case Condition::EQ:
			return num == level;
		case Condition::NE:
			return num != level;
		case Condition::GT:
			return num > level;
		case Condition::LT:
			return num < level;
		case Condition::GE:
			return num >= level;
		case Condition::LE:
			return num <= level;
		}
	}
	return true;
}

void CounterEffect::Watch()
{
	if (CanActive(CountNum, Data->Level, Data->Condition))
	{
		_count++;
		// 添加AE
		if (Data->Attach)
		{
			AttachEffect* aeManager = AE->AEManager;
			TechnoClass* pSource = AE->pSource;
			HouseClass* pSourceHouse = AE->pSourceHouse;
			if (Data->AttachToSource)
			{
				aeManager = GetAEManager<TechnoExt>(AE->pSource);
				if (pTechno)
				{
					pSource = pTechno;
					pSourceHouse = pTechno->Owner;
				}
				else if (pBullet)
				{
					pSource = pBullet->Owner;
					pSourceHouse = GetHouse(pBullet);
				}
				else
				{
					aeManager = nullptr;
				}
			}
			if (aeManager)
			{
				aeManager->Attach(Data->AttachEffects, Data->AttachChances, false, pSource, pSourceHouse);
			}
		}
		// 移除AE
		if (Data->Remove)
		{
			AttachEffect* aeManager = AE->AEManager;
			if (Data->RemoveToSource)
			{
				aeManager = GetAEManager<TechnoExt>(AE->pSource);
			}
			if (aeManager)
			{
				if (!Data->RemoveEffects.empty())
				{
					if (!Data->RemoveEffectsLevel.empty())
					{
						// 移除指定的层数
						std::map<std::string, int> aeTypes;
						int idx = 0;
						int count = Data->RemoveEffects.size();
						for (std::string removeAE : Data->RemoveEffects)
						{
							int level = -1;
							if (idx < count)
							{
								level = Data->RemoveEffectsLevel[idx];
							}
							if (level > 0)
							{
								aeTypes[removeAE] = level;
							}
						}
						if (!aeTypes.empty())
						{
							aeManager->DetachByName(aeTypes, Data->RemoveEffectsSkipNext);
						}
					}
					else
					{
						aeManager->DetachByName(Data->RemoveEffects, Data->RemoveEffectsSkipNext);
					}
				}
				if (!Data->RemoveEffectsWithMarks.empty())
				{
					aeManager->DetachByMarks(Data->RemoveEffectsWithMarks, Data->RemoveEffectsSkipNext);
				}
			}
		}
		// 检查触发次数
		if (Data->TriggeredTimes > 0 && ++_count >= Data->TriggeredTimes)
		{
			Deactivate();
			AE->TimeToDie();
		}
		else if(Data->ResetNum)
		{
			CountNum = Data->Num;
		}
	}
	// 计数器归零时移除
	if (Data->RemoveWhenZero && CountNum <= 0)
	{
		Deactivate();
		AE->TimeToDie();
	}
}

void CounterEffect::OnStart()
{
	CountNum = Data->Num;
}

void CounterEffect::OnUpdate()
{
	if (!AE->OwnerIsDead())
	{
		Watch();
	}
}

void CounterEffect::OnWarpUpdate()
{
	if (!AE->OwnerIsDead())
	{
		Watch();
	}
}

void CounterEffect::ModifyCount(CounterData delta)
{
	// Debug::Log("CounterEffect[%s]::ModifyCount - Before: CountNum=%d\n", Data->Mark.c_str(), CountNum);
	// Debug::Log("  Operation: Action=%d, Num=%d\n", (int)delta.Action, delta.Num);

	switch (delta.Action)
	{
	case CounterAction::INIT:
		CountNum = delta.Num;
		break;
	case CounterAction::ADD:
		CountNum += delta.Num;
		break;
	case CounterAction::SUB:
		CountNum -= delta.Num;
		break;
	case CounterAction::MUL:
		CountNum *= delta.Num;
		break;
	}

	// Debug::Log("  After calculation: CountNum=%d\n", CountNum);

	if (CountNum < Data->Min)
	{
		// Debug::Log("  Clamped to Min=%d\n", Data->Min);
		CountNum = Data->Min;
	}
	if (Data->Max >= 0 && CountNum > Data->Max)
	{
		// Debug::Log("  Clamped to Max=%d\n", Data->Max);
		CountNum = Data->Max;
	}

	// Debug::Log("  Final: CountNum=%d\n", CountNum);
}
