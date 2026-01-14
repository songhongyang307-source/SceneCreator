/*==============================================================================

  Game Effect[gameEffect.h]
														 Author : Youhei Sato
														 Date   : 2025/07/01
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef GAMEEFFECT_H
#define GAMEEFFECT_H

#include <DirectXMath.h>

void Effect_Initialize();

void Effect_Update(double elapsed_time);

void Effect_Finalize();
void Effect_Draw();
void Effect_Create(const DirectX::XMFLOAT2& position);


#endif // BULLET_H

