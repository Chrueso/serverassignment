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

sf::Packet makeRoomListPacket()
{
    const auto& rooms = GameRoomHandler::GetRooms();
    sf::Packet packet;
    packet << PacketType::RoomList << (int)rooms.size();
    for (GameRoom* room : rooms)
    {
        packet << (int)room->GetId() << room->GetName() << room->GetPlayerCount()
            << room->GetMaxPlayers() << room->inBattle;
    }
    return packet;
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
                    std::cout << incomingSocket->getRemoteAddress().value() << " connected. Assigning id: " <<
                        idIncrementor << std::endl;
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
                                std::cout << sock->getRemoteAddress().value() << " rejected and disconnected" <<
                                    std::endl;

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
                        case PacketType::ChatMessage:
                        {
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
                        case PacketType::ClientCreateRoom:
                        {
                            // a player can only be in one room at a time
                            if (player.getCurrentRoomId() != -1)
                            {
                                sf::Packet fail;
                                fail << PacketType::RoomJoinFailed << std::string("You are already in a room");
                                sendTo(player, fail);
                                break;
                            }

                            GameRoom* room = GameRoomHandler::CreateGameRoom(player.getName());
                            room->AddPlayer(&player);
                            player.setCurrentRoomId((int)room->GetId());
                            std::cout << "Room created with name: " << room->GetName() << " Id: " << room->GetId()
                                << " Players Count: " << room->GetPlayerCount() << std::endl;

                            sf::Packet outPacket;
                            outPacket << PacketType::RoomCreated;
                            outPacket << room->GetId();
                            outPacket << room->GetName();
                            outPacket << player.getId();

                            broadcast(outPacket);

                            sf::Packet joined;
                            joined << PacketType::RoomJoined << (int)room->GetId();
                            sendTo(player, joined); // the creator's client is waiting for this

                            sf::Packet list = makeRoomListPacket();
                            broadcast(list); // everyone's list updates
                            // might need a status check to see if this succeded but later ig
                            break;
                        }
                        break;
                        // case PacketType::ClientJoinRoom: 
                        // 	{
                        // 	int roomId;
                        // 	inPacket >> roomId;
                        //
                        // 	GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                        // 	room->AddPlayer(&player);
                        //
                        // 	sf::Packet outPacket;
                        // 	outPacket << PacketType::RoomJoined;
                        // 	outPacket << roomId;
                        // 	break;
                        // }
                        case PacketType::ClientRequestRoomList:
                        {
                            sf::Packet p = makeRoomListPacket();
                            sendTo(player, p);
                            break;
                        }

                        case PacketType::ClientJoinRoom:
                        {
                            int roomId;
                            inPacket >> roomId;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);

                            sf::Packet reply;
                            if (player.getCurrentRoomId() != -1) reply << PacketType::RoomJoinFailed << std::string(
                                "You are already in a room");
                            else if (room == nullptr) reply << PacketType::RoomJoinFailed << std::string(
                                "Room not found");
                            else if (room->IsFull()) reply << PacketType::RoomJoinFailed << std::string(
                                "Room is full");
                            else if (room->inBattle) reply << PacketType::RoomJoinFailed << std::string(
                                "Battle in progress");
                            else
                            {
                                room->AddPlayer(&player);
                                player.setCurrentRoomId(roomId);
                                std::cout << "PLAYER JOINED: " << player.getName() << " ID: " << player.getId() << std::endl;
                                reply << PacketType::RoomJoined << roomId;
                            }
                            sendTo(player, reply);

                            sf::Packet list = makeRoomListPacket();
                            broadcast(list);
                            break;
                        }

                        case PacketType::ClientLeaveRoom:
                        {
                            int roomId;
                            inPacket >> roomId;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;

                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());

                            sf::Packet leftPacket;
                            leftPacket << PacketType::RoomLeft << player.getId();   // "this player left"
                            sendTo(player, leftPacket);                             // the leaver goes back to the lobby
                            if (otherPlr) sendTo(*otherPlr, leftPacket);            // the other player's card empties

                            room->RemovePlayer(player.getId());
                            player.setCurrentRoomId(-1);
                            if (player.GetReadyState()) player.ToggleReadyState();  // not ready next time
                            player.SetCreature(-1);
                            if (otherPlr && otherPlr->GetReadyState()) otherPlr->ToggleReadyState();  // unready

                            GameRoomHandler::CleanupGameRoom();                     // deletes the room if its empty
                            sf::Packet list = makeRoomListPacket();
                            broadcast(list);                                        // lobby lists show 1/2 or remove the room
                            break;
                        }
                        case PacketType::ClientRequestRoomUpdate:
                        {
                            int roomId;
                            inPacket >> roomId;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;

                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) break;

                            sf::Packet updatePacket;
                            updatePacket << PacketType::RoomUpdate;
                            updatePacket << otherPlr->getId();
                            updatePacket << otherPlr->getName();
                            updatePacket << otherPlr->GetReadyState();
                            updatePacket << otherPlr->GetCreature();
                            // NEW: an int (0-2, -1 = none) instead of the pointer
                            updatePacket << otherPlr->GetWins();
                            updatePacket << otherPlr->GetLosses();
                            sendTo(player, updatePacket);
                            break;
                        }
                        case PacketType::ClientToggleReady:
                        {
                            int roomId;
                            inPacket >> roomId;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;
                            if (player.GetCreature() == -1) break;
                            player.ToggleReadyState();

                            sf::Packet updatePacket;
                            updatePacket << PacketType::RoomUpdate;
                            updatePacket << player.getId();
                            updatePacket << player.getName();
                            updatePacket << player.GetReadyState();
                            updatePacket << player.GetCreature();
                            updatePacket << player.GetWins();
                            updatePacket << player.GetLosses();
                            sendTo(player, updatePacket);

                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (otherPlr) sendTo(*otherPlr, updatePacket);

                            // GameStart later when both ready
                            if (room->StartGame()) {
                                sf::Packet startPacket;
                                startPacket << PacketType::GameStart;
                                sendTo(player, startPacket);
                                sendTo(*otherPlr, startPacket);
                            }
                            break;
                        }
                        case PacketType::ClientSetCreature:
                        {
                            int creatureType;
                            int roomId;
                            inPacket >> roomId;
                            inPacket >> creatureType;

                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;
                            if (creatureType < 0 || creatureType > 2) break;
                            if (player.GetReadyState()) break;

                            player.SetCreature(creatureType);

                            sf::Packet updatePacket;
                            updatePacket << PacketType::RoomUpdate;
                            updatePacket << player.getId();
                            updatePacket << player.getName();
                            updatePacket << player.GetReadyState();
                            updatePacket << player.GetCreature();
                            updatePacket << player.GetWins();
                            updatePacket << player.GetLosses();
                            sendTo(player, updatePacket);

                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (otherPlr) sendTo(*otherPlr, updatePacket);
                            break;

                            break;
                        }
                        case PacketType::RoomChat:
                        {
                            int roomId;
                            std::string msg;
                            inPacket >> roomId;
                            inPacket >> msg;
                            std::cout << msg;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (player.getCurrentRoomId() != room->GetId()) continue;
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) continue;
                            sf::Packet outPacket;
                            outPacket << PacketType::RoomChat;
                            outPacket << player.getName();
                            outPacket << player.getId();
                            outPacket << msg;

                            sendTo(*otherPlr, outPacket);
                            sendTo(player, outPacket);
                            break;
                        }
                        case PacketType::RoomEmote:
                        {
                            int roomId;
                            int emote;
                            inPacket >> roomId;
                            inPacket >> emote;
                            std::cout << emote << std::endl;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (player.getCurrentRoomId() != room->GetId()) continue;
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) continue;
                            sf::Packet outPacket;
                            outPacket << PacketType::RoomEmote;
                            outPacket << player.getName();
                            outPacket << player.getId();
                            outPacket << emote;

                            sendTo(*otherPlr, outPacket);
                            sendTo(player, outPacket);

                            break;
                        }
                        case PacketType::ClientRequestBattleUpdate: {
                            int roomId;
                            inPacket >> roomId;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) continue;
                            sf::Packet outPacket1;
                            outPacket1 << PacketType::BattleUpdate;
                            outPacket1 << room->GetRound();
                            outPacket1 << room->GetPlayerCount();
                            outPacket1 << player.getId();
                            outPacket1 << player.getName();
                            outPacket1 << player.GetCreature();
                            outPacket1 << player.GetCreatureData().health;
                            outPacket1 << player.GetCreatureData().maxHealth;
                            
                            
                            outPacket1 << room->GetRound();
                            outPacket1 << room->GetPlayerCount();
                            outPacket1 << otherPlr->getId();
                            outPacket1 << otherPlr->getName();
                            outPacket1 << otherPlr->GetCreature();
                            outPacket1 << otherPlr->GetCreatureData().health;
                            outPacket1 << otherPlr->GetCreatureData().maxHealth;
                            //sendTo(*otherPlr, outPacket1);
                            sendTo(player, outPacket1);
                            break;
                        }
                        case PacketType::ClientChooseMove: {
                            int roomId;
                            int moveType;
                            inPacket >> roomId;
                            inPacket >> moveType;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (!room || player.getCurrentRoomId() != room->GetId()) break;
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) continue;
                            sf::Packet moveLockPacket;
                            moveLockPacket << PacketType::MoveLocked;
                            moveLockPacket << player.getId();
                            //sendTo(*otherPlr, moveLockPacket);
                            sendTo(player, moveLockPacket);
                            int resultingDmg = 0;
                            switch (moveType) {
                            case 0: {
                                // Attack
                                resultingDmg = otherPlr->GetCreatureData().defend ? 0 : otherPlr->GetCreatureData().dmg;
                                player.GetCreatureData().TakeDmg(resultingDmg);
                                if (otherPlr->GetCreatureData().defend) {
                                    otherPlr->GetCreatureData().ToggleDefend();
                                }
                                break;
                            }
                            case 1: {
                                // Defend
                                player.GetCreatureData().ToggleDefend();
                                break;
                            }
                            case 2: {
                                // break
                                otherPlr->GetCreatureData().ToggleBreak();
                                break;
                            }
                            default: {
                                continue;
                            }
                            }

                            sf::Packet outPacket1;
                            outPacket1 << PacketType::RoundResult;
                            outPacket1 << room->GetRound();
                            outPacket1 << 2;
                            outPacket1 << player.getId();
                            outPacket1 << moveType;
                            outPacket1 << resultingDmg;
                            outPacket1 << player.GetCreatureData().health;
                            
                            
                            outPacket1 << room->GetRound();
                            outPacket1 << 2;
                            outPacket1 << otherPlr->getId();
                            outPacket1 << moveType;
                            outPacket1 << resultingDmg;
                            outPacket1 << otherPlr->GetCreatureData().health;
                            //sendTo(*otherPlr, outPacket1);
                            sendTo(player, outPacket1);
                            room->IncrementRound();
                            if (otherPlr->GetCreatureData().health == 0 && player.GetCreatureData().health == 0) {
                                sf::Packet gameEndPacket;
                                gameEndPacket << PacketType::BattleEnd;
                                gameEndPacket << -1;
                                sendTo(*otherPlr, gameEndPacket);
                                sendTo(player, gameEndPacket);
                            } else if (player.GetCreatureData().health == 0) {
                                sf::Packet gameEndPacket;
                                gameEndPacket << PacketType::BattleEnd;
                                gameEndPacket << otherPlr->getId();
                                sendTo(*otherPlr, gameEndPacket);
                                sendTo(player, gameEndPacket);
                            }
                            else if (otherPlr->GetCreatureData().health == 0) {
                                sf::Packet gameEndPacket;
                                gameEndPacket << PacketType::BattleEnd;
                                gameEndPacket << player.getId();
                                sendTo(*otherPlr, gameEndPacket);
                                sendTo(player, gameEndPacket);
                            }
                            break;
                        }
                        case PacketType::BattleEmote: {
                            int roomId;
                            int emote;
                            inPacket >> roomId;
                            inPacket >> emote;
                            std::cout << emote << std::endl;
                            GameRoom* room = GameRoomHandler::GetGameRoom(roomId);
                            if (player.getCurrentRoomId() != room->GetId()) continue;
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (!otherPlr) continue;
                            sf::Packet outPacket;
                            outPacket << PacketType::BattleEmote;
                            outPacket << player.getName();
                            outPacket << player.getId();
                            outPacket << emote;

                            sendTo(*otherPlr, outPacket);
                            sendTo(player, outPacket);
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

                        if (player.getCurrentRoomId() != -1)
                        {
                            GameRoom* room = GameRoomHandler::GetGameRoom(player.getCurrentRoomId());
                            PlayerConnection* otherPlr = GameRoomHandler::GetOtherPlayer(room->GetId(), player.getId());
                            if (otherPlr)
                            {
                                sf::Packet outPacket;
                                outPacket << PacketType::RoomLeft;
                                outPacket << player.getId();
                                sendTo(*otherPlr, outPacket);
                            }
                            room->RemovePlayer(player.getId());

                            GameRoomHandler::CleanupGameRoom();
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
