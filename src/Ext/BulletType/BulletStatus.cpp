#include "BulletStatus.h"

#include <fstream>
#include <Extension/WarheadTypeExt.h>

#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Physics.h>
#include <Ext/Helper/FLH.h>

#include <Ext/BulletType/Trajectory/ArcingTrajectory.h>
#include <Ext/BulletType/Trajectory/MissileTrajectory.h>
#include <Ext/BulletType/Trajectory/StraightTrajectory.h>

#include "Bounce.h"

#include <Ext/ObjectType/AttachEffect.h>
#include <Ext/EffectType/Effect/VectorEffect.h>
#include <Ext/TechnoType/TechnoStatus.h>
#include <Ext/TechnoType/DecoyMissile.h>

void BulletStatus::OnTechnoDelete(EventSystem* sender, Event e, void* args)
{
	if (args == pSource)
	{
		pSource = nullptr;
	}
	if (args == _pFakeTarget)
	{
		_pFakeTarget = nullptr;
	}
	if (args == _pBlackHole)
	{
		BlackHoleCancel();
	}
}

AttachEffect* BulletStatus::AEManager()
{
	AttachEffect* aeManager = nullptr;
	if (_parent)
	{
		aeManager = _parent->GetComponent<AttachEffect>();
	}
	return aeManager;
}

void BulletStatus::InitState()
{
	AttachState();

	// 根据类型分配弹道控制
	switch (GetBulletType())
	{
	case BulletType::ARCING:
		FindOrAttach<ArcingTrajectory>();
		FindOrAttach<Bounce>();
		break;
	case BulletType::MISSILE:
		FindOrAttach<MissileTrajectory>();
		break;
	case BulletType::ROCKET:
		FindOrAttach<StraightTrajectory>();
		break;
	}
}

void BulletStatus::SetFakeTarget(ObjectClass* pFakeTarget)
{
	_pFakeTarget = pFakeTarget;
}

void BulletStatus::Awake()
{
	EventSystems::General.AddHandler(Events::ObjectUnInitEvent, this, &BulletStatus::OnTechnoDelete);

	Tag = pBullet->GetType()->Name;

	pSource = pBullet->Owner;
	if (pSource)
	{
		pSourceHouse = pSource->Owner;
	}
	// 读取生命值
	int health = pBullet->Health;
	// 抛射体武器伤害为复述或者零时需要处理
	if (health < 0)
	{
		health = -health;
	}
	else if (health == 0)
	{
		health = 1; // 武器伤害为0，如[NukeCarrier]
	}

	INIBufferReader* reader = INI::GetSection(INI::Rules, pBullet->GetType()->ID);
	// 初始化生命
	this->life.Health = health;
	this->life.Read(reader);
	// 初始化伤害
	this->damage.Damage = health;
	if (TechnoStatus* sourceStatue = GetStatus<TechnoExt, TechnoStatus>(pSource))
	{
		if (sourceStatue->AntiBullet->IsAlive())
		{
			damage.Eliminate = sourceStatue->AntiBullet->Data.OneShotOneKill;
			damage.Harmless = sourceStatue->AntiBullet->Data.Harmless;
		}
	}

	// 初始化地面碰撞属性
	switch (trajectoryData->SubjectToGround)
	{
	case SubjectToGroundType::YES:
		this->SubjectToGround = true;
		break;
	case SubjectToGroundType::NO:
		this->SubjectToGround = false;
		break;
	default:
		this->SubjectToGround = !IsArcing() && !IsRocket() && !trajectoryData->IsStraight();
		break;
	}

	// 初始化状态机
	InitState();
}

void BulletStatus::Destroy()
{
	EventSystems::General.RemoveHandler(Events::ObjectUnInitEvent, this, &BulletStatus::OnTechnoDelete);
}

