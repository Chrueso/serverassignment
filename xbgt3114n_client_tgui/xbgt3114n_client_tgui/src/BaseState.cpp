#include "BaseState.h"
#include <iostream>

BaseState::~BaseState()
{
	// unsubscribe from network manager
	// by passing receiveHandle
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void BaseState::activate()
{
	// subscribe to network manager
	// and assign receiveHandle

	// handlePacket signature is acutually
	//		void(BaseState* this, sf::Packet packet)
	// and NOT
	//		void(sf:::Packet packet)
	// which is what is expected by NetworkManager subscribe
	// so to make it fit, we box the handle packet fn
	// into an anonymous fn with matching signature
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet inPacket) {
		handlePacket(inPacket);
		});
}