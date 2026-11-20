#pragma once
#include <SFML/Network.hpp>

struct Pokemon {
	int health;
	int dmg;
	int type;

	Pokemon(int hp, int d, int t) : health(hp), dmg(d), type(t) {}
	Pokemon() : health(100), dmg(1), type(-1) {}
};

inline Pokemon GetCreatureStats(int type)
{
	switch (type)
	{
	case 0:  return Pokemon(100, 10, 0);
	case 1:  return Pokemon(90, 20, 1);
	case 2:  return Pokemon(110, 5, 2);
	default: return Pokemon(); 
	}
}

class PlayerConnection
{
public:
	PlayerConnection(sf::TcpSocket* socket, int id);

	void setName(const std::string name);

	int getId();
	std::string getName();
	sf::TcpSocket* getSocket();
	bool isAuthenticated();

	// -1 = in the lobby		
	int getCurrentRoomId();
	void setCurrentRoomId(int roomId);
	bool GetReadyState() { return ready; }
	void ToggleReadyState() { ready = !ready; }
	int GetCreature() { return creatureId; }     
	void SetCreature(int c) { creatureId = c; }
	int GetWins() { return wins; }
	void IncreaseWin() { wins++; }
	int GetLosses() { return losses; }
	void IncreaseLoss() { losses++; }
private:
	int id;
	std::string name;
	bool authenticated;
	sf::TcpSocket* socket;
	
	int currentRoomId = -1;
	bool ready = false;
	int creatureId = -1;
	int wins = 0;
	int losses = 0;
};