void BulletStatus::TakeDamage(int damage, bool eliminate, bool harmless, bool checkInterceptable)
{
	if (!checkInterceptable || life.Interceptable)
	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "TakeDamage: damage=" << damage << " eliminate=" << eliminate << " health=" << life.Health << "\n";
		f.close();
		if (eliminate)
		{
			life.Detonate(harmless);
		}
		else
		{
			life.TakeDamage(damage, harmless);
		}
	}
}

void BulletStatus::TakeDamage(BulletDamage damageData, bool checkInterceptable)
{
	TakeDamage(damageData.Damage, damageData.Eliminate, damageData.Harmless, checkInterceptable);
}

void BulletStatus::ResetTarget(AbstractClass* pNewTarget, CoordStruct targetPos)
{
	pBullet->SetTarget(pNewTarget);
	if (targetPos == CoordStruct::Empty && pNewTarget)
	{
		targetPos = pNewTarget->GetCoords();
	}
	pBullet->TargetCoords = targetPos;
	// 重设弹道
	switch (GetBulletType())
	{
	case BulletType::ARCING:
		if (ArcingTrajectory* at = GetComponent<ArcingTrajectory>())
		{
			at->ResetTarget(pNewTarget, targetPos);
		}
		break;
	case BulletType::ROCKET:
		if (StraightTrajectory* st = GetComponent<StraightTrajectory>())
		{
			st->ResetTarget(pNewTarget, targetPos);
		}
	}
}

void BulletStatus::OnPut(CoordStruct* pLocation, DirType dir)
{
	if (!_initFlag)
	{
		_initFlag = true;
		InitState_BlackHole();
		// InitState_Bounce();
		InitState_Proximity();
		// // 弹道初始化
		// if (IsMissile())
		// {
		// 	InitState_ECM();
		// 	InitState_Trajectory_Missile();
		// }
		// else if (IsRocket())
		// {
		// 	InitState_Trajectory_Straight();
		// }
	}

}

void BulletStatus::InitState_BlackHole() {};
void BulletStatus::InitState_ECM() {};

