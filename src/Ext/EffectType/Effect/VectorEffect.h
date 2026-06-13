#pragma once

#include <string>
#include <vector>

#include <GeneralDefinitions.h>

#include "../EffectScript.h"
#include "VectorData.h"

class VectorEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(Vector);

	virtual void Clean() override
	{
		EffectScript::Clean();

		_initialLocation = {};
		_initialTarget = {};
		_initialOriginPos = {};
		_randomTargetOffset = {};
		_pLauncher = nullptr;
		_pSource = nullptr;
		_currentSpeed = 0;
		_elapsedFrames = 0;
		_moveFrame = 0;
	}

	virtual void OnStart() override;

	// 获取位移结果，包含位移向量和冻结状态等信息
	VectorResult GetVectorResult();

#pragma region Save/Load
	template <typename T>
	bool Serialize(T& stream) {
		stream
			.Process(this->_initialLocation)
			.Process(this->_initialTarget)
			.Process(this->_initialOriginPos)
			.Process(this->_randomTargetOffset)
			.Process(this->_pLauncher)
			.Process(this->_pSource)
			.Process(this->_currentSpeed)
			.Process(this->_elapsedFrames)
			.Process(this->_moveFrame);
		return stream.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectScript::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectScript::Save(stream);
		return const_cast<VectorEffect*>(this)->Serialize(stream);
	}
#pragma endregion

	CoordStruct _initialLocation{};
	CoordStruct _initialTarget{};
	CoordStruct _initialOriginPos{};
	CoordStruct _randomTargetOffset{};
	ObjectClass* _pLauncher = nullptr;
	ObjectClass* _pSource = nullptr;
	int _currentSpeed = 0;
	int _elapsedFrames = 0;
	int _moveFrame = 0;
};
