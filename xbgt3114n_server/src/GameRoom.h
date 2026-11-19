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
    void StartGame();
    unsigned int GetId() { return roomId; }
    std::string GetName() { return roomName; }
    int GetPlayerCount() { return playerList.size(); }
    // maybe a get player list?
};

