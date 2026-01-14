/*==============================================================================

			Score Controller[score.h]
														 Author : satou youhei
														 Date   : 2025/07/09
--------------------------------------------------------------------------------

==============================================================================*/


#ifndef SCORE_H
#define SCORE_H

void Score_Initialize(float x , float y , int digit);   //posion.x , position.y   ,   digit 桁　　　他にゼロ埋め、左寄せなど
void Score_Finalize();
void Score_Update();
void Score_Draw();

unsigned int Score_GetScore();

void Score_Increment(int score);
void Score_Reset();

void Score_DrawHighScore(float x, float y, unsigned int highscore);

#endif






