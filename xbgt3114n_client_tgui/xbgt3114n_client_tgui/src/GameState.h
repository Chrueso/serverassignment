#pragma once
#include "BaseState.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class GameState : public BaseState
{
    tgui::Gui gui; // important to be first

    tgui::Button::Ptr btn;
    tgui::ChatBox::Ptr cht;
    tgui::Label::Ptr label;
    tgui::EditBox::Ptr edt;

    // room
    tgui::Button::Ptr createRoomBtn;
    tgui::Button::Ptr joinRoomBtn;
    tgui::ListView::Ptr roomList;

    struct RoomInfo
    {
        int id;
        int players;
        int maxPlayers;
        bool inBattle;
    };
    std::vector<RoomInfo> rooms;
    void handleRoomList(sf::Packet& packet);
    void updateJoinButton();
    
    void sendMessage();
    void onCreateRoomPress();
    void onJoinRoomPress();

protected:
    void handlePacket(sf::Packet& packet) override;

public:
    GameState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);

    void activate() override;
    void pause() override;
    void resume() override;
    void handleEvent(sf::Event event) override;
    void update(sf::Time deltaTime) override;
    void draw() override;
};
