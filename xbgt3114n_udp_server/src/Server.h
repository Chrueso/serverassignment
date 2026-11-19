#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional>
#include <map>

struct PendingPacket {
	sf::Packet packet;
	sf::Clock timer;
	int attempts = 0;
};

class PlayerInstance
{
private:
	int id;
	std::string name;
	float lastPacketTime = -1.0f;

	sf::IpAddress ipAddress;
	unsigned short port;

	// keep track of pending RUDP per player
	std::map<std::uint16_t, PendingPacket> pendingReliablePackets;
	std::uint16_t nextSequenceId = 0;

public:

	std::uint16_t getSequenceIdThenIncrement() { return nextSequenceId++; }

	std::map<std::uint16_t, PendingPacket>& getPendingPacketMap() { return pendingReliablePackets; }

	void receiveAckFor(std::uint16_t sequenceId) {
		pendingReliablePackets.erase(sequenceId);
	}

	void addPendingPacket(sf::Packet& pPacket, std::uint16_t seqId) {
		pendingReliablePackets[seqId] = {pPacket, sf::Clock(), 0 };
	}

	PlayerInstance(sf::IpAddress ip, unsigned short port, int id, std::string name, float lastPacketTime)
		: id(id)
		, name(name)
		, lastPacketTime(lastPacketTime)
		, ipAddress(ip)
		, port(port)
	{
	}

	float getLastPacketTime()
	{
		return lastPacketTime;
	}

	void updateLastPacketTime(float newTime)
	{
		lastPacketTime = newTime;
	}

	sf::IpAddress getIpAddress() const
	{
		return ipAddress;
	}

	unsigned short getPort() const
	{
		return port;
	}
};

class Server
{
private:
	sf::UdpSocket socket;

	std::map<size_t, PlayerInstance> players;

	// C++17 allows us to define static member variables in header using inline
	inline static int idIncrementor = 0;
	inline static int playerCount = 0;

	float time = 0.0f;


public:

	float getTime() const
	{
		return time;
	}

	void host(unsigned short port);
	void send(sf::Packet& packet);
	void sendTo(sf::Packet& packet, int id);
	void sendTo(sf::Packet& packet, PlayerInstance& playerRef);

	void sendReliable(sf::Packet& packet);
	void sendToReliable(sf::Packet& packet, int id);
	void sendToReliable(sf::Packet& packet, PlayerInstance& playerRef);

	void update(float deltaTime);
};