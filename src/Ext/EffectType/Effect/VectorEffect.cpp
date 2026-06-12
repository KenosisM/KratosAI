#include "VectorEffect.h"

#include <fstream>
#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Status.h>
#include <Ext/Helper/Physics.h>
#include <Ext/Helper/Weapon.h>
#include <Ext/Helper/FLH.h>

#include <Ext/ObjectType/AttachEffect.h>
#include <Ext/EffectType/AttachEffectScript.h>
#include <Ext/BulletType/BulletStatus.h>

static bool HasAnyActiveVectorEffect(GameObject* pObj)
{
	bool found = false;
	if (pObj)
	{
		pObj->Foreach([&found](Component* c)
		{
			if (auto* ve = dynamic_cast<VectorEffect*>(c))
			{
				if (ve->IsActive())
				{
					found = true;
				}
			}
		});
	}
	return found;
}

void VectorEffect::OnStart()
{
	if (pTechno && pTechno->WhatAmI() == AbstractType::Building)
	{
		Deactivate();
		return;
	}

	_initialLocation = pObject->GetCoords();
	_elapsedFrames = 0;
	_moveFrame = 0;
	_currentAngle = 0;
	_currentWaveFrequency = Data->WaveFrequency;
	_currentSpinRate = Data->SpinRate;
	_currentTurretSpinRate = Data->TurretSpinRate;

	if (AE && AE->AEManager)
	{
		if (Data->Force && pBullet)
		{
			if (BulletStatus* status = GetStatus<BulletExt, BulletStatus>(pBullet))
			{
				status->VectorForced = true;
			}
		}
	}

	{
		std::ofstream f("I:\\kratosAI\\vector_debug.log", std::ios::app);
		f << "OnStart: id=" << pObject->GetType()->ID
			<< " Force=" << Data->Force << " Freeze=" << Data->Freeze
			<< " Speed=" << _currentSpeed
			<< " Pos=(" << _initialLocation.X << "," << _initialLocation.Y << "," << _initialLocation.Z << ")"
			<< " Vel=(" << pBullet->Velocity.X << "," << pBullet->Velocity.Y << "," << pBullet->Velocity.Z << ")"
			<< "\n";
		f.close();
	}

	if (Data->TargetRandF.X != 0 || Data->TargetRandF.Y != 0
		|| Data->TargetRandL.X != 0 || Data->TargetRandL.Y != 0
		|| Data->TargetRandH.X != 0 || Data->TargetRandH.Y != 0)
	{
		_randomTargetOffset.X = Random::RandomRanged(Data->TargetRandF.X, Data->TargetRandF.Y);
		_randomTargetOffset.Y = Random::RandomRanged(Data->TargetRandL.X, Data->TargetRandL.Y);
		_randomTargetOffset.Z = Random::RandomRanged(Data->TargetRandH.X, Data->TargetRandH.Y);
	}

	if (pTechno && !IsAircraft())
	{
		CoordStruct pos = pTechno->GetCoords();
		pTechno->UpdatePlacement(PlacementType::Remove);
		pTechno->UnmarkAllOccupationBits(pos);
		FootClass* pFoot = abstract_cast<FootClass*, true>(pTechno);
		if (pFoot)
		{
			ForceStopMoving(pFoot);
		}
	}

	if (pTechno)
	{
		_initialFacing = pTechno->PrimaryFacing.Current();

		if (Data->StartSpeed > 0)
		{
			_currentSpeed = Data->StartSpeed;
		}
		else
		{
			TechnoTypeClass* pType = pTechno->GetTechnoType();
			if (GetLocoType(pTechno) == LocoType::Jumpjet)
			{
				_currentSpeed = pType->JumpjetSpeed;
			}
			else
			{
				_currentSpeed = pType->Speed;
			}
			if (_currentSpeed <= 0)
			{
				_currentSpeed = pType->Speed;
			}
		}
	}
	else if (pBullet)
	{
		// 从抛射体 XY 速度分量计算面朝方向（North=0, East=16384, South=32768, West=49152）
		_initialFacing = DirStruct(std::atan2(pBullet->Velocity.X, pBullet->Velocity.Y));

		if (Data->StartSpeed > 0)
		{
			_currentSpeed = Data->StartSpeed;
		}
		else
		{
			_currentSpeed = pBullet->Speed;
		}
	}

	// 保存初始 Origin 位置（NoUpdate 模式用）
	if (Data->OriginNoUpdate)
	{
		switch (Data->Origin)
		{
		case VectorData::VectorOrigin::Target:
			if (pTechno && pTechno->Target)
				_initialOriginPos = pTechno->Target->GetCoords();
			else if (pBullet)
				_initialOriginPos = pBullet->TargetCoords;
			break;
		case VectorData::VectorOrigin::Launcher:
			if (pBullet && pBullet->Owner)
				_initialOriginPos = pBullet->Owner->GetCoords();
			else if (pTechno)
				_initialOriginPos = pTechno->GetCoords();
			break;
		case VectorData::VectorOrigin::Source:
			if (AE && AE->pSource)
				_initialOriginPos = AE->pSource->GetCoords();
			break;
		default:
			break;
		}
	}

	switch (Data->Origin)
	{
	case VectorData::VectorOrigin::Target:
		if (pTechno && pTechno->Target)
		{
			_initialTarget = pTechno->Target->GetCoords();
		}
		else if (pBullet)
		{
			_initialTarget = pBullet->TargetCoords;
		}
		break;
	case VectorData::VectorOrigin::Launcher:
		if (pBullet)
		{
			_pLauncher = pBullet->Owner;
		}
		else if (pTechno)
		{
			_pLauncher = pTechno;
		}
		break;
	case VectorData::VectorOrigin::Source:
		if (AE && AE->pSource)
		{
			_pSource = AE->pSource;
		}
		break;
	case VectorData::VectorOrigin::FLH:
		if (pTechno)
		{
			CoordStruct unitPos = pTechno->GetCoords();
			DirStruct unitFacing = pTechno->PrimaryFacing.Current();
			_initialOriginPos = GetFLHAbsoluteCoords(unitPos, Data->OriginFLH, unitFacing);
		}
		break;
	default:
		break;
	}

#ifdef DEBUG_AE
	Debug::Log("Vector: [%s] Start, Pos=(%d,%d,%d), Speed=%d\n",
		pObject->GetType()->ID, _initialLocation.X, _initialLocation.Y, _initialLocation.Z, _currentSpeed);
#endif
}

