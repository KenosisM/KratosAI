#include "CountTriggerEffect.h"

#include <Ext/Helper/Finder.h>
#include <Ext/Helper/FLH.h>
#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Status.h>

#include "CounterEffect.h"

bool CountTriggerEffect::CanActive(int num, Point2D range)
{
	if (range.Y >= range.X)
	{
		return num >= range.X && (range.Y < 0 || num <= range.Y);
	}
	return true;
}

void CountTriggerEffect::Watch()
{
	// 向AE管理器查找计数器
	CounterEffect* counter = AE->AEManager->Counters[Data->Mark];
	if (counter && counter->IsAlive())
	{
		int action_idx = 0;
		for (CountTriggerEntity& entity : Data->Actions)
		{
			action_idx++;
			if (CanActive(counter->CountNum, entity.Range))
			{
				// 移除计数器
				if (entity.RemoveCounter)
				{
					counter->RemoveCounter();
					return;
				}
				// 操作计数
				if (entity.Num != 0)
				{
					counter->ModifyCount(entity.Action, entity.Num);
				}
				if (entity.ResetNum)
				{
					counter->ResetNum();
				}

				// 添加AE
				if (entity.Attach)
				{
					AttachEffect* aeManager = AE->AEManager;
					TechnoClass* pSource = AE->pSource;
					HouseClass* pSourceHouse = AE->pSourceHouse;
					if (entity.AttachToSource)
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
					else if (entity.AttachToCounterSource)
					{
						aeManager = GetAEManager<TechnoExt>(counter->AE->pSource);
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
					else if (entity.AttachFromCounterSource)
					{
						pSource = counter->AE->pSource;
						pSourceHouse = counter->AE->pSourceHouse;
					}
					if (aeManager)
					{
						aeManager->Attach(entity.AttachEffects, entity.AttachChances, false, pSource, pSourceHouse);
					}
				}
				// 移除AE
				if (entity.Remove)
				{
					AttachEffect* aeManager = AE->AEManager;
					if (entity.RemoveToSource)
					{
						aeManager = GetAEManager<TechnoExt>(AE->pSource);
					}
					else if (entity.RemoveToCounterSource)
					{
						aeManager = GetAEManager<TechnoExt>(counter->AE->pSource);
					}
					if (aeManager)
					{
						if (!entity.RemoveEffects.empty())
						{
							if (!entity.RemoveEffectsLevel.empty())
							{
								// 移除指定的层数
								std::map<std::string, int> aeTypes;
								int idx = 0;
								int count = entity.RemoveEffects.size();
								for (std::string removeAE : entity.RemoveEffects)
								{
									int level = -1;
									if (idx < count)
									{
										level = entity.RemoveEffectsLevel[idx];
									}
									if (level > 0)
									{
										aeTypes[removeAE] = level;
									}
								}
								if (!aeTypes.empty())
								{
									aeManager->DetachByName(aeTypes, entity.RemoveEffectsSkipNext);
								}
							}
							else
							{
								aeManager->DetachByName(entity.RemoveEffects, entity.RemoveEffectsSkipNext);
							}
						}
						if (!entity.RemoveEffectsWithMarks.empty())
						{
							aeManager->DetachByMarks(entity.RemoveEffectsWithMarks, entity.RemoveEffectsSkipNext);
						}
					}
				}
				if (entity.TriggeredTimes > 0)
				{
					// 触发成功，计数
					_count[action_idx]++;
					// 检查触发次数
					if (_count[action_idx] >= entity.TriggeredTimes)
					{
						Deactivate();
						AE->TimeToDie();
						return;
					}
				}
			}
		}

	}

}

void CountTriggerEffect::OnUpdate()
{
	if (!AE->OwnerIsDead())
	{
		Watch();
	}
}

void CountTriggerEffect::OnWarpUpdate()
{
	if (!AE->OwnerIsDead())
	{
		Watch();
	}
}
