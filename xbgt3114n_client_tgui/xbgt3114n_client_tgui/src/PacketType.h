#pragma once
#include <SFML/Network.hpp>

enum class PacketType : std::uint8_t
{
	ClientJoinRequest,
	ServerJoinResponse,
	ServerJoinRejected,
	UserConnected,
	UserDisconnected,
	ChatMessage
};

// Overload sf::Packet's << and >> so we can push or pull PacketType
sf::Packet& operator <<
(sf::Packet& packet, const PacketType& type);

sf::Packet& operator >>
(sf::Packet& packet, PacketType& outType);