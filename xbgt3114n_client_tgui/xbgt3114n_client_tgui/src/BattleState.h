#pragma once
#include "BaseState.h"
#include "PacketType.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class BattleState : public BaseState
{
    tgui::Gui gui;

    tgui::Label::Ptr roundLabel, resultLabel, statusLabel;
    tgui::Label::Ptr names[2], moveLabels[2], creatureNames[2];
    tgui::Picture::Ptr creaturePics[2], emotePics[2];
    tgui::ProgressBar::Ptr hpBars[2];
    tgui::Button::Ptr moveBtns[3];
    tgui::Button::Ptr emoteBtns[4];
    tgui::Tabs::Ptr chatTabs;
    tgui::ChatBox::Ptr roomChat, lobbyChat;
    tgui::EditBox::Ptr chatInput;
    tgui::Panel::Ptr resultPanel;
    tgui::Label::Ptr resultTitle, resultInfo;


    int playerIds[2] = { -1, -1 };
    std::string playerNames[2];
    bool opponentLocked = false;
    bool battleOver = false;
    float emoteTimer[2] = { 0, 0 };

    int slotOf(int playerId);
    void chooseMove(int move);
    void setChoosing(bool canChoose);
    void setHp(int slot, int hp, int maxHp);
    void handleBattleUpdate(sf::Packet& packet);
    void handleRoundResult(sf::Packet& packet);
    void handleBattleEnd(sf::Packet& packet);
    void showResultPanel(const std::string& title, const std::string& info, tgui::Color color);
    void showEmote(int playerId, int emoteId);
    void onSendPress();

protected:
    void handlePacket(sf::Packet& packet) override;

public:
    BattleState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
    void activate() override;
    void pause() override {}
    void resume() override {}
    void handleEvent(sf::Event event) override { gui.handleEvent(event); }
    void update(sf::Time deltaTime) override;
    void draw() override { gui.draw(); }
};
