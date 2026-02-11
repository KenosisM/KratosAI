#pragma once

#include "../StateScript.h"
#include "DisableWeaponData.h"

class DisableWeaponState : public StateScript<DisableWeaponData>
{
public:
	STATE_SCRIPT(DisableWeapon);

	AbstractClass* LastTarget = nullptr;

	virtual void OnUpdate() override
	{
		StateScript<DisableWeaponData>::OnUpdate();

		if (Data.DisableWithTarget && Data.Enable && pTechno)
		{
			if (!pTechno->Target && LastTarget)
			{
				pTechno->SetTarget(LastTarget);
			}
			else if (pTechno->Target && pTechno->Target != LastTarget)
			{
				LastTarget = pTechno->Target;
			}
		}
	}

	virtual void Clean() override
	{
		StateScript<DisableWeaponData>::Clean();
		LastTarget = nullptr;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->LastTarget)
			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange)
	{
		StateScript<DisableWeaponData>::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const
	{
		StateScript<DisableWeaponData>::Save(stream);
		return const_cast<DisableWeaponState*>(this)->Serialize(stream);
	}
#pragma endregion
};
