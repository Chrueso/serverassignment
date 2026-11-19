#include "GameRoomHandler.h"
#include <iostream>
std::vector<GameRoom*> GameRoomHandler::roomList = {};
unsigned int GameRoomHandler::incrementalId = 0;

GameRoom* GameRoomHandler::CreateGameRoom(std::string roomName)
{
    GameRoom* room = new GameRoom(incrementalId++, roomName);
    roomList.push_back(room);
    return room;
}

GameRoom* GameRoomHandler::GetGameRoom(int roomId)
{
    auto it = roomList.begin();

    for (it; it != roomList.end(); it++) {
        GameRoom* room = *it;

        if (room->GetId() == roomId) {
            return room;
        }
    }
    return nullptr; 
}

void GameRoomHandler::CleanupGameRoom()
{
    auto it = roomList.begin();

    for (it; it != roomList.end();) {
        GameRoom* room = *it;

        if (room->GetPlayerCount() == 0) {
            it = roomList.erase(it);
            std::cout << "ROOM DELETED" << std::endl;
        }
        else {
            it++;
        }
    }
}
