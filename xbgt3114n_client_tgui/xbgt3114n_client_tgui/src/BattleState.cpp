#include "BattleState.h"
#include "RoomState.h"
#include "GameState.h"
#include "Creatures.h"
#include <algorithm>

static const tgui::Color COLOR_ME(0, 140, 0);
static const tgui::Color COLOR_ROOM(30, 60, 160);
static const tgui::Color COLOR_LOBBY(40, 40, 40);
static const tgui::Color COLOR_EVENT(200, 120, 0);
static const tgui::Color COLOR_BATTLE(140, 40, 160);

// Attack> Break, Break > Defend, Defend > Attack
static std::string describeRound(int mine, int theirs, const std::string& enemy)
{
    if (mine == theirs)
    {
        if (mine == 0) return "You both attacked!";
        if (mine == 1) return "You both defended. Nothing happened.";
        return "You both broke through!";
    }
    if (mine == 0 && theirs == 1) return enemy + " blocked your Attack!";
    if (mine == 0 && theirs == 2) return "Your Attack hit " + enemy + " before their Break!";
    if (mine == 1 && theirs == 0) return "You blocked " + enemy + "'s Attack!";
    if (mine == 1 && theirs == 2) return enemy + "'s Break smashed your Defend!";
    if (mine == 2 && theirs == 0) return enemy + "'s Attack hit you before your Break!";
    if (mine == 2 && theirs == 1) return "Your Break smashed " + enemy + "'s Defend!";
    return "";
}

BattleState::BattleState(StateMachine& machine, sf::RenderWindow& window, bool replace)
    : BaseState{ machine, window, replace }, gui(window)
{
    gui.loadWidgetsFromFile("../assets/form_battle.txt");

    roundLabel  = gui.get<tgui::Label>("RoundLabel");
    resultLabel = gui.get<tgui::Label>("ResultLabel");
    statusLabel = gui.get<tgui::Label>("StatusLabel");

    for (int i = 0; i < 2; i++)
    {
        std::string p = "P" + std::to_string(i + 1);
        names[i]           = gui.get<tgui::Label>(p + "Name");
        moveLabels[i]      = gui.get<tgui::Label>(p + "Move");
        creatureNames[i]   = gui.get<tgui::Label>(p + "CreatureName");
        creaturePics[i]    = gui.get<tgui::Picture>(p + "Creature");
        hpBars[i]          = gui.get<tgui::ProgressBar>(p + "HpBar");
        emotePics[i]       = gui.get<tgui::Picture>(p + "Emote");
        creaturePics[i]->setVisible(false);
        moveLabels[i]->setText("");
    }

    const char* moveButtonNames[3] = { "AttackButton", "DefendButton", "BreakButton" };
    for (int i = 0; i < 3; i++)
    {
        moveBtns[i] = gui.get<tgui::Button>(moveButtonNames[i]);
        moveBtns[i]->onPress([this, i] { chooseMove(i); });
    }

    for (int i = 0; i < 4; i++)
    {
        emoteBtns[i] = gui.get<tgui::Button>("Emote" + std::to_string(i));
        emoteBtns[i]->onPress([this, i] {
            sf::Packet p;
            p << PacketType::BattleEmote << NetworkLocator::get().getRoomId() << i;
            NetworkLocator::get().sendPacket(p);
        });
    }

    roomChat  = gui.get<tgui::ChatBox>("RoomChatBox");
    lobbyChat = gui.get<tgui::ChatBox>("LobbyChatBox");
    chatTabs  = gui.get<tgui::Tabs>("ChatTabs");
    chatInput = gui.get<tgui::EditBox>("ChatInput");
    chatTabs->add("Room");
    chatTabs->add("Lobby", false);
    chatTabs->onTabSelect([this](const tgui::String& tab) {
        roomChat->setVisible(tab == "Room");
        lobbyChat->setVisible(tab == "Lobby");
    });
    gui.get<tgui::Button>("SendButton")->onPress(&BattleState::onSendPress, this);
    chatInput->onReturnKeyPress(&BattleState::onSendPress, this);

    resultPanel = gui.get<tgui::Panel>("ResultPanel");
    resultTitle = gui.get<tgui::Label>("ResultTitle");
    resultInfo  = gui.get<tgui::Label>("ResultInfo");
    gui.get<tgui::Button>("BackButton")->onPress([this] {
        m_next = StateMachine::build<RoomState>(m_machine, m_window, true);
    });

    names[0]->setText(NetworkLocator::get().getPlayerName() + " (You)");
    names[1]->setText("Opponent");
    statusLabel->setText("Loading battle...");
    setChoosing(false);
}

