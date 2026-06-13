#pragma once

#include <string>
#include <vector>
#include <cmath>

#include <GeneralStructures.h>

#include <Ext/EffectType/Effect/EffectData.h>
#include <Ext/Helper/CastEx.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum class VectorFacing : int
{
	Destination = 0, Target = 1, Initial = 2,
};

template <>
inline bool Parser<VectorFacing>::TryParse(const char* pValue, VectorFacing* outValue)
{
	switch (toupper(static_cast<unsigned char>(*pValue)))
	{
	case 'T':
		if (outValue) { *outValue = VectorFacing::Target; }
		return true;
	case 'I':
		if (outValue) { *outValue = VectorFacing::Initial; }
		return true;
	default:
		if (outValue) { *outValue = VectorFacing::Destination; }
		return true;
	}
}

enum class VectorRotateAxis : int
{
	X = 0, Y = 1, Z = 2,
};

template <>
inline bool Parser<VectorRotateAxis>::TryParse(const char* pValue, VectorRotateAxis* outValue)
{
	switch (toupper(static_cast<unsigned char>(*pValue)))
	{
	case 'X':
		if (outValue) { *outValue = VectorRotateAxis::X; }
		return true;
	case 'Y':
		if (outValue) { *outValue = VectorRotateAxis::Y; }
		return true;
	default:
		if (outValue) { *outValue = VectorRotateAxis::Z; }
		return true;
	}
}

inline CoordStruct RodriguesRotate(CoordStruct v, CoordStruct k, double angleDeg)
{
	double rad = angleDeg * M_PI / 180.0;
	double c = std::cos(rad);
	double s = std::sin(rad);
	double t = 1.0 - c;

	double kLen = std::sqrt(k.X * k.X + k.Y * k.Y + k.Z * k.Z);
	if (kLen < 1e-6)
	{
		return v;
	}
	double kx = k.X / kLen;
	double ky = k.Y / kLen;
	double kz = k.Z / kLen;

	CoordStruct result;
	result.X = static_cast<int>(v.X * (kx * kx * t + c) + v.Y * (kx * ky * t - kz * s) + v.Z * (kx * kz * t + ky * s));
	result.Y = static_cast<int>(v.X * (ky * kx * t + kz * s) + v.Y * (ky * ky * t + c) + v.Z * (ky * kz * t - kx * s));
	result.Z = static_cast<int>(v.X * (kz * kx * t - ky * s) + v.Y * (kz * ky * t + kx * s) + v.Z * (kz * kz * t + c));
	return result;
}

inline CoordStruct GetRotateAxis(VectorRotateAxis axis)
{
	switch (axis)
	{
	case VectorRotateAxis::X: return { 1, 0, 0 };
	case VectorRotateAxis::Y: return { 0, 1, 0 };
	case VectorRotateAxis::Z: return { 0, 0, 1 };
	default: return { 0, 0, 1 };
	}
}

class VectorData : public EffectData
{
public:
	EFFECT_DATA(Vector);

	CoordStruct TargetFLH{};
	Point2D TargetRandF{};
	Point2D TargetRandL{};
	Point2D TargetRandH{};
	bool ReachTarget = false;

	int StartSpeed = 0;
	int EndSpeed = -1;
	int Acceleration = 0;
	int FrameStep = 1;
	int SpeedOscillation = 0;
	int SpeedOscFreq = 0;
	int SpeedOscPhase = 0;

	CoordStruct MoveTo{};
	CoordStruct GrowRate{};
	int GrowOscillationX = 0;
	int GrowOscillationY = 0;
	int GrowOscillationZ = 0;
	int GrowOscFreqX = 0;
	int GrowOscFreqY = 0;
	int GrowOscFreqZ = 0;
	int GrowOscPhaseX = 0;
	int GrowOscPhaseY = 0;
	int GrowOscPhaseZ = 0;
	bool Force = false;
	bool Freeze = false;

	CoordStruct WaveAmplitude{};
	CoordStruct WaveFrequency{};
	CoordStruct WavePhase{};
	CoordStruct WaveFreqAcceleration{};
	CoordStruct WaveFreqEndSpeed{};
	int WaveAmpOscillation = 0;
	int WaveAmpOscFreq = 0;
	int WaveAmpOscPhase = 0;
	int WaveFreqOscillation = 0;
	int WaveFreqOscFreq = 0;
	int WaveFreqOscPhase = 0;

	int AnglePerFrame = 0;
	int Radius = 0;
	VectorRotateAxis RotateAxis;
	CoordStruct RotateDirection{};

	int SpinRate = 0;
	int SpinEndSpeed = -1;
	int SpinAcceleration = 0;
	VectorRotateAxis SpinAxis;
	CoordStruct SpinDirection{};
	int SpinOscillation = 0;
	int SpinOscFreq = 0;
	int SpinOscPhase = 0;

