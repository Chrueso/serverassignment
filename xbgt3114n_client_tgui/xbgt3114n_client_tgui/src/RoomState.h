#pragma once
#include "BaseState.h"
#include "PacketType.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class RoomState : public BaseState
{
    tgui::Gui gui; 

    // widgets ([0] = card 1 on the left, [1] = card 2 on the right)
    tgui::Label::Ptr title, countdownLabel;
    tgui::Label::Ptr names[2], statuses[2], records[2], creatureNames[2];
    tgui::Picture::Ptr creaturePics[2], emotePics[2];
    tgui::Button::Ptr readyBtn, leaveBtn, sendBtn;
    tgui::Button::Ptr creatureBtns[3];
    tgui::Tabs::Ptr chatTabs;
    tgui::ChatBox::Ptr roomChat, lobbyChat;
    tgui::EditBox::Ptr chatInput;

    int slotPlayerId[2] = { -1, -1 };   // which player id is shown on each card
    float emoteTimer[2] = { 0, 0 };     // seconds left for each emote bubble
    float countdown = -1.0f;            // visual countdown, -1 = not counting

    void handleRoomUpdate(sf::Packet& packet);
    void showEmote(int playerId, int emoteId);
    void onSendPress();
    void sendSimple(PacketType type);   // send a packet with no fields

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