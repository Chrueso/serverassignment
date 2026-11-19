#pragma once
#include "StateMachine.hpp"
#include "NetworkLocator.h"

// This serves as the base state class
// specifically for XBGT3114N
// All state should subscribe to
// network manager by default
// and unsubscribe on destruction

namespace sf
{
	class Packet;
}

class BaseState : public State
{
	// subscription to network manager
	// returns a handle, cache it
	// to unsubscribe
	size_t receiveHandle;

protected:
	virtual void handlePacket(sf::Packet& packet) = 0;

public:

	BaseState(StateMachine& machine, sf::RenderWindow& window, bool replace = true)
		: State(machine, window, replace)
	{
	}

	virtual ~BaseState();

	void activate() override;
};
