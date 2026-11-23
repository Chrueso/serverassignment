#include "RoomState.h"
#include "GameState.h"
#include "BattleState.h"
#include "Creatures.h"

static const tgui::Color COLOR_ME(0, 140, 0);
static const tgui::Color COLOR_ROOM(30, 60, 160);
static const tgui::Color COLOR_LOBBY(40, 40, 40);
static const tgui::Color COLOR_EVENT(200, 120, 0);
static const tgui::Color COLOR_NOT_READY(180, 0, 0);

RoomState::RoomState(StateMachine& machine, sf::RenderWindow& window, bool replace)
    : BaseState{ machine, window, replace }, gui(window)
{
    gui.loadWidgetsFromFile("../assets/form_room.txt");

    title          = gui.get<tgui::Label>("RoomTitle");
    countdownLabel = gui.get<tgui::Label>("CountdownLabel");

    for (int i = 0; i < 2; i++)
    {
        std::string p = "P" + std::to_string(i + 1);
        names[i]         = gui.get<tgui::Label>(p + "Name");
        statuses[i]      = gui.get<tgui::Label>(p + "Status");
        records[i]       = gui.get<tgui::Label>(p + "Record");
        creatureNames[i] = gui.get<tgui::Label>(p + "CreatureName");
        creaturePics[i]  = gui.get<tgui::Picture>(p + "Creature");
        emotePics[i]     = gui.get<tgui::Picture>(p + "Emote");
    }

    for (int i = 0; i < 3; i++)
    {
        creatureBtns[i] = gui.get<tgui::Button>("Creature" + std::to_string(i));
        creatureBtns[i]->setText(CREATURE_NAMES[i]);
        creatureBtns[i]->onPress([this, i] {
            sf::Packet p;
            p << PacketType::ClientSetCreature << NetworkLocator::get().getRoomId() << i;
            NetworkLocator::get().sendPacket(p);
        });
    }

    for (int i = 0; i < 4; i++)
    {
        emoteBtns[i] = gui.get<tgui::Button>("Emote" + std::to_string(i));
        emoteBtns[i]->onPress([this, i] {
            sf::Packet p;
            p << PacketType::RoomEmote << NetworkLocator::get().getRoomId() << i;
            NetworkLocator::get().sendPacket(p);
        });
    }

    readyBtn = gui.get<tgui::Button>("ReadyButton");
    readyBtn->onPress([this] { sendRoomPacket(PacketType::ClientToggleReady); });

    leaveBtn = gui.get<tgui::Button>("LeaveButton");
    leaveBtn->onPress([this] { sendRoomPacket(PacketType::ClientLeaveRoom); });

    roomChat  = gui.get<tgui::ChatBox>("RoomChatBox");
    lobbyChat = gui.get<tgui::ChatBox>("LobbyChatBox");
    chatTabs  = gui.get<tgui::Tabs>("ChatTabs");
    chatInput = gui.get<tgui::EditBox>("ChatInput");
    sendBtn   = gui.get<tgui::Button>("SendButton");

    chatTabs->add("Room");
    chatTabs->add("Lobby", false);
    chatTabs->onTabSelect([this](const tgui::String& tab) {
        roomChat->setVisible(tab == "Room");
        lobbyChat->setVisible(tab == "Lobby");
    });
    sendBtn->onPress(&RoomState::onSendPress, this);
    chatInput->onReturnKeyPress(&RoomState::onSendPress, this);

    title->setText("Room");
    countdownLabel->setVisible(false);
    refreshCards();
}

void RoomState::sendRoomPacket(PacketType type)
{
    sf::Packet p;
    p << type << NetworkLocator::get().getRoomId();
    NetworkLocator::get().sendPacket(p);
}

void RoomState::activate()
{
    BaseState::activate();

    sf::Packet listReq;
    listReq << PacketType::ClientRequestRoomList;
    NetworkLocator::get().sendPacket(listReq);

    sendRoomPacket(PacketType::ClientRequestRoomUpdate);
}

void RoomState::handlePacket(sf::Packet& packet)
{
    PacketType type;
    packet >> type;
    int myId = NetworkLocator::get().getClientId();

    switch (type)
    {
    case PacketType::RoomUpdate:
        handleRoomUpdate(packet);
        break;

    case PacketType::RoomList:
        handleRoomList(packet);
        break;

    case PacketType::RoomChat: {
        std::string name, msg; int id;
        packet >> name >> id >> msg;
        roomChat->addLine(name + ": " + msg, id == myId ? COLOR_ME : COLOR_ROOM);
        break;
    }
    case PacketType::ChatMessage: {
        std::string name, msg; int id;
        packet >> name >> id >> msg;
        lobbyChat->addLine(name + ": " + msg, id == myId ? COLOR_ME : COLOR_LOBBY);
        break;
    }
    case PacketType::UserConnected:
    case PacketType::UserDisconnected: {
        std::string name;
        packet >> name;
        bool joined = (type == PacketType::UserConnected);
        lobbyChat->addLine("[Server] " + name + (joined ? " joined" : " left"), COLOR_EVENT);
        break;
    }
    case PacketType::RoomEmote: {
        std::string name; int id, emoteId;
        packet >> name >> id >> emoteId;
        showEmote(id, emoteId);
        break;
    }
    case PacketType::RoomLeft: {
        int leftId = -1;
        packet >> leftId;
        if (leftId == myId)
        {
            NetworkLocator::get().setRoomId(-1);
            m_next = StateMachine::build<GameState>(m_machine, m_window, true);
        }
        else
        {
            roomChat->addLine("[Server] " + (otherName.empty() ? std::string("The other player") : otherName)
                              + " left the room", COLOR_EVENT);
            clearOtherPlayer();
        }
        break;
    }
    case PacketType::GameStart:
        m_next = StateMachine::build<BattleState>(m_machine, m_window, true);
        break;
    }
}

