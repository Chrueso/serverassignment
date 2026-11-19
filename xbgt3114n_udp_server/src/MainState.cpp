#include "MainState.h"
#include "PacketType.h"

MainState::MainState(sf::RenderWindow& window)
	: State{ window }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_timesync.txt");

	timeLabel = gui.get<tgui::Label>("TimeLabel");
}

MainState::~MainState() {}

void MainState::activate() {}

void MainState::pause() {}

void MainState::resume() {}

void MainState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);

	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		Server& svr = ServerLocator::get();
		sf::Packet p0;
		p0 << PacketType::DummyPacket;
		p0 << "All";
		svr.sendReliable(p0);
	}
}

void MainState::update(sf::Time deltaTime)
{
	tickTime += deltaTime.asSeconds();

	Server& server = ServerLocator::get();
	float serverTime = server.getTime();
	timeLabel->setText(std::to_string(serverTime));

	circlePosition.x = 400 + 200 * cos(serverTime);
	circlePosition.y = 300 + 200 * sin(serverTime);

	if (tickTime >= tickInterval)
	{
		tickTime -= tickInterval;

		sf::Packet outPacket;
		//outPacket << HeaderType::Unreliable; // unreliable
		outPacket << PacketType::GameStateUpdate;
		outPacket << serverTime;
		outPacket << circlePosition.x << circlePosition.y;

		server.send(outPacket);
	}
}

void MainState::draw()
{
	m_window.clear({ 0,0,128 });
	sf::CircleShape cShape(100.0f);
	cShape.setFillColor(sf::Color::Yellow);
	cShape.setOrigin({ 100.0f, 100.0f });
	cShape.setPosition(circlePosition);
	m_window.draw(cShape);
	gui.draw();
}