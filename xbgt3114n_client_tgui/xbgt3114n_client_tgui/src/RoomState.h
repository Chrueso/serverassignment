#pragma once
#include "BaseState.h"
#include "PacketType.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class RoomState : public BaseState
{
    tgui::Gui gui;

    // [0] = me, [1] = other player
    tgui::Label::Ptr title, countdownLabel;
    tgui::Label::Ptr names[2], statuses[2], records[2], creatureNames[2];
    tgui::Picture::Ptr creaturePics[2], emotePics[2];
    tgui::Button::Ptr readyBtn, leaveBtn, sendBtn;
    tgui::Button::Ptr creatureBtns[3];
    tgui::Button::Ptr emoteBtns[4];
    tgui::Tabs::Ptr chatTabs;
    tgui::ChatBox::Ptr roomChat, lobbyChat;
    tgui::EditBox::Ptr chatInput;

    bool myReady = false;
    int  myCreature = -1;
    int  myWins = -1, myLosses = -1;

    int  otherId = -1;
    std::string otherName;
    bool otherReady = false;
    int  otherCreature = -1;
    int  otherWins = 0, otherLosses = 0;

    float emoteTimer[2] = { 0, 0 };

    void refreshCards();
    void clearOtherPlayer();
    void handleRoomUpdate(sf::Packet& packet);
    void handleRoomList(sf::Packet& packet);
    void showEmote(int playerId, int emoteId);
    void onSendPress();
    void sendRoomPacket(PacketType type);

protected:
    void handlePacket(sf::Packet& packet) override;

public:
    RoomState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
    void activate() override;
    void pause() override {}
    void resume() override {}
    void handleEvent(sf::Event event) override { gui.handleEvent(event); }
    void update(sf::Time deltaTime) override;
    void draw() override { gui.draw(); }
};
