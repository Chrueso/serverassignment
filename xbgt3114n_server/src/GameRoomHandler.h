#pragma once
#include "GameRoom.h"

class GameRoomHandler
{
private:
    static std::vector<GameRoom*> roomList;
protected:
    static unsigned int incrementalId;
public:
    static GameRoom* CreateGameRoom(std::string roomName);
    static GameRoom* GetGameRoom(int roomId);
    static void CleanupGameRoom();

    static const std::vector<GameRoom*>& GetRooms() { return roomList; }
};

