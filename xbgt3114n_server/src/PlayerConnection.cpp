#include "PlayerConnection.h"

PlayerConnection::PlayerConnection(sf::TcpSocket* socket, int id)
{
	this->socket = std::move(socket);
	this->id = id;
	authenticated = false;
}

int PlayerConnection::getId()
{
	return id;
}

void PlayerConnection::setName(const std::string name)
{
	this->name = name;
	authenticated = true;
}

std::string PlayerConnection::getName()
{
	return name;
}

sf::TcpSocket* PlayerConnection::getSocket()
{
	return socket;
}

bool PlayerConnection::isAuthenticated()
{
	return authenticated;
}

int PlayerConnection::getCurrentRoomId()
{
	return currentRoomId;
}

void PlayerConnection::setCurrentRoomId(int roomId)
{
	currentRoomId = roomId;
}