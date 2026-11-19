#include "GameState.h"
#include "PacketType.h"

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
    roomList->addColumn("Room", 140);
    roomList->addColumn("Host", 80);
    roomList->addColumn("Players", 60);
    roomList->addColumn("Status", 60);

    roomList->onItemSelect([this](int index) { updateJoinButton(); });
    roomList->onDoubleClick([this](int index) { onJoinRoomPress(); });

    createRoomBtn->onPress(&GameState::onCreateRoomPress, this);
    joinRoomBtn->onPress(&GameState::onJoinRoomPress, this);

    // Join stays disabled until a joinable room is selected (see updateJoinButton)
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
            cht->addLine("[Server] You joined room " + std::to_string(roomId), tgui::Color::Blue);

            break;
        }
    case PacketType::RoomJoinFailed:
        {
            std::string reason;
            packet >> reason;
            cht->addLine("[Server] Could not join: " + reason, tgui::Color::Red);
            createRoomBtn->setEnabled(true); // let them try again
            updateJoinButton();
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
        std::string name, host;
        packet >> roomInfo.id >> name >> host >> roomInfo.players >> roomInfo.maxPlayers >> roomInfo.inBattle;

        std::string status = roomInfo.inBattle
                                 ? "In Battle"
                                 : (roomInfo.players >= roomInfo.maxPlayers ? "Full" : "Waiting");
        std::string players = std::to_string(roomInfo.players) + "/" + std::to_string(roomInfo.maxPlayers);

        roomList->addItem({name, host, players, status}); // one cell per column
        rooms.push_back(roomInfo); // rooms[row] = details of that row

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
    int sel = roomList->getSelectedItemIndex();
    if (sel < 0 || sel >= (int)rooms.size()) return; // nothing selected

    sf::Packet outPacket;
    outPacket << PacketType::ClientJoinRoom << rooms[sel].id;
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
