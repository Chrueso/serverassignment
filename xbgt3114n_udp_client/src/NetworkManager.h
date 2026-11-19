#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional>
#include <map>
#include "LatencySimulationTransport.h"
// aliasing the std::function
using PacketHandler = std::function<void(sf::Packet)>;

class NetworkManager
{
private:
	LatencySimulationTransport transporta;
	sf::UdpSocket transport;
	std::optional<sf::IpAddress> connectedServerIp;
	unsigned short connectedServerPort;

	std::map<size_t, PacketHandler> listeners;
	size_t nextListenerId = 0;

	void handlePacket(sf::Packet& packet);

	int userId = -1;
	float time = 0.0f;
	float rtt = 0.0f;

	std::uint16_t nextSequenceId = 0; // This needs to be reset on joining server

public:

	float getRoundTripTime() const { return rtt; }
	float getLatency() const { return rtt * 0.5f; }
	float getTime() const { return time; }
	int getId() const { return userId; }
	void setId(int newId) { userId = newId; }

	size_t subscribe(PacketHandler handler);
	void unsubscribe(size_t id);

	bool connect(std::string name, const sf::IpAddress& ip, unsigned short port);

	int getConnectStatus();

	void send(sf::Packet& packet);
	void sendReliable(sf::Packet& packet);
	void update(float deltaTime);
};