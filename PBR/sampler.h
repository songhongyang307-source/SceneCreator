
/*==============================================================================

   SAMPLER [sampler.h]
														 Author : Youhei Sato
														 Date   : 2025/09/18
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SAMPLER_H
#define SAMPLER_H


#include <d3d11.h>
#include <DirectXMath.h>




void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

void Sampler_Finalize(void);

void Sampler_SetIBLClamp();

//depth buffer setting
void Sampler_SetFilterPoint();
void Sampler_SetFilterLinear();
void Sampler_SetFilterAnisotropic();



#endif  // SAMPLER_H