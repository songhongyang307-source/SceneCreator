/*------------------------------------------------------------------------


  keyboard input

------------------------------------------------------------------------ */
#ifndef KEY_LOGGER_H
#define KEY_LOGGER_H

#include "keyboard.h"



void KeyLogger_Initialize();

void KeyLogger_Update();

bool KeyLogger_IsPressed(Keyboard_Keys key); //是否按下按键不松开
bool KeyLogger_IsTrigger(Keyboard_Keys key);
bool KeyLogger_IsReleased(Keyboard_Keys key);

#endif