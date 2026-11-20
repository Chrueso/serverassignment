#pragma once
#include "PlayerConnection.h"

class GameRoom
{
private:
    int maxPlayers = 2;

protected:
    std::vector<PlayerConnection*> playerList;
    unsigned int roomId;
    std::string roomName;
public:
    GameRoom(unsigned int id, std::string roomName);
    void AddPlayer(PlayerConnection* player);
    void RemovePlayer(int playerId);
    bool StartGame();
    unsigned int GetId() { return roomId; }
    std::string GetName() { return roomName; }
    int GetPlayerCount() { return playerList.size(); }
    
    // add
    int GetMaxPlayers() { return maxPlayers; }
    bool IsFull() { return (int)playerList.size() >= maxPlayers; }
    bool inBattle = false; 
    
    std::vector<PlayerConnection*> GetPlayerList() { return playerList; }
};

