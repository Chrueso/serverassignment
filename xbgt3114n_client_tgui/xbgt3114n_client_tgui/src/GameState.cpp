#include "GameState.h"
#include "PacketType.h"

GameState::GameState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: BaseState{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_game.txt");

	gui.get<tgui::Button>("Button1")->onPress(&GameState::sendMessage, this);
	cht = gui.get<tgui::ChatBox>("ChatBox1");
	edt = gui.get<tgui::EditBox>("EditBox1");
}

void GameState::handlePacket(sf::Packet& packet)
{
	PacketType type;
	packet >> type;

	switch (type) {
	case PacketType::UserConnected: {
		std::string playerName;
		packet >> playerName;
		cht->addLine(playerName + " has connected", tgui::Color::Blue);
		break;
	}
	case PacketType::UserDisconnected: {
		std::string playerName;
		packet >> playerName;
		cht->addLine(playerName + " has disconnected", tgui::Color::Blue);
		break;
	}
	case PacketType::ChatMessage: {
		std::string playerName;
		std::string msg;
		int playerId;
		packet >> playerName >> playerId >> msg;
		int clientId = NetworkLocator::get().getClientId();

		cht->addLine(playerName + ": " + msg, playerId == clientId ? tgui::Color::Green : tgui::Color::Black);
		break;
	}
	}
}

void GameState::sendMessage()
{
	// 1. get text from editbox
	std::string msg = edt->getText().toStdString();
	if (msg.length() > 0) {
		sf::Packet outPacket;

		outPacket << PacketType::ChatMessage << msg;
		NetworkLocator::get().sendPacket(outPacket);
		edt->setText("");
	}
	// 2. if text > 0, send out packet
}

void GameState::activate()
{
	BaseState::activate();
}

void GameState::pause() {}

void GameState::resume() {}

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