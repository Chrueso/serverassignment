#include "PacketType.h"

sf::Packet& operator <<(sf::Packet& packet,
	const PacketType& type)
{
	return packet << (std::uint8_t)type;
}

sf::Packet& operator>>(sf::Packet& packet,
	PacketType& outType)
{
	std::uint8_t type;
	packet >> type;
	outType = (PacketType)type;
	return packet;
}

sf::Packet& operator <<(sf::Packet& packet,
	const HeaderType& type)
{
	return packet << (std::uint8_t)type;
}

sf::Packet& operator>>(sf::Packet& packet,
	HeaderType& outType)
{
	std::uint8_t type;
	packet >> type;
	outType = (HeaderType)type;
	return packet;
}