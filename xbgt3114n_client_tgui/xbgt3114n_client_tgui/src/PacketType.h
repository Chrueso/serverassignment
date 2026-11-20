#pragma once
#include <SFML/Network.hpp>

enum class PacketType : std::uint8_t
{
    ClientJoinRequest,
    ServerJoinResponse,
    ServerJoinRejected,
    UserConnected,
    UserDisconnected,
    ChatMessage,

    // Room
    ClientCreateRoom,
    RoomCreated,
    RoomJoined,
    ClientJoinRoom,
    ClientLeaveRoom,
    RoomEnded,
    RoomJoinFailed,         

    // Room display
    ClientRequestRoomList,  
    RoomList,                //count, Per room id: id, name, players, maxPlayers, inBattle
    
    // 
    ClientRequestRoomUpdate, // Client > Server : when joined
    RoomUpdate,              // Server > Room: when theres snapshot updated
    ClientToggleReady,       // Cleint > Server
    ClientSetCreature,       // Client > Server : creatureID
    RoomChat,                // Client > Server : msg | Server > room: name, id, msg
    RoomEmote,               // Client > Server emoteId | Server> (room? client?): name, id, emote
    RoomLeft,                // Server > CLient Maybe in chat? :"You have left room "name"" 
    GameStart                // Server>Room    "countdown finished, go to battle"
};

// Overload sf::Packet's << and >> so we can push or pull PacketType
sf::Packet& operator <<
(sf::Packet& packet, const PacketType& type);

sf::Packet& operator >>
(sf::Packet& packet, PacketType& outType);
