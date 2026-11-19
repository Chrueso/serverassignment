#pragma once
#include <SFML/Network.hpp>

enum class HeaderType : std::uint8_t {

	Unreliable = 0,
	Reliable = 1,
	Ack = 2
};

sf::Packet& operator <<
(sf::Packet& packet, const HeaderType& type);

sf::Packet& operator >>
(sf::Packet& packet, HeaderType& outType);

enum class PacketType : std::uint8_t
{
	ClientJoinRequest,
	ServerJoinResponse,
	ServerJoinRejected,
	TimeSyncRequest,
	TimeSyncResponse,
	GameStateUpdate,
	ServerDisconnect,
	DummyPacket,

};

// Overload sf::Packet's << and >> so we can push or pull PacketType
sf::Packet& operator <<
(sf::Packet& packet, const PacketType& type);

sf::Packet& operator >>
(sf::Packet& packet, PacketType& outType);

