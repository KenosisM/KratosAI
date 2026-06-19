#include "PassengerEnterEffect.h"

#include <Ext/Helper/Scripts.h>

void PassengerEnterEffect::OnUpdate()
{
	OnWarpUpdate();
}

void PassengerEnterEffect::OnWarpUpdate()
{
	if (AE->OwnerIsDead() || !pTechno || !IsAlive())
		return;

	TechnoClass* pTransporter = pTechno->Transporter;
	TechnoClass* pPrevious = _transporter;

	if (pTransporter == pPrevious)
		return;

	_transporter = pTransporter;

	if (pTransporter)
	{
		AttachEffect* transporterAEM = GetAEManager<TechnoExt>(pTransporter);
		if (transporterAEM)
		{
			for (const std::string& aeName : Data->AttachEffects)
			{
				transporterAEM->Attach(aeName, false, pTechno, nullptr, CoordStruct::Empty, -1, true);
			}
		}
	}
}