	int TurretSpinRate = 0;
	int TurretSpinEndSpeed = -1;
	int TurretSpinAcceleration = 0;
	VectorRotateAxis TurretSpinAxis;
	CoordStruct TurretSpinDirection{};
	int TurretSpinOscillation = 0;
	int TurretSpinOscFreq = 0;
	int TurretSpinOscPhase = 0;
	bool FreeTurret = false;

	VectorFacing Facing = VectorFacing::Destination;

	enum class VectorOrigin : int
	{
		World = 0, Target = 1, Launcher = 2, Self = 3, Realtime = 4, Source = 5, FLH = 6,
	};
	VectorOrigin Origin = VectorOrigin::Self;
	CoordStruct OriginFLH{};
	bool OriginNoUpdate = false;

	VectorData() : EffectData()
	{
		AffectBuilding = false;
	}


	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "Vector.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		TargetFLH = reader->Get(title + "TargetFLH", TargetFLH);
		TargetRandF = reader->Get(title + "TargetRandF", TargetRandF);
		TargetRandL = reader->Get(title + "TargetRandL", TargetRandL);
		TargetRandH = reader->Get(title + "TargetRandH", TargetRandH);
		ReachTarget = reader->Get(title + "ReachTarget", ReachTarget);

		StartSpeed = reader->Get(title + "StartSpeed", StartSpeed);
		EndSpeed = reader->Get(title + "EndSpeed", EndSpeed);
		Acceleration = reader->Get(title + "Acceleration", Acceleration);
		FrameStep = reader->Get(title + "FrameStep", FrameStep);
		if (FrameStep < 1) FrameStep = 1;
		SpeedOscillation = reader->Get(title + "SpeedOscillation", SpeedOscillation);
		SpeedOscFreq = reader->Get(title + "SpeedOscFreq", SpeedOscFreq);
		SpeedOscPhase = reader->Get(title + "SpeedOscPhase", SpeedOscPhase);

		MoveTo = reader->Get(title + "MoveTo", MoveTo);
		GrowRate = reader->Get(title + "GrowRate", GrowRate);
		GrowOscillationX = reader->Get(title + "GrowOscillationX", GrowOscillationX);
		GrowOscillationY = reader->Get(title + "GrowOscillationY", GrowOscillationY);
		GrowOscillationZ = reader->Get(title + "GrowOscillationZ", GrowOscillationZ);
		GrowOscFreqX = reader->Get(title + "GrowOscFreqX", GrowOscFreqX);
		GrowOscFreqY = reader->Get(title + "GrowOscFreqY", GrowOscFreqY);
		GrowOscFreqZ = reader->Get(title + "GrowOscFreqZ", GrowOscFreqZ);
		GrowOscPhaseX = reader->Get(title + "GrowOscPhaseX", GrowOscPhaseX);
		GrowOscPhaseY = reader->Get(title + "GrowOscPhaseY", GrowOscPhaseY);
		GrowOscPhaseZ = reader->Get(title + "GrowOscPhaseZ", GrowOscPhaseZ);
		Force = reader->Get(title + "Force", Force);
		Freeze = reader->Get(title + "Freeze", Freeze);

		WaveAmplitude = reader->Get(title + "WaveAmplitude", WaveAmplitude);
		WaveFrequency = reader->Get(title + "WaveFrequency", WaveFrequency);
		WavePhase = reader->Get(title + "WavePhase", WavePhase);
		WaveFreqAcceleration = reader->Get(title + "WaveFreqAcceleration", WaveFreqAcceleration);
		WaveFreqEndSpeed = reader->Get(title + "WaveFreqEndSpeed", WaveFreqEndSpeed);
		WaveAmpOscillation = reader->Get(title + "WaveAmpOscillation", WaveAmpOscillation);
		WaveAmpOscFreq = reader->Get(title + "WaveAmpOscFreq", WaveAmpOscFreq);
		WaveAmpOscPhase = reader->Get(title + "WaveAmpOscPhase", WaveAmpOscPhase);
		WaveFreqOscillation = reader->Get(title + "WaveFreqOscillation", WaveFreqOscillation);
		WaveFreqOscFreq = reader->Get(title + "WaveFreqOscFreq", WaveFreqOscFreq);
		WaveFreqOscPhase = reader->Get(title + "WaveFreqOscPhase", WaveFreqOscPhase);

		AnglePerFrame = reader->Get(title + "AnglePerFrame", AnglePerFrame);
		Radius = reader->Get(title + "Radius", Radius);
		RotateAxis = reader->Get(title + "RotateAxis", RotateAxis);
		RotateDirection = reader->Get(title + "RotateDirection", RotateDirection);

		SpinRate = reader->Get(title + "SpinRate", SpinRate);
		SpinEndSpeed = reader->Get(title + "SpinEndSpeed", SpinEndSpeed);
		SpinAcceleration = reader->Get(title + "SpinAcceleration", SpinAcceleration);
		SpinAxis = reader->Get(title + "SpinAxis", SpinAxis);
		SpinDirection = reader->Get(title + "SpinDirection", SpinDirection);
		SpinOscillation = reader->Get(title + "SpinOscillation", SpinOscillation);
		SpinOscFreq = reader->Get(title + "SpinOscFreq", SpinOscFreq);
		SpinOscPhase = reader->Get(title + "SpinOscPhase", SpinOscPhase);

