#include "CounterEffect.h"

#include <Ext/Helper/Finder.h>
#include <Ext/Helper/FLH.h>
#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Status.h>

void CounterEffect::Watch()
{
	// 计数器归零时移除
	for (CounterEntity& entity : Data->RemoveWhenNums)
	{
		if (entity.RemoveWhenNum.Y >= entity.RemoveWhenNum.X)
		{
			if (CountNum >= entity.RemoveWhenNum.X && (entity.RemoveWhenNum.Y < 0 || CountNum <= entity.RemoveWhenNum.Y))
			{
				Deactivate();
				AE->TimeToDie();
				return;
			}
		}
	}
}

void CounterEffect::AddSelfToManager()
{
	if (_started && !_pause && AE->AEManager)
	{
		AE->AEManager->AddCounter(AEData, this);
		// Debug::Log("CounterEffect::AddSelfToManager(), Mark: %s, AE: %s\n", AEData.Counter.Mark.c_str(), AEData.Name.c_str());
	}
}

void CounterEffect::RemoveSelfFromManager()
{
	if (AE->AEManager)
	{
		AE->AEManager->RemoveCounter(AEData);
	}
}

void CounterEffect::OnStart()
{
	ResetNum();
	// 向AE管理器注册自己
	AddSelfToManager();
}

void CounterEffect::OnPause()
{
	// 向AE管理器注销自己
	RemoveSelfFromManager();
}

void CounterEffect::OnRecover()
{
	// 向AE管理器注册自己
	AddSelfToManager();
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

void CounterEffect::ModifyCount(CounterAction action, int num)
{
	switch (action)
	{
	case CounterAction::INIT:
		CountNum = num;
		break;
	case CounterAction::ADD:
		CountNum += num;
		break;
	case CounterAction::SUB:
		CountNum -= num;
		break;
	case CounterAction::MUL:
		CountNum *= num;
		break;
	}

	if (CountNum < Data->Min)
	{
		CountNum = Data->Min;
	}
	if (Data->Max >= 0 && CountNum > Data->Max)
	{
		CountNum = Data->Max;
	}
}

void CounterEffect::ResetNum()
{
	double num = Data->Num;
	if (Data->NumType != CounterType::Number)
	{
		ObjectClass* pFrom = Data->NumFromSource ? AE->pSource : pObject;
		if (pFrom && !IsDeadOrInvisible(pFrom))
		{
			switch (Data->NumType)
			{
			case CounterType::HP:
				num = pFrom->Health;
				break;
			case CounterType::MaxHP:
				if (Data->NumFromSource)
				{
					num = pFrom->GetType()->Strength;
				}
				else
				{
					num = IsBullet() ? pFrom->Health : pFrom->GetType()->Strength;
				}
				break;
			}
		}
	}
	CountNum = num;
}

void CounterEffect::RemoveCounter()
{
	RemoveSelfFromManager();
	Deactivate();
	AE->TimeToDie();
}