void BulletStatus::OnUpdate()
{

	// 自毁
	OnUpdate_DestroySelf();

	CoordStruct location = pBullet->GetCoords();

	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "OnUpdate: VectorForced=" << VectorForced
			<< " Speed=" << pBullet->Speed
			<< " IsDetonate=" << life.IsDetonate
			<< " health=" << life.Health
			<< " Vel=(" << pBullet->Velocity.X << "," << pBullet->Velocity.Y << "," << pBullet->Velocity.Z << ")"
			<< " Pos=(" << location.X << "," << location.Y << "," << location.Z << ")"
			<< "\n";
		f.close();
	}
	// 潜地
	if (!life.IsDetonate && !HasPreImpactAnim(pBullet->WH))
	{
		// Vector Force+Freeze 时跳过潜地检测
		if (VectorForced && !life.IsDetonate)
		{
			// 检查是否有 Freeze VectorEffect
			bool hasFreezeVec = false;
			if (AttachEffect* aem = AEManager())
			{
				aem->ForeachChild([&](Component* c)
				{
					if (auto* aes = dynamic_cast<AttachEffectScript*>(c))
					{
						aes->ForeachChild([&](Component* c2)
						{
							if (auto* ve = dynamic_cast<VectorEffect*>(c2))
							{
								if (ve->IsActive() && ve->Data && ve->Data->Freeze)
									hasFreezeVec = true;
							}
						});
					}
				});
			}
			if (hasFreezeVec)
			{
				// Freeze模式：不碰Speed/Velocity，让引擎正常计算弹道，
				// 在OnUpdateEnd通过SetLocation暴力拉回
				return;
			}
		}

		if ((SubjectToGround || GetComponent<Bounce>()) && pBullet->GetHeight() < 0)
		{
			// 抛射体潜入地下，重新设置目标参数，并手动引爆
			std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
			f << "DETONATE: underground, height=" << pBullet->GetHeight() << " pos=(" << location.X << "," << location.Y << "," << location.Z << ")\n";
			f.close();
			CoordStruct targetPos = location;
			if (CellClass* pTargetCell = MapClass::Instance->TryGetCellAt(location))
			{
				targetPos.Z = pTargetCell->GetCoordsWithBridge().Z;
				pBullet->SetTarget(pTargetCell);
			}
			pBullet->TargetCoords = targetPos;
			life.Detonate();
		}
		if (!life.IsDetonate && IsArcing() && pBullet->GetHeight() <= 8)
		{
			// Arcing 近炸
			CoordStruct tempSourcePos = location;
			tempSourcePos.Z = 0;
			CoordStruct tempTargetPos = pBullet->TargetCoords;
			tempTargetPos.Z = 0;
			if (tempSourcePos.DistanceFrom(tempTargetPos) <= static_cast<double>(256 + pBullet->Type->Acceleration))
			{
				std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
				f << "DETONATE: arcing near ground, height=" << pBullet->GetHeight() << " dist=" << tempSourcePos.DistanceFrom(tempTargetPos) << "\n";
				f.close();
				life.Detonate();
			}
		}
	}

	// 生命检查
	if (life.IsDetonate)
	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "DETONATE: life.IsDetonate=true, health=" << life.Health << " pos=(" << location.X << "," << location.Y << "," << location.Z << ")\n";
		f.close();
		if (!life.IsHarmless)
		{
			pBullet->Detonate(location);
		}
		pBullet->UnInit();
		//重要，击杀自己后中断所有后续循环
		Break();
		return;
	}
	if (life.Health <= 0)
	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "DETONATE: health<=0, health=" << life.Health << "\n";
		f.close();
		life.IsDetonate = true;
	}
	// 其他逻辑
	if (!IsDeadOrInvisible(pBullet, this))
	{
		OnUpdate_BlackHole();
		OnUpdate_Vector();
		OnUpdate_Paintball();
		OnUpdate_RecalculateStatus();
		OnUpdate_SelfLaunchOrPumpAction();
		OnUpdate_GiftBox(); // 礼盒会删除对象，所以要放最后
	}
}

void BulletStatus::OnUpdateEnd()
{
	if (!IsDeadOrInvisible(pBullet, this))
	{
		CoordStruct location = pBullet->GetCoords();
		OnUpdateEnd_BlackHole(location); // 黑洞会更新位置，要第一个执行
		OnUpdateEnd_Vector(location);
		OnUpdateEnd_Proximity(location);
	}
};


void BulletStatus::OnDetonate(CoordStruct* pCoords, bool& skip)
{
	ObjectClass* pTemp = _pFakeTarget;
	_pFakeTarget = nullptr;
	if (pTemp)
	{
		pTemp->UnInit();
	}
	if (!skip)
	{
		if ((skip = OnDetonate_AntiBullet(pCoords)) == true)
		{
			return;
		}
		if ((skip = OnDetonate_GiftBox(pCoords)) == true)
		{
			return;
		}
		if ((skip = OnDetonate_SelfLaunch(pCoords)) == true)
		{
			return;
		}
	}
};

