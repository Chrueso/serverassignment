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
