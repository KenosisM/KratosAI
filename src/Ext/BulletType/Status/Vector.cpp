#include "../BulletStatus.h"

#include <Ext/Helper/FLH.h>
#include <Ext/Helper/Physics.h>
#include <Ext/Helper/Weapon.h>
#include <Ext/Helper/Scripts.h>

void BulletStatus::OnUpdate_Vector()
{
	// 被黑洞捕获时，停止 Vector 执行
	if (CaptureByBlackHole)
		return;

	_vectorResult = AEManager()->MarginVectorOffset();
	VectorForced = !_vectorResult.MoveDisp.IsEmpty();

	// 检查是否有 Freeze，有则不设 Velocity（Freeze 在 OnUpdateEnd 处理）
	if (!_vectorResult.Freeze && !_vectorResult.MoveDisp.IsEmpty())
	{
		// 设置 Velocity = 位移方向，让引擎协作移动抛射体
		pBullet->Velocity.X = static_cast<double>(_vectorResult.MoveDisp.X);
		pBullet->Velocity.Y = static_cast<double>(_vectorResult.MoveDisp.Y);
		pBullet->Velocity.Z = static_cast<double>(_vectorResult.MoveDisp.Z);
	}
}

void BulletStatus::OnUpdateEnd_Vector(CoordStruct& sourcePos)
{
	// 被黑洞捕获时，停止 Vector 执行
	if (CaptureByBlackHole)
		return;

	if (VectorForced)
	{
		// Freeze + Force: 强制把抛射体拉回冻结位置（Freeze 场景仍需 SetLocation）
		if (_vectorResult.Freeze && !_vectorResult.FrozenPos.IsEmpty())
		{
			pBullet->SetLocation(_vectorResult.FrozenPos);
			pBullet->SourceCoords = _vectorResult.FrozenPos;
			sourcePos = _vectorResult.FrozenPos;
			return;
		}

		// 抛射体撞到地面
		bool canMove = pBullet->GetHeight() > 0;
		// 检查悬崖
		if (canMove)
		{
			CoordStruct nextCellPos = CoordStruct::Empty;
			bool onBridge = false;
			switch (CanMoveTo(sourcePos, _vectorResult.MoveDisp, _vectorResult.CanPassBuilding, nextCellPos, onBridge))
			{
			case PassError::UNDERGROUND:
			case PassError::HITWALL:
			case PassError::HITBUILDING:
			case PassError::DOWNBRIDGE:
			case PassError::UPBRIDEG:
				canMove = false;
				break;
			}
		}
		if (!canMove)
		{
			// 原地爆炸
			TakeDamage();
		}
		else
		{
			// 被黑洞吸走
			pBullet->SetLocation(_vectorResult.MoveDisp);
			// 修改了位置所以更新位置
			sourcePos = _vectorResult.MoveDisp;
		}
	}
}
