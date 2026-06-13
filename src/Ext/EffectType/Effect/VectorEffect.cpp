#include "VectorEffect.h"

#include <Ext/Helper/Scripts.h>
#include <Ext/Helper/Status.h>
#include <Ext/Helper/Physics.h>
#include <Ext/Helper/Weapon.h>
#include <Ext/Helper/FLH.h>

#include <Ext/ObjectType/AttachEffect.h>
#include <Ext/EffectType/AttachEffectScript.h>
#include <Ext/BulletType/BulletStatus.h>

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

	if (Data->TargetRandF.X != 0 || Data->TargetRandF.Y != 0
		|| Data->TargetRandL.X != 0 || Data->TargetRandL.Y != 0
		|| Data->TargetRandH.X != 0 || Data->TargetRandH.Y != 0)
	{
		_randomTargetOffset.X = Random::RandomRanged(Data->TargetRandF.X, Data->TargetRandF.Y);
		_randomTargetOffset.Y = Random::RandomRanged(Data->TargetRandL.X, Data->TargetRandL.Y);
		_randomTargetOffset.Z = Random::RandomRanged(Data->TargetRandH.X, Data->TargetRandH.Y);
	}

	if (pTechno)
	{
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

VectorResult VectorEffect::GetVectorResult()
{
	VectorResult result;
	if (Data->Freeze)
	{
		result.Freeze = true;
		// 取第一个 Freeze VectorEffect 的初始位置作为冻结锚点
		if (result.FrozenPos.IsEmpty())
		{
			result.FrozenPos = _initialLocation;
		}
	}
	else
	{
		CoordStruct currentPos = pObject->GetCoords();
		CoordStruct originPos = currentPos; // 默认原点为当前位置，后续根据 Origin 类型调整
		switch (Data->Origin)
		{
		case VectorData::VectorOrigin::World:
		case VectorData::VectorOrigin::Realtime:
			break;
		case VectorData::VectorOrigin::Target:
			originPos = Data->OriginNoUpdate ? _initialOriginPos : _initialTarget;
			break;
		case VectorData::VectorOrigin::Launcher:
			if (Data->OriginNoUpdate)
				originPos = _initialOriginPos;
			else if (_pLauncher && !IsDeadOrInvisible(_pLauncher))
				originPos = _pLauncher->GetCoords();
			break;
		case VectorData::VectorOrigin::Source:
			if (Data->OriginNoUpdate)
				originPos = _initialOriginPos;
			else if (_pSource && !IsDeadOrInvisible(_pSource))
				originPos = _pSource->GetCoords();
			break;
		case VectorData::VectorOrigin::Self:
			originPos = _initialLocation;
			break;
		case VectorData::VectorOrigin::FLH:
			originPos = _initialOriginPos;
			break;
		default:
			break;
		}

		CoordStruct frameTargetFlh;
		frameTargetFlh.X = Data->TargetFLH.X + _randomTargetOffset.X;
		frameTargetFlh.Y = Data->TargetFLH.Y + _randomTargetOffset.Y;
		frameTargetFlh.Z = Data->TargetFLH.Z + _randomTargetOffset.Z;

		// TargetFLH → 世界坐标（与 MoveTo 一致）
		CoordStruct frameTargetDisp;
		if (Data->Origin == VectorData::VectorOrigin::World)
		{
			frameTargetDisp = frameTargetFlh;
		}
		else if (pBullet)
		{
			double vx = pBullet->Velocity.X;
			double vy = pBullet->Velocity.Y;
			double len = std::sqrt(vx * vx + vy * vy);
			if (len > 1e-6)
			{
				double fwdX = vx / len;
				double fwdY = vy / len;
				frameTargetDisp.X = static_cast<int>(frameTargetFlh.X * fwdX - frameTargetFlh.Y * fwdY);
				frameTargetDisp.Y = static_cast<int>(-(frameTargetFlh.X * fwdY + frameTargetFlh.Y * fwdX));
				frameTargetDisp.Z = frameTargetFlh.Z;
			}
			else
			{
				frameTargetDisp = frameTargetFlh;
			}
		}
		else if (pTechno)
		{
			DirStruct facing = pTechno->PrimaryFacing.Current();
			double rad = facing.GetRadian();
			double cosA = std::cos(rad);
			double sinA = std::sin(rad);
			frameTargetDisp.X = static_cast<int>(frameTargetFlh.X * cosA - frameTargetFlh.Y * sinA);
			frameTargetDisp.Y = static_cast<int>(-(frameTargetFlh.X * sinA + frameTargetFlh.Y * cosA));
			frameTargetDisp.Z = frameTargetFlh.Z;
		}
		else
		{
			frameTargetDisp = frameTargetFlh;
		}

		CoordStruct frameTarget;
		frameTarget.X = originPos.X + frameTargetDisp.X;
		frameTarget.Y = originPos.Y + frameTargetDisp.Y;
		frameTarget.Z = originPos.Z + frameTargetDisp.Z;

		bool hasTarget = Data->TargetFLH.X != 0 || Data->TargetFLH.Y != 0 || Data->TargetFLH.Z != 0;

		// --- 速度计算（加速度 + 振荡）---
		int speed = _currentSpeed;
		if (Data->Acceleration != 0)
		{
			speed += Data->Acceleration * _elapsedFrames;
		}
		if (Data->EndSpeed >= 0)
		{
			speed = std::clamp(speed, std::min(_currentSpeed, Data->EndSpeed), std::max(_currentSpeed, Data->EndSpeed));
		}
		if (Data->SpeedOscillation != 0 && Data->SpeedOscFreq != 0)
		{
			double phase = (_elapsedFrames * Data->SpeedOscFreq + Data->SpeedOscPhase) * M_PI / 180.0;
			speed += static_cast<int>(Data->SpeedOscillation * std::sin(phase));
		}
		if (speed <= 0) speed = 1;

		// --- MoveTo 振荡 ---
		CoordStruct growOsc{};
		if (Data->GrowOscFreqX != 0)
		{
			double phaseX = (_elapsedFrames * Data->GrowOscFreqX + Data->GrowOscPhaseX) * M_PI / 180.0;
			growOsc.X = static_cast<int>(Data->GrowOscillationX * std::sin(phaseX));
		}
		if (Data->GrowOscFreqY != 0)
		{
			double phaseY = (_elapsedFrames * Data->GrowOscFreqY + Data->GrowOscPhaseY) * M_PI / 180.0;
			growOsc.Y = static_cast<int>(Data->GrowOscillationY * std::sin(phaseY));
		}
		if (Data->GrowOscFreqZ != 0)
		{
			double phaseZ = (_elapsedFrames * Data->GrowOscFreqZ + Data->GrowOscPhaseZ) * M_PI / 180.0;
			growOsc.Z = static_cast<int>(Data->GrowOscillationZ * std::sin(phaseZ));
		}

		// --- MoveTo FLH 位移 ---
		CoordStruct moveFlh{};
		moveFlh.X = Data->MoveTo.X + static_cast<int>(Data->GrowRate.X * _moveFrame) + growOsc.X;
		moveFlh.Y = Data->MoveTo.Y + static_cast<int>(Data->GrowRate.Y * _moveFrame) + growOsc.Y;
		moveFlh.Z = Data->MoveTo.Z + static_cast<int>(Data->GrowRate.Z * _moveFrame) + growOsc.Z;

		// FLH → 世界坐标（与游戏引擎 GetFLHAbsoluteCoords 一致，Y 轴镜像）
		CoordStruct moveDisp;
		if (Data->Origin == VectorData::VectorOrigin::World)
		{
			// World 模式下 MoveTo 已是世界坐标，不做旋转
			moveDisp = moveFlh;
		}
		else if (pBullet)
		{
			double vx = pBullet->Velocity.X;
			double vy = pBullet->Velocity.Y;
			double len = std::sqrt(vx * vx + vy * vy);
			if (len > 1e-6)
			{
				double fwdX = vx / len;
				double fwdY = vy / len;
				moveDisp.X = static_cast<int>(moveFlh.X * fwdX - moveFlh.Y * fwdY);
				moveDisp.Y = static_cast<int>(-(moveFlh.X * fwdY + moveFlh.Y * fwdX));
				moveDisp.Z = moveFlh.Z;
			}
			else
			{
				moveDisp = moveFlh;
			}
		}
		else if (pTechno)
		{
			DirStruct facing = pTechno->PrimaryFacing.Current();
			double rad = facing.GetRadian();
			double cosA = std::cos(rad);
			double sinA = std::sin(rad);
			moveDisp.X = static_cast<int>(moveFlh.X * cosA - moveFlh.Y * sinA);
			moveDisp.Y = static_cast<int>(-(moveFlh.X * sinA + moveFlh.Y * cosA));
			moveDisp.Z = moveFlh.Z;
		}
		else
		{
			moveDisp = moveFlh;
		}

		// --- 波浪运动（垂直于运动方向的正弦偏移）---
		CoordStruct waveDisp{};
		if (Data->WaveAmplitude.X != 0 || Data->WaveAmplitude.Y != 0 || Data->WaveAmplitude.Z != 0)
		{
			double freqX = Data->WaveFrequency.X + Data->WaveFreqAcceleration.X * _elapsedFrames;
			double freqY = Data->WaveFrequency.Y + Data->WaveFreqAcceleration.Y * _elapsedFrames;
			double freqZ = Data->WaveFrequency.Z + Data->WaveFreqAcceleration.Z * _elapsedFrames;

			double ampX = Data->WaveAmplitude.X;
			double ampY = Data->WaveAmplitude.Y;
			double ampZ = Data->WaveAmplitude.Z;
			if (Data->WaveAmpOscillation != 0 && Data->WaveAmpOscFreq != 0)
			{
				double ampPhase = (_elapsedFrames * Data->WaveAmpOscFreq + Data->WaveAmpOscPhase) * M_PI / 180.0;
				double ampOsc = Data->WaveAmpOscillation * std::sin(ampPhase);
				ampX += ampOsc;
				ampY += ampOsc;
				ampZ += ampOsc;
			}

			waveDisp.X = static_cast<int>(ampX * std::sin((freqX * _elapsedFrames + Data->WavePhase.X) * M_PI / 180.0));
			waveDisp.Y = static_cast<int>(ampY * std::sin((freqY * _elapsedFrames + Data->WavePhase.Y) * M_PI / 180.0));
			waveDisp.Z = static_cast<int>(ampZ * std::sin((freqZ * _elapsedFrames + Data->WavePhase.Z) * M_PI / 180.0));
		}

		// --- 圆周旋转 ---
		CoordStruct rotateDisp{};
		if (Data->AnglePerFrame != 0 && Data->Radius != 0)
		{
			CoordStruct axis = GetRotateAxis(Data->RotateAxis);
			double angle = Data->AnglePerFrame * _elapsedFrames;
			CoordStruct offset = { Data->Radius, 0, 0 };
			rotateDisp = RodriguesRotate(offset, axis, angle);
			// 减去第一帧的偏移，使旋转从当前位置开始
			CoordStruct firstFrame = RodriguesRotate(offset, axis, 0);
			rotateDisp.X -= firstFrame.X;
			rotateDisp.Y -= firstFrame.Y;
			rotateDisp.Z -= firstFrame.Z;
		}

		// --- 朝目标飞行 + MoveTo 偏移 + 波浪 + 旋转 ---
		CoordStruct nextPos;
		if (hasTarget)
		{
			CoordStruct dirVec;
			dirVec.X = frameTarget.X - currentPos.X;
			dirVec.Y = frameTarget.Y - currentPos.Y;
			dirVec.Z = frameTarget.Z - currentPos.Z;
			double dirLen = std::sqrt(static_cast<double>(dirVec.X * dirVec.X + dirVec.Y * dirVec.Y + dirVec.Z * dirVec.Z));

			// ReachTarget：到达目标后停止
			if (Data->ReachTarget && dirLen <= speed)
			{
				nextPos = frameTarget;
			}
			else if (dirLen > 1e-6)
			{
				nextPos.X = currentPos.X + static_cast<int>(dirVec.X / dirLen * speed) + moveDisp.X + waveDisp.X + rotateDisp.X;
				nextPos.Y = currentPos.Y + static_cast<int>(dirVec.Y / dirLen * speed) + moveDisp.Y + waveDisp.Y + rotateDisp.Y;
				nextPos.Z = currentPos.Z + static_cast<int>(dirVec.Z / dirLen * speed) + moveDisp.Z + waveDisp.Z + rotateDisp.Z;
			}
			else
			{
				nextPos.X = currentPos.X + moveDisp.X + waveDisp.X + rotateDisp.X;
				nextPos.Y = currentPos.Y + moveDisp.Y + waveDisp.Y + rotateDisp.Y;
				nextPos.Z = currentPos.Z + moveDisp.Z + waveDisp.Z + rotateDisp.Z;
			}
		}
		else
		{
			nextPos.X = currentPos.X + moveDisp.X + waveDisp.X + rotateDisp.X;
			nextPos.Y = currentPos.Y + moveDisp.Y + waveDisp.Y + rotateDisp.Y;
			nextPos.Z = currentPos.Z + moveDisp.Z + waveDisp.Z + rotateDisp.Z;
		}

		// 合并偏移量
		result.MoveDisp.X += nextPos.X - currentPos.X;
		result.MoveDisp.Y += nextPos.Y - currentPos.Y;
		result.MoveDisp.Z += nextPos.Z - currentPos.Z;

		_elapsedFrames += Data->FrameStep;
		_moveFrame++;
	}

	return result;
}
