/*==============================================================================

   Solar Display [sky.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SKY_H
#define SKY_H

#include <DirectXMath.h>


void Sky_Initialize();

void Sky_Draw();


void Sky_Finalize(void);

void Sky_SetPosition(const DirectX::XMFLOAT3& position);
#endif // !SKY_H