		TurretSpinRate = reader->Get(title + "TurretSpinRate", TurretSpinRate);
		TurretSpinEndSpeed = reader->Get(title + "TurretSpinEndSpeed", TurretSpinEndSpeed);
		TurretSpinAcceleration = reader->Get(title + "TurretSpinAcceleration", TurretSpinAcceleration);
		TurretSpinAxis = reader->Get(title + "TurretSpinAxis", TurretSpinAxis);
		TurretSpinDirection = reader->Get(title + "TurretSpinDirection", TurretSpinDirection);
		TurretSpinOscillation = reader->Get(title + "TurretSpinOscillation", TurretSpinOscillation);
		TurretSpinOscFreq = reader->Get(title + "TurretSpinOscFreq", TurretSpinOscFreq);
		TurretSpinOscPhase = reader->Get(title + "TurretSpinOscPhase", TurretSpinOscPhase);
		FreeTurret = reader->Get(title + "FreeTurret", FreeTurret);

		Facing = reader->Get(title + "Facing", Facing);

		std::string originStr = reader->Get(title + "Origin", std::string{ "Self" });
		if (originStr == "World") Origin = VectorOrigin::World;
		else if (originStr == "Target") Origin = VectorOrigin::Target;
		else if (originStr == "Launcher") Origin = VectorOrigin::Launcher;
		else if (originStr == "Self") Origin = VectorOrigin::Self;
		else if (originStr == "Realtime") Origin = VectorOrigin::Realtime;
		else if (originStr == "Source") Origin = VectorOrigin::Source;
		else if (originStr == "FLH") Origin = VectorOrigin::FLH;
		else Origin = VectorOrigin::Self;

		OriginFLH = reader->Get(title + "OriginFLH", OriginFLH);
		OriginNoUpdate = reader->Get(title + "OriginNoUpdate", OriginNoUpdate);

		Enable = !MoveTo.IsEmpty() || !TargetFLH.IsEmpty() || AnglePerFrame != 0 || Radius != 0
			|| SpinRate != 0 || !WaveAmplitude.IsEmpty() || Force || Freeze;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		stream
			.Process(this->TargetFLH)
			.Process(this->TargetRandF)
			.Process(this->TargetRandL)
			.Process(this->TargetRandH)
			.Process(this->ReachTarget)

			.Process(this->StartSpeed)
			.Process(this->EndSpeed)
			.Process(this->Acceleration)
			.Process(this->FrameStep)
			.Process(this->SpeedOscillation)
			.Process(this->SpeedOscFreq)
			.Process(this->SpeedOscPhase)

			.Process(this->MoveTo)
			.Process(this->GrowRate)
			.Process(this->GrowOscillationX)
			.Process(this->GrowOscillationY)
			.Process(this->GrowOscillationZ)
			.Process(this->GrowOscFreqX)
			.Process(this->GrowOscFreqY)
			.Process(this->GrowOscFreqZ)
			.Process(this->GrowOscPhaseX)
			.Process(this->GrowOscPhaseY)
			.Process(this->GrowOscPhaseZ)
			.Process(this->Force)
			.Process(this->Freeze)

			.Process(this->WaveAmplitude)
			.Process(this->WaveFrequency)
			.Process(this->WavePhase)
			.Process(this->WaveFreqAcceleration)
			.Process(this->WaveFreqEndSpeed)
			.Process(this->WaveAmpOscillation)
			.Process(this->WaveAmpOscFreq)
			.Process(this->WaveAmpOscPhase)
			.Process(this->WaveFreqOscillation)
			.Process(this->WaveFreqOscFreq)
			.Process(this->WaveFreqOscPhase)

			.Process(this->AnglePerFrame)
			.Process(this->Radius)
			.Process(this->RotateAxis)
			.Process(this->RotateDirection)

			.Process(this->SpinRate)
			.Process(this->SpinEndSpeed)
			.Process(this->SpinAcceleration)
			.Process(this->SpinAxis)
			.Process(this->SpinDirection)
			.Process(this->SpinOscillation)
			.Process(this->SpinOscFreq)
			.Process(this->SpinOscPhase)

			.Process(this->TurretSpinRate)
			.Process(this->TurretSpinEndSpeed)
			.Process(this->TurretSpinAcceleration)
			.Process(this->TurretSpinAxis)
			.Process(this->TurretSpinDirection)
			.Process(this->TurretSpinOscillation)
			.Process(this->TurretSpinOscFreq)
			.Process(this->TurretSpinOscPhase)
			.Process(this->FreeTurret)

			.Process(this->Facing)
			.Process(this->Origin)
			.Process(this->OriginFLH)
			.Process(this->OriginNoUpdate);
		return stream.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectData::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectData::Save(stream);
		return const_cast<VectorData*>(this)->Serialize(stream);
	}
#pragma endregion
};
