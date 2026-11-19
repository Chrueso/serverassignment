#include <iostream>
#include "Server.h"
#include "PacketType.h"



void Server::host(unsigned short port)
{
	if (socket.bind(port) == sf::Socket::Status::Done)
	{
		socket.setBlocking(false);
		std::cout << "Hosted server with port: " << port << std::endl;
	}
}

void Server::send(sf::Packet& packet)
{
	for (auto& p : players)
	{
		PlayerInstance& pi = p.second;
		//sf::Socket::Status status = socket.send(packet, pi.getIpAddress(), pi.getPort());
		sendTo(packet, pi);
	}
}

void Server::sendTo(sf::Packet& packet, int id)
{
	auto it = players.find(id);

	if (it != players.end())
	{
		PlayerInstance& pi = it->second;
		//sf::Socket::Status status = socket.send(packet, pi.getIpAddress(), pi.getPort());
		sendTo(packet, pi);
	}
}

void Server::sendTo(sf::Packet& packet, PlayerInstance& playerRef)
{
	// wrap the packet in another packet with unreliable header
	// send out that packet instead of the input packet
	sf::Packet wrappedPacket;
	wrappedPacket << HeaderType::Unreliable;
	wrappedPacket.append(packet.getData(), packet.getDataSize());
	auto status = socket.send(wrappedPacket, playerRef.getIpAddress(), playerRef.getPort());
}

void Server::sendReliable(sf::Packet& packet)
{
	for (auto& p : players)
	{
		PlayerInstance& pi = p.second;
		//sf::Socket::Status status = socket.send(packet, pi.getIpAddress(), pi.getPort());
		sendToReliable(packet, pi);
	}
}

void Server::sendToReliable(sf::Packet& packet, int id)
{
	auto it = players.find(id);

	if (it != players.end())
	{
		PlayerInstance& pi = it->second;
		//sf::Socket::Status status = socket.send(packet, pi.getIpAddress(), pi.getPort());
		sendTo(packet, pi);
	}
}

void Server::sendToReliable(sf::Packet& packet, PlayerInstance& playerRef)
{
	std::uint16_t sequenceId = playerRef.getSequenceIdThenIncrement();
	sf::Packet wrappedPacket;
	wrappedPacket << HeaderType::Reliable << sequenceId;

	wrappedPacket.append(packet.getData(), packet.getDataSize());


	playerRef.addPendingPacket(wrappedPacket, sequenceId);
	auto s = socket.send(wrappedPacket, playerRef.getIpAddress(), playerRef.getPort());
}

void Server::update(float deltaTime)
{
	time += deltaTime;

	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Packet packet;
	if (socket.receive(packet, sender, senderPort) == sf::Socket::Status::Done)
	{
		std::cout << "Message received from client " << sender.value() << std::endl;

		HeaderType headerType;
		//PacketType type;
		packet >> headerType;

		//bool isReliable = reliable  == HeaderType::Reliable; // if reliable is more than 0, the value is the sequence id.
		
		if (headerType == HeaderType::Ack) {
			// find out who sender is as playerinstance
			auto it = std::find_if(players.begin(), players.end(), [&sender, senderPort](const std::pair<size_t, PlayerInstance>& pair)
				{
					return pair.second.getIpAddress() == sender.value() && pair.second.getPort() == senderPort;
				});
			// if find or find if is successful, 'it' will not point to the end of the collection
			if (it != players.end())
			{
				// player already connected, what to do?
				PlayerInstance& pi = it->second;
				std::uint16_t seqId;
				packet >> seqId;
				pi.receiveAckFor(seqId);

				std::cout << " Received ack from " << pi.getIpAddress() << " : " << pi.getPort() << " | for seqID: " << seqId << std::endl;
			}
		}
		else {
			PacketType type;
			packet >> type;
			// TODO: if reliable, send ack to sender
			if (type == PacketType::ClientJoinRequest)
			{
				// Check if requester is already connected
				auto it = std::find_if(players.begin(), players.end(), [&sender, senderPort](const std::pair<size_t, PlayerInstance>& pair)
					{
						return pair.second.getIpAddress() == sender.value() && pair.second.getPort() == senderPort;
					});

				if (it != players.end())
				{
					// player already connected, what to do?
				}
				else
				{
					std::string name;
					packet >> name;
					std::cout << "Welcoming " << name << ::std::endl;

					players.emplace(std::piecewise_construct,
						std::forward_as_tuple(idIncrementor),
						std::forward_as_tuple(sender.value(), senderPort, idIncrementor, name, time));

					sf::Packet outgoingPacket;
					outgoingPacket << HeaderType::Unreliable; // TODO: needs to be reliable i.e. send again if packet is lost
					outgoingPacket << PacketType::ServerJoinResponse;
					outgoingPacket << idIncrementor;

					// this response packet could be lost in transit.
					// which means the newly created PlayerInstance (above) will be a ghost (oooOOoooOooo)
					// heartbeat check will drop this ghost player eventually
					if (socket.send(outgoingPacket, sender.value(), senderPort) == sf::Socket::Status::Done)
					{
						std::cout << "Sent response" << ::std::endl;
					}

					idIncrementor++;
					playerCount++;
				}
			}
			//else if (type == PacketType::UserDisconnected)
			//{
			//	// if player explicitly quit, tell everyone
			//	// though this isn't needed if heartbeat check is done.
			//}
			else if (type == PacketType::TimeSyncRequest)
			{
				// if time sync, pong back
				// for this, do not update lastPacketTime

				float senderTime;
				packet >> senderTime;

				sf::Packet outgoingPacket;
				outgoingPacket << HeaderType::Unreliable; // no need reliable here
				outgoingPacket << PacketType::TimeSyncResponse;
				outgoingPacket << time << senderTime;
				if (socket.send(outgoingPacket, sender.value(), senderPort) == sf::Socket::Status::Done)
				{
					std::cout << "Sent server time" << ::std::endl;
				}
			}

		}

		
	}

	// handle RUDP pending packets for every clients
	// 
	// iterate through all player instance, for each pi
	// Iterate though all pending packets, for each pending
	// find out how much time has passed since last attempt
	// if time > waitThreshold
	// if pending attempts < maxattempt
	// incremeent pending ttempt
	// restart pending timer
	// send pending packet again
	// otherwise
	// should drop the player because failed several times
	
	static float WaitThreshold = 0.5f;
	static int MaxAttempts = 3;

	for (auto& [id, pi] : players) {
		for (auto& [seqId, pPacket] : pi.getPendingPacketMap()) {
			float t = pPacket.timer.getElapsedTime().asSeconds();
			if (t > WaitThreshold) {
				if (pPacket.attempts < MaxAttempts) {
					pPacket.attempts++;
					pPacket.timer.restart();
					// resend directly instead of using sendTOReliable
					// if we use the latter, it will wrap our already wrapped packet
					// which is wrong
					auto s = socket.send(pPacket.packet, pi.getIpAddress(), pi.getPort());
				}
				else {
					// should drop player

				}
			}
		}
	}


	// note:
	// technically wait threshhold should be longer per failed attempt
	// to give more time for ack to arrive
	// 
	// check heartbeat to drop player if they didn't send data for some time.
}