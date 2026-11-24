#include "GameRoom.h"

#include <iostream>

GameRoom::GameRoom(unsigned int id, std::string roomName)
{
    this->roomName = roomName + "'s room";
    this->roomId = id;
}

void GameRoom::AddPlayer(PlayerConnection* player)
{
    playerList.push_back(player);
}

void GameRoom::RemovePlayer(int playerId)
{
    auto it = playerList.begin();
    
    for (it; it != playerList.end(); it++) {
        PlayerConnection* player = *it;

        if (player->getId() == playerId) {
            playerList.erase(it);
            std::cout << "PLAYER LEFT: " << player->getName() << " ID: " << player->getId() << std::endl;
            return;
        }
    }
}

bool GameRoom::StartGame()
{
    // something here
    bool canStart = true;

    auto it = playerList.begin();

    for (it; it != playerList.end(); it++) {
        PlayerConnection* player = *it;

        if (player->GetReadyState() == false) {
            return false;
        }
    }

    return canStart;
}

void GameRoom::UpdateTime()
{
    if (startTimer) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        auto diff = (now - timer);
        if (diff >= std::chrono::seconds(3)) {
            StartBattle();
        }
    }
}

void GameRoom::StartBattle()
{
    inBattle = true;
    startTimer = false;
    std::cout << "STARTING BATTLE NOW" << std::endl;
}

void GameRoom::StartTimer()
{
    timer = std::chrono::steady_clock::now();
    startTimer = true;
    std::cout << "TIMER STARTED" << std::endl;
    
}

void GameRoom::StopTimer()
{
    timer = std::chrono::steady_clock::now();
    startTimer = false;
    std::cout << "TIMER STOPPED" << std::endl;
}

bool GameRoom::BothPlayersLocked()
{
    bool canStart = true;

    auto it = playerList.begin();

    for (it; it != playerList.end(); it++) {
        PlayerConnection* player = *it;

        if (player->GetLocked() == false) {
            return false;
        }
    }

    return canStart;
}