void BattleState::activate()
{
    BaseState::activate();

    sf::Packet p;
    p << PacketType::ClientRequestBattleUpdate << NetworkLocator::get().getRoomId();
    NetworkLocator::get().sendPacket(p);
}

int BattleState::slotOf(int playerId)
{
    if (playerId == NetworkLocator::get().getClientId()) return 0;
    if (playerIds[1] == -1) playerIds[1] = playerId;
    return playerId == playerIds[1] ? 1 : -1;
}

void BattleState::handlePacket(sf::Packet& packet)
{
    PacketType type;
    packet >> type;
    int myId = NetworkLocator::get().getClientId();

    switch (type)
    {
    case PacketType::BattleUpdate:
        handleBattleUpdate(packet);
        break;

    case PacketType::MoveLocked: {
        int id;
        packet >> id;
        if (slotOf(id) == 1)
        {
            opponentLocked = true;
            moveLabels[1]->setText("Ready");
            if (moveBtns[0]->isEnabled())
                statusLabel->setText(playerNames[1] + " has chosen!");
        }
        break;
    }
    case PacketType::RoundResult:
        handleRoundResult(packet);
        break;

    case PacketType::BattleEnd:
        handleBattleEnd(packet);
        break;

    case PacketType::BattleEmote: {
        std::string name; int id, emoteId;
        packet >> name >> id >> emoteId;
        showEmote(id, emoteId);
        break;
    }
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
    case PacketType::RoomLeft: {
        int leftId = -1;
        packet >> leftId;
        if (leftId == myId)
        {
            NetworkLocator::get().setRoomId(-1);
            m_next = StateMachine::build<GameState>(m_machine, m_window, true);
        }
        else if (!battleOver)
        {
            battleOver = true;
            setChoosing(false);
            roomChat->addLine("[Server] " + playerNames[1] + " left the battle", COLOR_EVENT);
            showResultPanel("Opponent left", playerNames[1] + " left the battle.", COLOR_EVENT);
        }
        break;
    }
    }
}

void BattleState::handleBattleUpdate(sf::Packet& packet)
{
    int round, count;
    if (!(packet >> round >> count)) return;
    roundLabel->setText("Round " + std::to_string(round));

    for (int i = 0; i < count; i++)
    {
        int id, creature, hp, maxHp;
        std::string name;
        packet >> id >> name >> creature >> hp >> maxHp;

        int slot = slotOf(id);
        if (slot < 0) continue;
        playerIds[slot] = id;
        playerNames[slot] = name;
        names[slot]->setText(slot == 0 ? name + " (You)" : name);
        creatureNames[slot]->setText(creatureName(creature, "?"));
        showCreaturePicture(creaturePics[slot], creature);
        setHp(slot, hp, maxHp);
    }

    if (!battleOver)
    {
        setChoosing(true);
        statusLabel->setText("Choose your move");
    }
}

