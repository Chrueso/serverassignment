#include "GameState.h"
#include "PacketType.h"
#include "RoomState.h"

GameState::GameState(StateMachine& machine, sf::RenderWindow& window, bool replace)
    : BaseState{machine, window, replace}, gui(window)
{
    gui.loadWidgetsFromFile("../assets/form_game.txt");

    gui.get<tgui::Button>("Button1")->onPress(&GameState::sendMessage, this);
    cht = gui.get<tgui::ChatBox>("ChatBox1");
    edt = gui.get<tgui::EditBox>("EditBox1");

    // room
    createRoomBtn = gui.get<tgui::Button>("CreateRoomButton");
    joinRoomBtn = gui.get<tgui::Button>("JoinRoomButton");
    roomList = gui.get<tgui::ListView>("RoomList");
    roomList->addColumn("Room", 400);
    roomList->addColumn("Players", 110);
    roomList->addColumn("Status", 116);

    roomList->onItemSelect([this](int index) { updateJoinButton(); });
    roomList->onDoubleClick([this](int index) { onJoinRoomPress(); });

    createRoomBtn->onPress(&GameState::onCreateRoomPress, this);
    joinRoomBtn->onPress(&GameState::onJoinRoomPress, this);

    joinRoomBtn->setEnabled(false);
}

void GameState::handlePacket(sf::Packet& packet)
{
    PacketType type;
    packet >> type;

    switch (type)
    {
    case PacketType::UserConnected:
        {
            std::string playerName;
            packet >> playerName;
            cht->addLine(playerName + " has joined", tgui::Color::Blue);
            break;
        }
    case PacketType::UserDisconnected:
        {
            std::string playerName;
            packet >> playerName;
            cht->addLine(playerName + " has disconnected", tgui::Color::Blue);
            break;
        }
    case PacketType::ChatMessage:
        {
            std::string playerName;
            std::string msg;
            int playerId;
            packet >> playerName >> playerId >> msg;
            int clientId = NetworkLocator::get().getClientId();

            cht->addLine(playerName + ": " + msg, playerId == clientId ? tgui::Color::Green : tgui::Color::Black);
            break;
        }
    case PacketType::RoomList:
        handleRoomList(packet);
        break;
    case PacketType::RoomJoined:
        {
            int roomId;
            packet >> roomId;
            NetworkLocator::get().setRoomId(roomId);
            m_next = StateMachine::build<RoomState>(m_machine, m_window, true);
            break;
        }
    case PacketType::RoomJoinFailed:
        {
            std::string reason;
            packet >> reason;
            cht->addLine("[Server] Could not join: " + reason, tgui::Color::Red);
            createRoomBtn->setEnabled(true);
            updateJoinButton();
            break;
        }
    case PacketType::RoomCreated:
        {
            int roomId, hostId;
            std::string roomName;
            packet >> roomId >> roomName >> hostId;

            if (hostId == NetworkLocator::get().getClientId())
                cht->addLine("[Server] You created " + roomName, tgui::Color::Blue);
            else
                cht->addLine("[Server] " + roomName + " was created", tgui::Color::Blue);
            break;
        }
    }
}

void GameState::handleRoomList(sf::Packet& packet)
{
    int selectedRoomId = -1;
    int selected = roomList->getSelectedItemIndex();
    if (selected >= 0 && selected < (int)rooms.size())
    {
        selectedRoomId = rooms[selected].id;
    }

    roomList->removeAllItems();
    rooms.clear();

    int count = 0;
    packet >> count;
    for (int i = 0; i < count; i++)
    {
        RoomInfo roomInfo;
        std::string name;
        packet >> roomInfo.id >> name >> roomInfo.players >> roomInfo.maxPlayers >> roomInfo.inBattle;

        std::string status = roomInfo.inBattle
                                 ? "In Battle"
                                 : (roomInfo.players >= roomInfo.maxPlayers ? "Full" : "Waiting");
        std::string players = std::to_string(roomInfo.players) + "/" + std::to_string(roomInfo.maxPlayers);

        roomList->addItem({name, players, status});
        rooms.push_back(roomInfo);

        if (roomInfo.id == selectedRoomId)
            roomList->setSelectedItem(rooms.size() - 1);
    }
    updateJoinButton();
}

void GameState::updateJoinButton()
{

    // enable join button depend on room status
    int selected = roomList->getSelectedItemIndex();
    bool allowedToJoin = selected >= 0 && selected < (int)rooms.size()
        && !rooms[selected].inBattle
        && rooms[selected].players < rooms[selected].maxPlayers;
    joinRoomBtn->setEnabled(allowedToJoin);
}

void GameState::sendMessage()
{
    // 1. get text from editbox
    std::string msg = edt->getText().toStdString();
    if (msg.length() > 0)
    {
        sf::Packet outPacket;

        outPacket << PacketType::ChatMessage << msg;
        NetworkLocator::get().sendPacket(outPacket);
        edt->setText("");
    }
    // 2. if text > 0, send out packet
}

void GameState::onCreateRoomPress()
{
    sf::Packet outPacket;
    outPacket << PacketType::ClientCreateRoom;
    NetworkLocator::get().sendPacket(outPacket);
    createRoomBtn->setEnabled(false);
}

void GameState::onJoinRoomPress()
{
    int selected = roomList->getSelectedItemIndex();
    if (selected < 0 || selected >= (int)rooms.size()) return;

    sf::Packet outPacket;
    outPacket << PacketType::ClientJoinRoom << rooms[selected].id;
    NetworkLocator::get().sendPacket(outPacket);

    joinRoomBtn->setEnabled(false);
    createRoomBtn->setEnabled(false);
}

void GameState::activate()
{
    BaseState::activate();

    sf::Packet p;
    p << PacketType::ClientRequestRoomList;
    NetworkLocator::get().sendPacket(p);
}

void GameState::pause()
{
}

void GameState::resume()
{
}

void GameState::handleEvent(sf::Event event)
{
    gui.handleEvent(event);
}

void GameState::update(sf::Time deltaTime)
{
}

void GameState::draw()
{
    gui.draw();
}