void BulletStatus::OnUpdate_Vector()
{
	if (!VectorForced)
		return;

	// 被黑洞捕获时，停止 Vector 执行
	if (CaptureByBlackHole)
		return;

	AttachEffect* aem = AEManager();
	if (!aem)
		return;

	// 检查是否有 Freeze，有则不设 Velocity（Freeze 在 OnUpdateEnd 处理）
	bool hasFreeze = false;
	aem->ForeachChild([&](Component* c)
	{
		if (auto* aes = dynamic_cast<AttachEffectScript*>(c))
		{
			aes->ForeachChild([&](Component* c2)
			{
				if (auto* ve = dynamic_cast<VectorEffect*>(c2))
				{
					if (ve->IsActive() && ve->Data && ve->Data->Freeze)
						hasFreeze = true;
				}
			});
		}
	});
	if (hasFreeze)
		return;

	CoordStruct currentPos = pBullet->GetCoords();
	CoordStruct totalDisplacement{};

	aem->ForeachChild([&](Component* c)
	{
		if (auto* aes = dynamic_cast<AttachEffectScript*>(c))
		{
			aes->ForeachChild([&](Component* c2)
			{
				if (auto* ve = dynamic_cast<VectorEffect*>(c2))
				{
					if (!ve->IsActive() || !ve->Data || ve->Data->Freeze)
						return;

					CoordStruct originPos = currentPos;
					switch (ve->Data->Origin)
					{
					case VectorData::VectorOrigin::World:
					case VectorData::VectorOrigin::Realtime:
						originPos = currentPos;
						break;
					case VectorData::VectorOrigin::Target:
						originPos = ve->Data->OriginNoUpdate ? ve->_initialOriginPos : ve->_initialTarget;
						break;
					case VectorData::VectorOrigin::Launcher:
						if (ve->Data->OriginNoUpdate)
							originPos = ve->_initialOriginPos;
						else if (ve->_pLauncher && !IsDeadOrInvisible(ve->_pLauncher))
							originPos = ve->_pLauncher->GetCoords();
						break;
					case VectorData::VectorOrigin::Source:
						if (ve->Data->OriginNoUpdate)
							originPos = ve->_initialOriginPos;
						else if (ve->_pSource && !IsDeadOrInvisible(ve->_pSource))
							originPos = ve->_pSource->GetCoords();
						break;
					case VectorData::VectorOrigin::Self:
						originPos = ve->_initialLocation;
						break;
					case VectorData::VectorOrigin::FLH:
						originPos = ve->_initialOriginPos;
						break;
					default:
						break;
					}

					CoordStruct frameTarget;
					frameTarget.X = originPos.X + ve->Data->TargetFLH.X + ve->_randomTargetOffset.X;
					frameTarget.Y = originPos.Y + ve->Data->TargetFLH.Y + ve->_randomTargetOffset.Y;
					frameTarget.Z = originPos.Z + ve->Data->TargetFLH.Z + ve->_randomTargetOffset.Z;

					int speed = ve->_currentSpeed;

					// MoveTo FLH 位移
					CoordStruct moveFlh{};
					moveFlh.X = ve->Data->MoveTo.X + static_cast<int>(ve->Data->GrowRate.X * ve->_moveFrame);
					moveFlh.Y = ve->Data->MoveTo.Y + static_cast<int>(ve->Data->GrowRate.Y * ve->_moveFrame);
					moveFlh.Z = ve->Data->MoveTo.Z + static_cast<int>(ve->Data->GrowRate.Z * ve->_moveFrame);

					// FLH → 世界坐标（只用速度XY分量算朝向，Z轴始终垂直地面）
					CoordStruct moveDisp;
					if (ve->Data->Origin != VectorData::VectorOrigin::World)
					{
						double vx = pBullet->Velocity.X;
						double vy = pBullet->Velocity.Y;
						double len = std::sqrt(vx * vx + vy * vy);
						if (len > 1e-6)
						{
							double fwdX = vx / len;
							double fwdY = vy / len;
							moveDisp.X = static_cast<int>(moveFlh.X * fwdX - moveFlh.Y * fwdY);
							moveDisp.Y = static_cast<int>(moveFlh.X * fwdY + moveFlh.Y * fwdX);
							moveDisp.Z = moveFlh.Z;
						}
						else
						{
							moveDisp = moveFlh;
						}
					}
					else
					{
						moveDisp = moveFlh;
					}

					{
						std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
						f << "OnUpdateVec: facingRaw=" << ve->_initialFacing.Raw
							<< " moveFlh=(" << moveFlh.X << "," << moveFlh.Y << "," << moveFlh.Z << ")"
							<< " moveDisp=(" << moveDisp.X << "," << moveDisp.Y << "," << moveDisp.Z << ")"
							<< " Pos=(" << currentPos.X << "," << currentPos.Y << "," << currentPos.Z << ")"
							<< "\n";
						f.close();
					}

					// 朝目标飞行 + MoveTo 偏移
					CoordStruct nextPos;
					if (!frameTarget.IsEmpty() && (ve->Data->TargetFLH.X != 0 || ve->Data->TargetFLH.Y != 0 || ve->Data->TargetFLH.Z != 0))
					{
						CoordStruct dirVec;
						dirVec.X = frameTarget.X - currentPos.X;
						dirVec.Y = frameTarget.Y - currentPos.Y;
						dirVec.Z = frameTarget.Z - currentPos.Z;
						double dirLen = std::sqrt(static_cast<double>(dirVec.X * dirVec.X + dirVec.Y * dirVec.Y + dirVec.Z * dirVec.Z));
						if (dirLen > 1e-6)
						{
							nextPos.X = currentPos.X + static_cast<int>(dirVec.X / dirLen * speed) + moveDisp.X;
							nextPos.Y = currentPos.Y + static_cast<int>(dirVec.Y / dirLen * speed) + moveDisp.Y;
							nextPos.Z = currentPos.Z + static_cast<int>(dirVec.Z / dirLen * speed) + moveDisp.Z;
						}
						else
						{
							nextPos.X = currentPos.X + moveDisp.X;
							nextPos.Y = currentPos.Y + moveDisp.Y;
							nextPos.Z = currentPos.Z + moveDisp.Z;
						}
					}
					else
					{
						nextPos.X = currentPos.X + moveDisp.X;
						nextPos.Y = currentPos.Y + moveDisp.Y;
						nextPos.Z = currentPos.Z + moveDisp.Z;
					}

					totalDisplacement.X += nextPos.X - currentPos.X;
					totalDisplacement.Y += nextPos.Y - currentPos.Y;
					totalDisplacement.Z += nextPos.Z - currentPos.Z;
				}
			});
		}
	});

	if (!totalDisplacement.IsEmpty())
	{
		// 设置 Velocity = 位移方向，让引擎协作移动抛射体
		pBullet->Velocity.X = static_cast<double>(totalDisplacement.X);
		pBullet->Velocity.Y = static_cast<double>(totalDisplacement.Y);
		pBullet->Velocity.Z = static_cast<double>(totalDisplacement.Z);
	}

	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "SetVel: Vel=(" << pBullet->Velocity.X << "," << pBullet->Velocity.Y << "," << pBullet->Velocity.Z << ")"
			<< " Speed=" << pBullet->Speed
			<< " Pos=(" << pBullet->GetCoords().X << "," << pBullet->GetCoords().Y << "," << pBullet->GetCoords().Z << ")"
			<< "\n";
		f.close();
	}
}

