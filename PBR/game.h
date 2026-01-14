/*====================================================


ゲーム本体「game.h」



=============================================================*/

#ifndef GAME_H
#define GAME_H

void Game_Initialize();

void Game_Finalize();

void Game_Update(double elapsed_time);

void Game_Draw();

void TerrainEdit_Update(double dt);

#endif//GAME_H