void BattleState::handleRoundResult(sf::Packet& packet)
{
    int round, count;
    if (!(packet >> round >> count)) return;

    int moves[2] = { -1, -1 };
    int damage[2] = { 0, 0 };
    for (int i = 0; i < count; i++)
    {
        int id, move, damageTaken, hp;
        packet >> id >> move >> damageTaken >> hp;
        int slot = slotOf(id);
        if (slot < 0) continue;
        moves[slot] = move;
        damage[slot] = damageTaken;
        setHp(slot, hp, (int)hpBars[slot]->getMaximum());
        moveLabels[slot]->setText(moveName(move));
    }

    std::string text = describeRound(moves[0], moves[1], playerNames[1])
                     + "  (You -" + std::to_string(damage[0])
                     + ", " + playerNames[1] + " -" + std::to_string(damage[1]) + ")";
    resultLabel->setText(text);
    roomChat->addLine("[Round " + std::to_string(round) + "] " + text, COLOR_BATTLE);

    roundLabel->setText("Round " + std::to_string(round + 1));
    opponentLocked = false;
    if (!battleOver)
    {
        setChoosing(true);
        statusLabel->setText("Choose your move");
    }
}

void BattleState::handleBattleEnd(sf::Packet& packet)
{
    int winnerId = -1;
    packet >> winnerId;
    battleOver = true;
    setChoosing(false);
    statusLabel->setText("Battle over");

    if (winnerId == -1)
        showResultPanel("DRAW", "Nobody wins this time.", COLOR_EVENT);
    else if (winnerId == NetworkLocator::get().getClientId())
        showResultPanel("VICTORY!", "You beat " + playerNames[1] + "!", COLOR_ME);
    else
        showResultPanel("DEFEAT", playerNames[1] + " wins the battle.", tgui::Color(180, 0, 0));
}

void BattleState::showResultPanel(const std::string& title, const std::string& info, tgui::Color color)
{
    resultTitle->setText(title);
    resultTitle->getRenderer()->setTextColor(color);
    resultInfo->setText(info);
    resultPanel->setVisible(true);
    resultPanel->moveToFront();
}

void BattleState::chooseMove(int move)
{
    if (battleOver) return;

    sf::Packet p;
    p << PacketType::ClientChooseMove << NetworkLocator::get().getRoomId() << move;
    NetworkLocator::get().sendPacket(p);

    setChoosing(false);
    moveLabels[0]->setText(moveName(move));
    moveLabels[1]->setText(opponentLocked ? "Ready" : "...");
    statusLabel->setText("Waiting for " + (playerNames[1].empty() ? std::string("opponent") : playerNames[1]) + "...");
}

void BattleState::setChoosing(bool canChoose)
{
    for (auto& b : moveBtns) b->setEnabled(canChoose && !battleOver);
}

void BattleState::setHp(int slot, int hp, int maxHp)
{
    if (maxHp <= 0) maxHp = 1;
    hp = std::clamp(hp, 0, maxHp);

    hpBars[slot]->setMinimum(0);
    hpBars[slot]->setMaximum((unsigned int)maxHp);
    hpBars[slot]->setValue((unsigned int)hp);
    hpBars[slot]->setText(std::to_string(hp) + " / " + std::to_string(maxHp));

    float ratio = (float)hp / maxHp;
    tgui::Color fill = ratio > 0.5f ? tgui::Color(60, 179, 113)
                     : ratio > 0.25f ? tgui::Color(240, 160, 30)
                     : tgui::Color(210, 50, 40);
    hpBars[slot]->getRenderer()->setFillColor(fill);
}

void BattleState::showEmote(int playerId, int emoteId)
{
    if (emoteId < 0 || emoteId > 3) return;
    int slot = slotOf(playerId);
    if (slot < 0) return;

    emotePics[slot]->getRenderer()->setTexture(
        tgui::Texture("../assets/emote/room_" + std::to_string(emoteId) + ".png"));
    emotePics[slot]->setVisible(true);
    emotePics[slot]->moveToFront();
    emoteTimer[slot] = 2.0f;
}

void BattleState::onSendPress()
{
    std::string msg = chatInput->getText().trim().toStdString();
    if (msg.empty()) return;

    sf::Packet p;
    if (chatTabs->getSelected() == "Room")
        p << PacketType::RoomChat << NetworkLocator::get().getRoomId() << msg;
    else
        p << PacketType::ChatMessage << msg;
    NetworkLocator::get().sendPacket(p);
    chatInput->setText("");
}

void BattleState::update(sf::Time deltaTime)
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