void BulletStatus::OnUpdateEnd_Vector(CoordStruct& sourcePos)
{
	// 被黑洞捕获时，停止 Vector 执行
	if (CaptureByBlackHole)
		return;

	AttachEffect* aem = AEManager();
	if (!aem)
		return;

	bool anyActiveVector = false;
	bool anyFreeze = false;
	CoordStruct frozenPos = CoordStruct::Empty;
	aem->ForeachChild([&](Component* c)
	{
		if (auto* aes = dynamic_cast<AttachEffectScript*>(c))
		{
			aes->ForeachChild([&](Component* c2)
			{
				if (auto* ve = dynamic_cast<VectorEffect*>(c2))
				{
					if (ve->IsActive() && ve->Data)
					{
						anyActiveVector = true;
						if (ve->Data->Freeze)
						{
							anyFreeze = true;
							// 取第一个 Freeze VectorEffect 的初始位置作为冻结锚点
							if (frozenPos.IsEmpty())
								frozenPos = ve->_initialLocation;
						}
					}
				}
			});
		}
	});

	if (!anyActiveVector)
		return;

	// Freeze + Force: 强制把抛射体拉回冻结位置（Freeze 场景仍需 SetLocation）
	if (anyFreeze && VectorForced && !frozenPos.IsEmpty())
	{
		{
			std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
			f << "FreezePull: frozenPos=(" << frozenPos.X << "," << frozenPos.Y << "," << frozenPos.Z << ")"
				<< " curPos=(" << pBullet->GetCoords().X << "," << pBullet->GetCoords().Y << "," << pBullet->GetCoords().Z << ")"
				<< " Vel=(" << pBullet->Velocity.X << "," << pBullet->Velocity.Y << "," << pBullet->Velocity.Z << ")"
				<< " Speed=" << pBullet->Speed
				<< "\n";
			f.close();
		}
		pBullet->SetLocation(frozenPos);
		pBullet->SourceCoords = frozenPos;
		sourcePos = frozenPos;
		return;
	}

	// Force 非 Freeze：Velocity 已在 OnUpdate_Vector 中设置，引擎已移动抛射体
	// 此处只做碰撞检测
	if (VectorForced)
	{
		if (pBullet->GetHeight() <= 0)
		{
			std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
			f << "DETONATE: Force underground, height=" << pBullet->GetHeight() << " pos=(" << pBullet->GetCoords().X << "," << pBullet->GetCoords().Y << "," << pBullet->GetCoords().Z << ")\n";
			f.close();
			TakeDamage();
			return;
		}
		sourcePos = pBullet->GetCoords();
		return;
	}

	CoordStruct currentPos = pBullet->GetCoords();
	CoordStruct totalDisplacement{};

	aem->ForeachChild([&](Component* c)
	{
		if (auto* aes = dynamic_cast<AttachEffectScript*>(c))
		{
			aes->ForeachChild([&](Component* c2)
			{
				if (auto* ve = dynamic_cast<VectorEffect*>(c2))
				{
					if (!ve->IsActive() || !ve->Data)
						return;

					if (ve->Data->Freeze)
						return;

					CoordStruct originPos = currentPos;

					// 计算 Origin 位置
					switch (ve->Data->Origin)
					{
					case VectorData::VectorOrigin::World:
					case VectorData::VectorOrigin::Realtime:
						originPos = currentPos;
						break;
					case VectorData::VectorOrigin::Target:
						originPos = ve->Data->OriginNoUpdate ? ve->_initialOriginPos : ve->_initialTarget;
						break;
					case VectorData::VectorOrigin::Launcher:
						if (ve->Data->OriginNoUpdate)
							originPos = ve->_initialOriginPos;
						else if (ve->_pLauncher && !IsDeadOrInvisible(ve->_pLauncher))
							originPos = ve->_pLauncher->GetCoords();
						break;
					case VectorData::VectorOrigin::Source:
						if (ve->Data->OriginNoUpdate)
							originPos = ve->_initialOriginPos;
						else if (ve->_pSource && !IsDeadOrInvisible(ve->_pSource))
							originPos = ve->_pSource->GetCoords();
						break;
					case VectorData::VectorOrigin::Self:
						originPos = ve->_initialLocation;
						break;
					case VectorData::VectorOrigin::FLH:
						originPos = ve->_initialOriginPos;
						break;
					default:
						break;
					}
					CoordStruct randOff = ve->_randomTargetOffset;

					CoordStruct frameTarget;
					frameTarget.X = originPos.X + ve->Data->TargetFLH.X + randOff.X;
					frameTarget.Y = originPos.Y + ve->Data->TargetFLH.Y + randOff.Y;
					frameTarget.Z = originPos.Z + ve->Data->TargetFLH.Z + randOff.Z;

					int speed = ve->_currentSpeed;

					CoordStruct moveFlh{};
					moveFlh.X = ve->Data->MoveTo.X + static_cast<int>(ve->Data->GrowRate.X * ve->_moveFrame);
					moveFlh.Y = ve->Data->MoveTo.Y + static_cast<int>(ve->Data->GrowRate.Y * ve->_moveFrame);
					moveFlh.Z = ve->Data->MoveTo.Z + static_cast<int>(ve->Data->GrowRate.Z * ve->_moveFrame);

					// FLH → 世界坐标（只用速度XY分量算朝向，Z轴始终垂直地面）
					CoordStruct moveDisp;
					if (ve->Data->Origin != VectorData::VectorOrigin::World)
					{
						double vx = pBullet->Velocity.X;
						double vy = pBullet->Velocity.Y;
						double len = std::sqrt(vx * vx + vy * vy);
						if (len > 1e-6)
						{
							double fwdX = vx / len;
							double fwdY = vy / len;
							moveDisp.X = static_cast<int>(moveFlh.X * fwdX - moveFlh.Y * fwdY);
							moveDisp.Y = static_cast<int>(moveFlh.X * fwdY + moveFlh.Y * fwdX);
							moveDisp.Z = moveFlh.Z;
						}
						else
						{
							moveDisp = moveFlh;
						}
					}
					else
					{
						moveDisp = moveFlh;
					}

					CoordStruct nextPos;
					if (!frameTarget.IsEmpty() && (ve->Data->TargetFLH.X != 0 || ve->Data->TargetFLH.Y != 0 || ve->Data->TargetFLH.Z != 0))
					{
						CoordStruct dirVec;
						dirVec.X = frameTarget.X - currentPos.X;
						dirVec.Y = frameTarget.Y - currentPos.Y;
						dirVec.Z = frameTarget.Z - currentPos.Z;
						double dirLen = std::sqrt(static_cast<double>(dirVec.X * dirVec.X + dirVec.Y * dirVec.Y + dirVec.Z * dirVec.Z));
						if (dirLen > 1e-6)
						{
							nextPos.X = currentPos.X + static_cast<int>(dirVec.X / dirLen * speed) + moveDisp.X;
							nextPos.Y = currentPos.Y + static_cast<int>(dirVec.Y / dirLen * speed) + moveDisp.Y;
							nextPos.Z = currentPos.Z + static_cast<int>(dirVec.Z / dirLen * speed) + moveDisp.Z;
						}
						else
						{
							nextPos.X = currentPos.X + moveDisp.X;
							nextPos.Y = currentPos.Y + moveDisp.Y;
							nextPos.Z = currentPos.Z + moveDisp.Z;
						}
					}
					else
					{
						nextPos.X = currentPos.X + moveDisp.X;
						nextPos.Y = currentPos.Y + moveDisp.Y;
						nextPos.Z = currentPos.Z + moveDisp.Z;
					}

					totalDisplacement.X += nextPos.X - currentPos.X;
					totalDisplacement.Y += nextPos.Y - currentPos.Y;
					totalDisplacement.Z += nextPos.Z - currentPos.Z;
				}
			});
		}
	});

	if (totalDisplacement.IsEmpty())
		return;

	CoordStruct finalPos;
	finalPos.X = currentPos.X + totalDisplacement.X;
	finalPos.Y = currentPos.Y + totalDisplacement.Y;
	finalPos.Z = currentPos.Z + totalDisplacement.Z;

	if (pBullet->GetHeight() <= 0)
	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "DETONATE: non-Force underground, height=" << pBullet->GetHeight() << "\n";
		f.close();
		TakeDamage();
		return;
	}
	CoordStruct nextCellPos = CoordStruct::Empty;
	bool onBridge = false;
	PassError passError = CanMoveTo(currentPos, finalPos, false, nextCellPos, onBridge);
	if (passError != PassError::PASS && passError != PassError::NONE)
	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "DETONATE: collision, passError=" << static_cast<int>(passError) << "\n";
		f.close();
		TakeDamage();
		return;
	}

	pBullet->SetLocation(finalPos);
	pBullet->SourceCoords = finalPos;
	sourcePos = finalPos;
}
