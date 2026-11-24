#pragma once
#include "PlayerConnection.h"
#include <chrono>
class GameRoom
{
private:
    int maxPlayers = 2;

protected:
    std::vector<PlayerConnection*> playerList;
    unsigned int roomId;
    std::string roomName;
    int round = 0;
    bool inBattle = false;
    std::chrono::steady_clock::time_point timer;
    bool startTimer = false;
    
public:
    GameRoom(unsigned int id, std::string roomName);
    void AddPlayer(PlayerConnection* player);
    void RemovePlayer(int playerId);
    bool StartGame();

    void UpdateTime();
    void StartBattle();
    void StartTimer();
    void StopTimer();
    bool BothPlayersLocked();
    unsigned int GetId() { return roomId; }
    std::string GetName() { return roomName; }
    int GetPlayerCount() { return playerList.size(); }
    
    
    int GetMaxPlayers() { return maxPlayers; }
    bool IsFull() { return (int)playerList.size() >= maxPlayers; }
    
    
    std::vector<PlayerConnection*> GetPlayerList() { return playerList; }

    int GetRound() { return round; }
    void IncrementRound() { round++; }

    void SetInBattle(bool inbattle) { inBattle = inbattle; }
    bool GetInBattle() { return inBattle; }
    bool GetTimerStarted() { return startTimer; }
    
};

