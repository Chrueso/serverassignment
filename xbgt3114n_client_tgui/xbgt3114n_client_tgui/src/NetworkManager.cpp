#include "NetworkManager.h"

bool NetworkManager::connect(sf::IpAddress& ipAddr, unsigned short port) {

	if (socket.connect(ipAddr, port) == sf::Socket::Status::Done) {
		// we are connected to server
		// send client join req, and wait for server join response
		
		// this makes send and receive not blocking the thread
		socket.setBlocking(false);
		return true;
	}
	socket.setBlocking(true);
	return false;
}

int NetworkManager::getClientId() {
	return clientId;
}

void NetworkManager::sendPacket(sf::Packet& p) {
	// queue this pakcet for send which happens in update
	sendQueue.push(p);
}

void NetworkManager::update() {
	// technically should check if socket is connected or not before continuing
	if (socket.isBlocking()) { return; }
	// process sendQueue
	while (!sendQueue.empty()) {
		if (socket.send(sendQueue.front()) == sf::Socket::Status::Done) {
			// packet received from the other side
			sendQueue.pop();
		}
		else {
			// packet still not done sending
			// break from loop
			break;
		}

		// if packet successfuly received from the other side
		// then we continue sending the next one until
		// one of the packet is not successfully sent within one check
	}
	// process receiving data
	sf::Packet inPacket;
	if (socket.receive(inPacket) == sf::Socket::Status::Done) {
		handlePacketInternal(inPacket);
	}

}

void NetworkManager::handlePacketInternal(sf::Packet& inPacket) {
	// loop through all listeners
	// call the function, passing in copy of packet
	// why copies instead of reference? because each listener may want to handle the packet differently

	for (auto const& [id, fn] : listeners) {
		// fn is void(sf::Packet) so it will pass by value
		// hence copy is automatically made for every listeners
		fn(inPacket);
	}
}

size_t NetworkManager::subscribe(PacketHandler fn) {
	size_t id = nextId++;
	listeners[id] = fn;

	return id;

}

void NetworkManager::unsubscribe(size_t id) {
	listeners.erase(id);
}

void NetworkManager::setClientId(int id) {
	clientId = id;
}