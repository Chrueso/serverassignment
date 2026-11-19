#include "NetworkManager.h"
#include "PacketType.h"
#include <iostream>

static bool connectPending = false;
static float connectTimeOut = 0.0f;

// need to reset when user disconnected
static bool syncPending = false;
static float nextSynceTime = 0.0f;

size_t NetworkManager::subscribe(PacketHandler handler)
{
	size_t id = nextListenerId++;
	listeners[id] = handler;
	return id;
}

// Removes the listener associated with the ID
void NetworkManager::unsubscribe(size_t id)
{
	listeners.erase(id);
}

bool NetworkManager::connect(std::string name, const sf::IpAddress& ip, unsigned short port)
{
	sf::Packet packet;
	packet << HeaderType::Reliable; // technically should be reliable
	packet << PacketType::ClientJoinRequest;
	packet << name;

	if (transport.send(packet, ip, port) == sf::Socket::Status::Done)
	{
		connectPending = true;
		connectTimeOut = time + 5.0f;

		// in UDP, sending out always succeed as long as the packet
		// size is less than sf::UdpSocket::MaxDatagramSize

		// we wouldn't know whether server received our connection yet!
		std::cout << "Connecting to " << ip << ":" << port << "..." << std::endl;
	}

	// since we sent out request, server should send us back response

	sf::Packet incoming;
	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Socket::Status status = transport.receive(incoming, sender, senderPort);

	// PONDER:
	// What if bad actor sent us something in between our request>response sequence???
	// Need to check if 'sender' and 'senderPort' matches 'ip' and 'port'
	// if not, restart receiving
	// until when? make a timeout!
	//
	// In this example, we assume no such thing as bad stuff. Dream world.

	sf::Packet copyIncoming = incoming;
	HeaderType reliable;
	PacketType type;
	copyIncoming >> reliable >> type;

	if (type == PacketType::ServerJoinResponse)
	{
		transport.setBlocking(false);
		connectPending = false;
		connectedServerIp = ip;
		connectedServerPort = port;
		std::cout << "Connected to " << ip << ":" << port << std::endl;

		handlePacket(incoming);
		syncPending = false;
		nextSynceTime = 0.0f;
		// with the two above, time sync req will trigger in update()
		// on the first time

		/*
		sf::Packet timePacket;
		timePacket << 0;
		timePacket << PacketType::TimeSyncRequest;
		timePacket << time;
		send(timePacket);
		*/
		return true;
	}

	return false;
}

// 0: not connected
// 1: connected
// 2: pending
int NetworkManager::getConnectStatus()
{
	if (connectPending) return 2; // pending
	else return connectedServerIp.has_value() ? 1 : 0;
}

void NetworkManager::send(sf::Packet& packet)
{
	if (!connectedServerIp.has_value())
		return;

	// only send out when connected to server.
	sf::Socket::Status status = transport.send(packet, connectedServerIp.value(), connectedServerPort);

	if (status == sf::Socket::Status::Done)
	{
		std::cout << "Sent!" << std::endl;
	}
}
void NetworkManager::sendReliable(sf::Packet& packet)
{
	if (!connectedServerIp.has_value())
		return;

	// TODO: Implement reliable on top of UDP
	//       Server needs to respond that it had received
}

void NetworkManager::update(float deltaTime)
{
	// only required if using latencysimulationtransport
	transporta.update();
	time += deltaTime;

	if (connectPending && time > connectTimeOut)
	{
		connectPending = false;
		std::cout << "Failed to connect" << std::endl;
	}

	// if we are not connected to server, don't proceed.
	if (!connectedServerIp.has_value())
		return;

	sf::Packet incoming;
	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Socket::Status status = transport.receive(incoming, sender, senderPort);

	// partial send/receive in UDP connection
	if (status == sf::Socket::Status::Done)
	{
		// only handle received packet if we got it from the connected server
		if (sender.value() == connectedServerIp.value() && connectedServerPort == senderPort)
		{
			handlePacket(incoming);
		}
		else
		{
			// something else is sending us data. Do nothing
		}
	}

	// TODO: Reliable resending after some period have passed.
	//       Each reliable packet has timer

	// TODO: Periodic time synchronization
	// if time sync fails several times in a row, then assume server is dead
	if (time > nextSynceTime) {
		if (syncPending) {
			// already trying to sync but no response from server
			std::cout << "Time suync did not receive response!" << std::endl;
			// increment attempt
			// if attempt > max_attempt
			// drop connection
			// note: rememeber to reseset attempt when getting time sync response
		}
		syncPending = true;
		nextSynceTime = time + 5.0f; // set eagerly in case we did not get response from server
		sf::Packet timePacket;
		timePacket << HeaderType::Unreliable;
		timePacket << PacketType::TimeSyncRequest;
		timePacket << time;
		send(timePacket);
	}
}

void NetworkManager::handlePacket(sf::Packet& packet)
{
	sf::Packet copy = sf::Packet(packet);

	HeaderType reliable;
	//PacketType type;
	copy >> reliable;
	bool continueOn = true;
	if (reliable == HeaderType::Reliable)
	{
		// TODO: Handle reliable ack
		// TODO: Do not reprocess duplicate packets
		//       (server may resend packet again because it didn't receive ack from client!)
		std::uint16_t sequenceId;
		copy >> sequenceId;

		// send Ack if it is not future id
		//std::cout << nextSequenceId << " | " << sequenceId << std::endl;
		if (sequenceId <= nextSequenceId) {
			sf::Packet ackPacket;
			ackPacket << HeaderType::Ack << sequenceId;
			send(ackPacket);

			std::cout << "Send ack:" << sequenceId << std::endl;
		}
		

		// only process packets where sequenceId matches exactly what we want
		// discard everything else, this includes RUDP with future sequence id
		//	Technically should send ack and buffer then instead of waste bandwidth
		if (sequenceId == nextSequenceId) {
			// sequenceId is exactly nextSequenceId, so process it
			// incremeent because we expect next RUDP having higher Id
			nextSequenceId++;
		}
		else {
			continueOn = false;
		}

	}
	if (!continueOn) return;
	// read packet type after processing header type
	// because if it is reliable, there's extra data before PacketType
	PacketType type;
	copy >> type;

	// if the response is time synchronization, handle it internally and not send out
	if (type == PacketType::TimeSyncResponse)
	{
		// TUTORIAL
		float serverTime, sentTime;
		copy >> serverTime >> sentTime;
		//time sync req expects a responce, making it a good candidate to measure round trip time (rtt)
		rtt = sentTime - time;
		// to get more precise server time, add rtt/2 to serverTime
		time = serverTime + rtt / 2.0f;
		syncPending = false;
		nextSynceTime = time + 5.0f; // time has changed so we reset the nextSyncTime
	}
	else
	{
		// PacketHandler signature is void(sf::Packet) which means
		// the 'packet' is passed by value. Each subscriber gets a copy
		// of the packet.
		// This is to ensure that each subscriber can do their own packet
		// unpacking without affecting other subscribers.
		for (auto const& [id, handler] : listeners)
		{
			handler(packet);
		}
	}
}