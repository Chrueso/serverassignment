#pragma once
#include <TGUI/TGUI.hpp>
#include <string>

const int CREATURE_COUNT = 3;
extern const char* CREATURE_NAMES[CREATURE_COUNT];

const int MOVE_COUNT = 3;   // 0 Attack, 1 Defend, 2 Break
extern const char* MOVE_NAMES[MOVE_COUNT];

std::string creatureName(int creature, const std::string& none = "No creature yet");
std::string moveName(int move);

void showCreaturePicture(tgui::Picture::Ptr pic, int creature);
