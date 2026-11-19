#pragma once
#include <SFML/Network.hpp>

class PlayerConnection
{
public:
	PlayerConnection(sf::TcpSocket* socket, int id);

	void setName(const std::string name);

	int getId();
	std::string getName();
	sf::TcpSocket* getSocket();
	bool isAuthenticated();

private:
	int id;
	std::string name;
	bool authenticated;
	sf::TcpSocket* socket;
};
