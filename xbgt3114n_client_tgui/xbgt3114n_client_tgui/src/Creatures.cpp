#include "Creatures.h"

const char* CREATURE_NAMES[CREATURE_COUNT] = { "CreatureA", "CreatureB", "CreatureC" };
const char* MOVE_NAMES[MOVE_COUNT] = { "Attack", "Defend", "Break" };

std::string creatureName(int creature, const std::string& none)
{
    return (creature >= 0 && creature < CREATURE_COUNT) ? CREATURE_NAMES[creature] : none;
}

std::string moveName(int move)
{
    return (move >= 0 && move < MOVE_COUNT) ? MOVE_NAMES[move] : "?";
}

void showCreaturePicture(tgui::Picture::Ptr pic, int creature)
{
    if (creature < 0 || creature >= CREATURE_COUNT)
    {
        pic->setVisible(false);
        return;
    }
    pic->getRenderer()->setTexture(tgui::Texture("../assets/creatures/creature_" + std::to_string(creature) + ".png"));
    pic->setVisible(true);
}
