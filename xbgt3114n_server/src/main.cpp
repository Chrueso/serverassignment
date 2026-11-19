#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include "PacketType.h"
#include "GameRoomHandler.h"

static std::vector<PlayerConnection> players;

static int idIncrementor = 0;
static int playerCount = 0;

static const int MAX_PLAYER_COUNT = 4;

bool sendTo(PlayerConnection& player, sf::Packet& packet)
{
	// get player socket
	sf::TcpSocket* pSocket = player.getSocket();
	// if null, return false;
	if (pSocket == NULL) return false;

	// send packet to socket
	sf::Socket::Status sendStatus = pSocket->send(packet);
	// if not done, log error and return false
	
	if (sendStatus != sf::Socket::Status::Done) return false;
	
	// otherwise return true
	return true;
}

void broadcast(sf::Packet& packet, int excludeId = -1)
{
	// loop all players
	// if not authenticated, skip
	// if id matches exclude id, skip
	// otherwise, sendTo that player

	for (PlayerConnection player : players)
	{
		if (!player.isAuthenticated()) continue;

		if (player.getId() == excludeId) continue;

		sendTo(player, packet);
	}
}

int main()
{
	static sf::TcpListener listener;
	static sf::SocketSelector selector;

	unsigned short port = 53000;

	if (listener.listen(port) != sf::Socket::Status::Done)
	{
		return -1; // Error
	}

	std::cout << "Server started using port:" << port << std::endl;

	selector.add(listener);

	bool isRunning = true;
	while (isRunning)
	{
		if (selector.wait())
		{
			// Handle users connecting to server (us).
			if (selector.isReady(listener))
			{
				// Just accept any users trying to connect.
				// We process actual accept or reject when
				// user send their id.
				sf::TcpSocket* incomingSocket = new sf::TcpSocket();
				if (listener.accept(*incomingSocket) == sf::Socket::Status::Done)
				{
					std::cout << incomingSocket->getRemoteAddress().value() << " connected. Assigning id: " << idIncrementor << std::endl;
					// - create a PlayerConnection, passing the socket
					//   current value of idIncrementor
					// - add to player list

					// using emplace_back means we create PlayerConnection directly on the `players` address
					players.emplace_back(PlayerConnection(incomingSocket, idIncrementor));
					// create the entity like PlayerConnection conn and then players.push(conn) is copying the object.

					// - add the socket to selector so we start accepting packets from that user.
					selector.add(*(players.back()).getSocket());

					// - increment idIncrementor for next usage.
					++idIncrementor;
				}
				else
				{
					delete incomingSocket;
				}
			}

			//std::vector<PlayerConnection>::iterator it;
			//for(it = players.begin(); it != players.end();)
			for (int i = 0; i < players.size();)
			{
				PlayerConnection& player = players[i];
				sf::TcpSocket* sock = player.getSocket();
				if (selector.isReady(*sock))
				{
					sf::Packet inPacket;
					sf::Socket::Status status = sock->receive(inPacket);

					if (status == sf::Socket::Status::Done)
					{
						// User sent packet to us -> process and handle it
						PacketType type;
						inPacket >> type;

						switch (type)
						{
						case PacketType::ClientJoinRequest:
						{
							// full format for ClientJoinRequest:
							// PacketType::ClientJoinRequest
							// std::string
							// bool

							// after user join, they need to send their id
							// process to either accept or reject
							std::string name;
							bool isAdmin;

							inPacket >> name >> isAdmin;
							bool allowed = isAdmin || playerCount < MAX_PLAYER_COUNT;
							if (allowed)
							{
								// technically admin shouldn't take a slot.
								// but don't have method to check player is admin
								// if (isAdmin)
								playerCount++;

								std::cout << sock->getRemoteAddress().value() << " sets name as "
									<< name << std::endl;

								player.setName(name); // Sideffect: also set authenticated to true

								// send acception to the user
								sf::Packet acceptPacket;
								acceptPacket << PacketType::ServerJoinResponse << player.getId();
								sendTo(player, acceptPacket);

								// broadcast connection to all other users
								sf::Packet broadcastPacket;
								broadcastPacket << PacketType::UserConnected << name;
								broadcast(broadcastPacket, player.getId());
							}
							else
							{
								std::cout << sock->getRemoteAddress().value() << " rejected and disconnected" << std::endl;

								// send rejection to the user.
								sf::Packet outPacket;
								outPacket << PacketType::ServerJoinRejected;
								sendTo(player, outPacket);

								// - Delete the player from players vector
								// - remove socket from selector
								// - delete socket
								players.erase(players.begin() + i);
								selector.remove(*sock);
								delete sock;

								// erasing the plyaer will shift the next player to this index
								// so skip increment.
								continue;
							}
						}
						break;
						case PacketType::ChatMessage: {
							// Minimally, packet needs to have
							// std::string which is formatted <NAME>: <MSG>
							// optionally, the plaer id as well
							std::string msg;
							inPacket >> msg;
							std::cout << msg;

							sf::Packet outPacket;
							outPacket << PacketType::ChatMessage;
							outPacket << player.getName();
							outPacket << player.getId();
							outPacket << msg;

							broadcast(outPacket);
							break;
						}
							//case PacketType::Emoting:
							//	minimally, packet needs to have emoteId and player id
							//	break;
							// Lobby created
						break;
						case PacketType::ClientCreateRoom: {

							
							GameRoom* room = GameRoomHandler::CreateGameRoom(player.getName());
							room->AddPlayer(&player);
							std::cout << "Room created with name: " << room->GetName() << " Id: " << room->GetId() << " Players Count: " << room->GetPlayerCount() << std::endl;
							
							sf::Packet outPacket;
							outPacket << PacketType::RoomCreated;
							outPacket << room->GetId();
							outPacket << room->GetName();
							outPacket << player.getId();

							broadcast(outPacket);
							// might need a status check to see if this succeded but later ig
							break;
						}
						break;
						case PacketType::ClientJoinRoom: {
							int roomId;
							inPacket >> roomId;

							GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
							room->AddPlayer(&player);

							sf::Packet outPacket;
							outPacket << PacketType::RoomJoined;
							outPacket << roomId;
							break;
						}
						}

						
					}

					else if (status == sf::Socket::Status::Disconnected)
					{
						// if connection is terminated
						// FIN handshake will occur behind the scenes
						// so we will get this from the client when
						// the client disconnects from the server.

						std::cout << player.getName() << " disconnected" << std::endl;
						// tell everyone except the disconnected user that someone disconnected
						sf::Packet outPacket;
						outPacket << PacketType::UserDisconnected << player.getName();
						broadcast(outPacket, player.getId());

						// player count increases when user successfully authenticate
						// so we need to properly bookkeep our head count.
						// TODO: admin check, currently don't have method to check player is admin
						if (player.isAuthenticated())
						{
							playerCount--;
						}
						// - Delete the player from players vector
						players.erase(players.begin() + i);
						// - remove socket from selector
						selector.remove(*sock);
						// - delete socket
						delete sock;

						// erasing the plyaer will shift the next player to this index
						// so skip increment.
						continue;
					}
				}
				// Only increment when we are not removing any player.
				i++;
			}
		}
	}
}