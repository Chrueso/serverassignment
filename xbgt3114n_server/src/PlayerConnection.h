#pragma once
#include <SFML/Network.hpp>

struct Pokemon {
	int health;
	int dmg;
	int type;

	Pokemon(int hp, int d, int t) : health(hp), dmg(d), type(t) {}
	Pokemon() : health(100), dmg(1), type(-1) {}
};

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
	Pokemon* GetPokemon() { return chosenPokemon; }
	void SetPokemon(Pokemon* choicePokemon) { chosenPokemon = choicePokemon; }
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
	Pokemon* chosenPokemon = {};
	int wins = 0;
	int losses = 0;
};
