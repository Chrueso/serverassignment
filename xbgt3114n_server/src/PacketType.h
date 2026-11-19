#pragma once
#include <SFML/Network.hpp>

enum class PacketType : std::uint8_t
{
    ClientJoinRequest,
    ServerJoinResponse,
    ServerJoinRejected,
    UserConnected,
    UserDisconnected,
    ChatMessage,

    // Room
    ClientCreateRoom,
    RoomCreated,
    RoomJoined,
    ClientJoinRoom,
    ClientLeaveRoom,
    RoomEnded,
    RoomJoinFailed,

    // Room display
    ClientRequestRoomList,
    RoomList                //count, Per room id: id, name, players, maxPlayers, inBattle
};

struct PacketHeader
{
	PacketType type;
	std::uint16_t sequenceId;
};

// Overload sf::Packet's << and >> so we can push or pull PacketType
sf::Packet& operator <<
(sf::Packet& packet, const PacketType& type);

sf::Packet& operator >>
(sf::Packet& packet, PacketType& outType);