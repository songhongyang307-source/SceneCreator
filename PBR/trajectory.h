/*==============================================================================

  軌跡エフェクトの描画[trajectory.h]
														 Author : Youhei Sato
														 Date   : 2025/07/01
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <DirectXMath.h>

void Trajectory_Initialize();

void Trajectory_Update(double elapsed_time);

void Trajectory_Finalize();
void Trajectory_Draw();

void Trajectory_Create(const DirectX::XMFLOAT2& position,
	const DirectX::XMFLOAT4& color,
	float size, double lifetime);
#endif // TRAJECTORY_H

