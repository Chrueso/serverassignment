#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional> // for storing function
#include <map> // for storing subscription


//aliasing the std::function
using PacketHandler = std::function<void(sf::Packet)>;

class NetworkManager
{
private:
	sf::TcpSocket socket;
	int clientId = -1; // received from server when succesfully joined
	int roomId = -1;
	std::string playerName;

	std::queue<sf::Packet> sendQueue;
	// key is subscription id
	// value is the function to call
	std::map<size_t, PacketHandler> listeners;
	// internal id for subscriptions
	// every single subscription will increase this number/id
	size_t nextId = 0;

	void handlePacketInternal(sf::Packet& inPacket);

public:
	bool connect(sf::IpAddress& ipAddr, unsigned short port);

	int getClientId();
	void setClientId(int id);

	int getRoomId() { return roomId; }
	void setRoomId(int id) { roomId = id; }
	const std::string& getPlayerName() { return playerName; }
	void setPlayerName(const std::string& name) { playerName = name; }

	void sendPacket(sf::Packet& p);

	void update();

	size_t subscribe(PacketHandler fn);
	void unsubscribe(size_t handle);

	// This is an implementation of a non-blocking network manager
	// Initially it is blocking, and only unblocks when connection is successful
	// It MUST revert to blocking when disconnected from server.
	//
	// Responsibilities:
	// 1. Handles TcpSocket connectivity, including caching its clientId
	//      Note: server MUST provide clientId
	//
	// 2. Handles buffered packet sending.
	//      Send requests are queued and sent out in FIFO order, but only
	//      when the older one is received by the server.
	//
	//      Note: this requires an update() function to be called in
	//            main Application loop.
	//
	// 3. Handles receiving packet and forwarding to listeners
};
