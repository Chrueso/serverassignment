#include "GameState.h"
#include "PacketType.h"

GameState::GameState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_timesync.txt");
	timeLabel = gui.get<tgui::Label>("TimeLabel");
}

GameState::~GameState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void GameState::handlePacket(sf::Packet& packet)
{
	HeaderType reliable;
	PacketType type;
	packet >> reliable >> type;

	// reliable shouldn't be used here, NetworkManager should handle it internally!

	if (type == PacketType::GameStateUpdate)
	{
		float serverTime;
		sf::Vector2f incomingPosition;
		packet >> serverTime >> incomingPosition.x >> incomingPosition.y;

		//circlePosition = incomingPosition\
		
		// TUTORIAL: Snapshot interpolation
		snapshotBuffer.add(serverTime, { incomingPosition });
	}
	else if (type == PacketType::DummyPacket) {
		std::string text;
		packet >> text;

	}
}

void GameState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void GameState::pause() {}

void GameState::resume() {}

void GameState::handleEvent(sf::Event event)
{
	// Pass events to TGUI
	gui.handleEvent(event);
}

void GameState::update(sf::Time deltaTime)
{
	timeLabel->setText(std::to_string(NetworkLocator::get().getTime()));

	// TUTORIAL: Snapshot interpolation
	float netTime = NetworkLocator::get().getTime();
	netTime -= 0.2f;
	// how far back to simulate actually depends on server tickrate and latency
	CircleSnapshot a, b;
	double alpha;

	if (snapshotBuffer.try_get_neighbors(netTime, a, b, alpha)) {
		CircleSnapshot interpolated = CircleSnapshot::interpolate(a, b, alpha);
		circlePosition = interpolated.position;

		snapshotBuffer.prune_older_than(netTime);
	}
}

void GameState::draw()
{
	// Uncomment this when entering snapshot interpolation tutorial
	
	sf::CircleShape cShape(100.0f);
	cShape.setOrigin({ 100.0f, 100.0f });
	cShape.setPosition(circlePosition);
	m_window.draw(cShape);
	

	gui.draw();
}