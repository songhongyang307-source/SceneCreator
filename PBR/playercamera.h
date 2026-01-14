/*==============================================================================

  Player Camera Controller[playercamera.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H



#include <DirectXMath.h>




void PlayerCamera_Initialize();
void PlayerCamera_Update(double elapsed_time);
void PlayerCamera_Finalize(void);

//const DirectX::XMFLOAT4X4& PlayerCamera_GetMatrix();

//const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix();

const DirectX::XMFLOAT3& PlayerCamera_GetFront();

const DirectX::XMFLOAT3& PlayerCamera_GetPosition();

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix();

const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix();
//float Camera_GetFov();

//void Camera_DebugDraw();
#endif  // PLAYER_CAMERA_H

