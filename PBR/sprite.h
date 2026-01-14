#pragma once
/*
スプライト表示
*/

#ifndef SPRITE_H
#define SPRITE_H

#include <d3d11.h>
#include <DirectXMath.h>


void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Sprite_Finalize(void);

void Sprite_Begin();
//void Sprite_Draw(float dx,float dy, float dw, float dh);
// 
//テクスチャ全表示
void Sprite_Draw(int texid, float dx,float dy, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });

//テクスチャ全表示(改变显示大小)
void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });

//テクスチャ全表示(改变显示大小) 旋转
void Sprite_Draw_R(int texid, float dx, float dy, float dw, float dh, float angle, float pivotx, float pivoty, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });

//UV CUT
void Sprite_Draw(int texid, float dx, float dy,  float px, float py, int pw , int ph, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });

//UV CUT(改变显示大小)
void Sprite_Draw(int texid, float dx, float dy,float dw,float dh, float px, float py, int pw, int ph, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });
//                   texid 读取图片在texture类的构造体数组中的位置（第几个纹理图
//                   dx dy 实现将纹理图显示在屏幕中时，左上角的像素点的位置
//					 dw dh 纹理图在屏幕中显示的宽度和高度 （与）
//                   dx dy and dw dh 共同确定了图片在屏幕中的位置与:大小:？
//							 px				py							 pw								ph 
//  uv截取面积确定   uv中左上角的位置    uv右上角的位置			 从左边到右边的截取边界			从上边到下边的截取边界
//UV CUT(改变显示大小) 旋转
void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, float px, float py, int pw, int ph, float angle, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 1.0f , 1.0f });

void Sprite_Draw(float dx, float dy, float dw, float dh, const DirectX::XMFLOAT4& color);
float GetScreenWidth(int texid);


float GetScreenHeight(int texid);


#endif 