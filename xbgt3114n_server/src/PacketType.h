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

    // Lobby room display
    ClientRequestRoomList,
    RoomList,                //count, Per room id: id, name, players, maxPlayers, inBattle

    // In room
    ClientRequestRoomUpdate, // Client > Server : when joined
    RoomUpdate,              // Server > Room: when theres snapshot updated
    ClientToggleReady,       // Cleint > Server
    ClientSetCreature,       // Client > Server : creatureID
    RoomChat,                // Client > Server : msg | Server > room: name, id, msg
    RoomEmote,               // Client > Server emoteId | Server> (room? client?): name, id, emote
    RoomLeft,                // Server > CLient Maybe in chat? :"You have left room "name"" 
    GameStart,               // Server>Room    "countdown finished, go to battle"
    CountdownStart,            // Server > Room: seconds
    CountdownCancel,           // Server > Room

    // Battle
    ClientRequestBattleUpdate, // Client > Server: roomId
    BattleUpdate,              // Server > Client: round, count, then per player: id, name, creature, hp, maxHp
    ClientChooseMove,          // Client > Server: roomId, move (0 Attack, 1 Defend, 2 Break)
    MoveLocked,                // Server > Room: playerId
    RoundResult,               // Server > Room: round, count, then per player: id, move, damageTaken, hp
    BattleEnd,                 // Server > Room: winnerId (-1 = draw)
    BattleEmote                // Client > Server: roomId, emoteId | Server > Room: name, id, emoteId
};

struct PacketHeader
{
	PacketType type;
	std::uint16_t sequenceId;
};

// Overload sf::Packet's << and >> so we can push or pull PacketType
sf::Packet& operator <<
(sf::Packet& packet, const PacketType& type);

sf::Packet& operator >>
(sf::Packet& packet, PacketType& outType);