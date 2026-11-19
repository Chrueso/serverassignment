#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"
#include "SnapshotBuffer.h"
struct CircleSnapshot {
	sf::Vector2f position;

	static CircleSnapshot interpolate(const CircleSnapshot& a, const CircleSnapshot& b, float alpha) {
		sf::Vector2f interPos = a.position * (1.0f - alpha) + b.position * alpha;
		CircleSnapshot data;
		data.position = interPos;

		return data;
	}
};
class GameState : public State
{
	tgui::Gui gui; // important to be first

	tgui::Label::Ptr timeLabel;
	size_t receiveHandle;

	sf::Vector2f circlePosition;

	SnapshotBuffer<CircleSnapshot> snapshotBuffer;

	void handlePacket(sf::Packet& packet);

public:

	GameState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
	~GameState();

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};