void RoomState::handleRoomUpdate(sf::Packet& packet)
{
    int id, creature, wins, losses;
    std::string name;
    bool ready;
    if (!(packet >> id >> name >> ready >> creature >> wins >> losses)) return;

    if (id == NetworkLocator::get().getClientId())
    {
        myReady = ready;
        myCreature = creature;
        myWins = wins;
        myLosses = losses;
    }
    else
    {
        otherId = id;
        otherName = name;
        otherReady = ready;
        otherCreature = creature;
        otherWins = wins;
        otherLosses = losses;
    }
    refreshCards();
}

void RoomState::handleRoomList(sf::Packet& packet)
{
    int myRoom = NetworkLocator::get().getRoomId();
    int count = 0;
    packet >> count;
    for (int i = 0; i < count; i++)
    {
        int id, players, maxPlayers;
        std::string name;
        bool inBattle;
        packet >> id >> name >> players >> maxPlayers >> inBattle;
        if (id != myRoom) continue;

        title->setText(name);
        if (players >= 2 && otherId == -1)
            sendRoomPacket(PacketType::ClientRequestRoomUpdate);
        else if (players < 2 && otherId != -1)
            clearOtherPlayer();
    }
}

void RoomState::clearOtherPlayer()
{
    otherId = -1;
    otherName.clear();
    otherReady = false;
    otherCreature = -1;
    otherWins = otherLosses = 0;
    refreshCards();
}

static std::string creatureText(int c)
{
    return creatureName(c);
}

void RoomState::showCreature(int card, int creature)
{
    showCreaturePicture(creaturePics[card], creature);
}

void RoomState::refreshCards()
{
    names[0]->setText(NetworkLocator::get().getPlayerName() + " (You)");
    statuses[0]->setText(myReady ? "READY" : "NOT READY");
    statuses[0]->getRenderer()->setTextColor(myReady ? COLOR_ME : COLOR_NOT_READY);
    creatureNames[0]->setText(creatureText(myCreature));
    showCreature(0, myCreature);
    records[0]->setText(myWins < 0 ? "W - / L -"
                        : "W " + std::to_string(myWins) + " / L " + std::to_string(myLosses));

    if (otherId == -1)
    {
        names[1]->setText("Waiting for player...");
        statuses[1]->setText("");
        creatureNames[1]->setText("");
        records[1]->setText("");
        showCreature(1, -1);
    }
    else
    {
        names[1]->setText(otherName);
        statuses[1]->setText(otherReady ? "READY" : "NOT READY");
        statuses[1]->getRenderer()->setTextColor(otherReady ? COLOR_ME : COLOR_NOT_READY);
        creatureNames[1]->setText(creatureText(otherCreature));
        showCreature(1, otherCreature);
        records[1]->setText("W " + std::to_string(otherWins) + " / L " + std::to_string(otherLosses));
    }

    readyBtn->setText(myReady ? "Cancel" : "Ready");
    readyBtn->setEnabled(myCreature != -1);
    for (int i = 0; i < 3; i++)
    {
        creatureBtns[i]->setText(i == myCreature ? std::string(CREATURE_NAMES[i]) + " (chosen)"
                                                 : std::string(CREATURE_NAMES[i]));
        creatureBtns[i]->setEnabled(!myReady);
    }

    bool alone = (otherId == -1);
    for (auto& b : emoteBtns) b->setEnabled(!alone);
}

void RoomState::onSendPress()
{
    std::string msg = chatInput->getText().trim().toStdString();
    if (msg.empty()) return;

    sf::Packet p;
    if (chatTabs->getSelected() == "Room")
    {
        if (otherId == -1)
        {
            roomChat->addLine("(Nobody else is in the room yet)", COLOR_EVENT);
            return;
        }
        p << PacketType::RoomChat << NetworkLocator::get().getRoomId() << msg;
    }
    else
        p << PacketType::ChatMessage << msg;
    NetworkLocator::get().sendPacket(p);
    chatInput->setText("");
}

void RoomState::showEmote(int playerId, int emoteId)
{
    if (emoteId < 0 || emoteId > 3) return;
    int card = (playerId == NetworkLocator::get().getClientId()) ? 0
             : (playerId == otherId ? 1 : -1);
    if (card == -1) return;

    emotePics[card]->getRenderer()->setTexture(
        tgui::Texture("../assets/emote/room_" + std::to_string(emoteId) + ".png"));
    emotePics[card]->setVisible(true);
    emoteTimer[card] = 2.0f;
}

void RoomState::update(sf::Time deltaTime)
{
    float dt = deltaTime.asSeconds();
    for (int i = 0; i < 2; i++)
    {
        if (emoteTimer[i] > 0)
        {
            emoteTimer[i] -= dt;
            if (emoteTimer[i] <= 0) emotePics[i]->setVisible(false);
        }
    }
}
