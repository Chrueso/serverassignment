#include "GameRoom.h"



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
            return;
        }
    }
}

bool GameRoom::StartGame()
{
    // something here
}