void VectorEffect::End(CoordStruct location)
{
	if (Data->Force && pBullet)
	{
		if (BulletStatus* status = GetStatus<BulletExt, BulletStatus>(pBullet))
		{
			status->VectorForced = false;
		}
	}

	if (pObject && !HasAnyActiveVectorEffect(dynamic_cast<GameObject*>(pObject)))
	{
		if (pTechno && !IsBuilding() && !IsDeadOrInvisible(pTechno))
		{
			FootClass* pFoot = abstract_cast<FootClass*, true>(pTechno);
			if (pFoot)
			{
				pFoot->Locomotor->Unlock();
			}
			CoordStruct pos = pTechno->GetCoords();
			CellClass* pCell = MapClass::Instance->TryGetCellAt(pos);
			int groundZ = pCell ? pCell->GetCoordsWithBridge().Z : 0;
			if (pos.Z > groundZ)
			{
				FallingExceptAircraft(pTechno, 0, false);
			}
		}
	}

	if (pBullet && !IsDeadOrInvisible(pBullet))
	{
		pBullet->SourceCoords = pBullet->GetCoords();
		RecalculateBulletVelocity(pBullet);
	}

	EffectScript::End(location);
}

void VectorEffect::OnPause()
{
	if (pBullet && !IsDeadOrInvisible(pBullet))
	{
		pBullet->SourceCoords = pBullet->GetCoords();
		RecalculateBulletVelocity(pBullet);
	}
}

void VectorEffect::OnRecover()
{
	_initialLocation = pObject->GetCoords();
}
