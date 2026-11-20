#include "RoomState.h"
#include "GameState.h"  
#include <cmath>

static const char* CREATURE_NAMES[3] = { "CreatureA", "CreatureB", "CreatureC" };  // your own creatures

static const tgui::Color COLOR_ME(0, 140, 0);         
static const tgui::Color COLOR_ROOM(30, 60, 160);     
static const tgui::Color COLOR_LOBBY(40, 40, 40);     
static const tgui::Color COLOR_EVENT(200, 120, 0);    

RoomState::RoomState(StateMachine& machine, sf::RenderWindow& window, bool replace)
    : BaseState{ machine, window, replace }, gui(window)
{
    gui.loadWidgetsFromFile("../assets/form_room.txt");

    title          = gui.get<tgui::Label>("RoomTitle");
    countdownLabel = gui.get<tgui::Label>("CountdownLabel");

    //Player cards
    for (int i = 0; i < 2; i++)
    {
        std::string p = "P" + std::to_string(i + 1);
        names[i]         = gui.get<tgui::Label>(p + "Name");
        statuses[i]      = gui.get<tgui::Label>(p + "Status");
        records[i]       = gui.get<tgui::Label>(p + "Record");
        creatureNames[i] = gui.get<tgui::Label>(p + "CreatureName");
        creaturePics[i]  = gui.get<tgui::Picture>(p + "Creature");
        emotePics[i]     = gui.get<tgui::Picture>(p + "Emoji");
    }

    // creature picker
    for (int i = 0; i < 3; i++)
    {
        creatureBtns[i] = gui.get<tgui::Button>("Creature" + std::to_string(i));
        creatureBtns[i]->setText(CREATURE_NAMES[i]);
        creatureBtns[i]->onPress([this, i] {
            sf::Packet p;
            p << PacketType::ClientSetCreature << i;
            NetworkLocator::get().sendPacket(p);
        });
    }

    // emote buttons
    for (int i = 0; i < 4; i++)
    {
        gui.get<tgui::Button>("Emoji" + std::to_string(i))->onPress([this, i] 
            {
            sf::Packet p;
            p << PacketType::RoomEmote << i;
            NetworkLocator::get().sendPacket(p);
        });
    }

    // Ready
    readyBtn = gui.get<tgui::Button>("ReadyButton");
    readyBtn->onPress([this] 
        {
        sendSimple(PacketType::ClientToggleReady);
        readyBtn->setEnabled(false);              
    });
    
    // Leave
    leaveBtn = gui.get<tgui::Button>("LeaveButton");
    leaveBtn->onPress([this] {
        sendSimple(PacketType::ClientLeaveRoom);
        leaveBtn->setEnabled(false);              
    });

    // Room and lobby chat
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
    chatInput->onReturnKeyPress(&RoomState::onSendPress, this);   // Enter also sends

    // clear the gui-builder placeholder text until the first RoomUpdate arrives
    title->setText("Loading room...");
    countdownLabel->setVisible(false);
}

void RoomState::sendSimple(PacketType type)
{
    sf::Packet p;
    p << type;
    NetworkLocator::get().sendPacket(p);
}

void RoomState::activate()
{
    BaseState::activate();                              
    sendSimple(PacketType::ClientRequestRoomUpdate);    
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
    case PacketType::GameStart:
        roomChat->addLine("[Server] Battle starting!", COLOR_EVENT);
        break;

    case PacketType::RoomLeft:
        m_next = StateMachine::build<GameState>(m_machine, m_window, true);   // back to the lobby
        break;
    }
}

void RoomState::handleRoomUpdate(sf::Packet& packet)
{
    int roomId, serverCountdown, count;
    std::string roomName;
    packet >> roomId >> roomName >> serverCountdown >> count;   // SAME order as the server
    title->setText(roomName);

    int myId = NetworkLocator::get().getClientId();
    bool iAmReady = false;

    for (int i = 0; i < 2; i++)
    {
        // player card
        if (i < count)    
        {
            int id, creatureId, wins, losses;
            std::string name;
            bool ready;
            packet >> id >> name >> creatureId >> ready >> wins >> losses;

            slotPlayerId[i] = id;
            names[i]->setText(id == myId ? name + " (You)" : name);
            statuses[i]->setText(ready ? "READY" : "NOT READY");
            statuses[i]->getRenderer()->setTextColor(ready ? COLOR_ME : tgui::Color(180, 0, 0));
            records[i]->setText("W " + std::to_string(wins) + " / L " + std::to_string(losses));
            bool validCreature = creatureId >= 0 && creatureId < 3;
            creatureNames[i]->setText(validCreature ? CREATURE_NAMES[creatureId] : "?");
            

            if (id == myId) iAmReady = ready;
        }
        else              // empty card
        {
            slotPlayerId[i] = -1;
            names[i]->setText("Waiting for player...");
            statuses[i]->setText("");
            records[i]->setText("");
            creatureNames[i]->setText("");
        }
    }

    // countdown
    bool locked = serverCountdown >= 0;
    if (locked && countdown < 0) countdown = (float)serverCountdown;   // start once
    if (!locked) countdown = -1.0f;
    countdownLabel->setVisible(locked);

    //player local button 
    readyBtn->setText(iAmReady ? "Cancel" : "Ready");
    readyBtn->setEnabled(!locked);
    leaveBtn->setEnabled(!locked);
    for (auto& b : creatureBtns) b->setEnabled(!locked && !iAmReady);  
}

void RoomState::onSendPress()
{
    std::string msg = chatInput->getText().trim().toStdString();
    if (msg.empty()) return;

    bool toRoom = chatTabs->getSelected() == "Room";     // the tab decides where it goes
    sf::Packet p;
    p << (toRoom ? PacketType::RoomChat : PacketType::ChatMessage) << msg;
    NetworkLocator::get().sendPacket(p);
    chatInput->setText("");
}

void RoomState::showEmote(int playerId, int emoteId)
{
    if (emoteId < 0 || emoteId > 3) return;
    for (int i = 0; i < 2; i++)
    {
        if (slotPlayerId[i] != playerId) continue;      
        // temp because no image will crash
        try
        {
            emotePics[i]->getRenderer()->setTexture("../assets/emoji/room_" + std::to_string(emoteId) + ".png");
        }
        catch (const tgui::Exception&)                  
        {
            roomChat->addLine("(emote " + std::to_string(emoteId + 1) + ")", COLOR_EVENT);
            return;
        }
        emotePics[i]->setVisible(true);
        emoteTimer[i] = 2.0f;                           
    }
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

    // start countdown
    if (countdown > 0)                                  
    {
        countdown -= dt;
        int shown = (int)std::ceil(countdown);
        countdownLabel->setText("Battle starts in " + std::to_string(shown > 0 ? shown : 0) + "...");
    }
}