# 🎠 potential-carnival
Experiments in MMO design.

## Goals
I have an ongoing interest in MMORPG design and implementation. I have also previously been interested in roguelikes. I would like to try and combine these two interestes and implement a very simple MMORPG, presented as a roguelike.

## Ideas
### Architecture
I would like to present a seamless world. In order to distribute processing load, distinct gameplay areas will need to be hosted on seperate processes or servers. I will need to provide a way of allowing players to interact with other entities across the invisible boundaries between processes.
- <https://gameserverarchitecture.com/2016/03/pattern-seamless-world-game-server/>

### Design
- Login Server
- Game Servers (x N)
- World Server
- Char Server
- DB Server (+ DB)

### Parts (?)
- C++ coroutines TS
- C++ networking TS
- CMake
- Google Protobuff (https://developers.google.com/protocol-buffers)
- FlatBuffers (https://google.github.io/flatbuffers/)
- ZeroMQ (https://zeromq.org/)
- Lua or LuaJIT, with Sol3


### Client
I like the idea of a web-based client, so it is very quick for someone to make a character and jump into the game. Given that I'm awful at web-anything, I'll start with a simple native client written in C++ and see where I go after that. I will probably use `libtcod` to present the roguelike aesthetic I'm interested in.

`libtcod` allows drawing to an offscreen buffer, enabling transparency. Nice feature.

I'm quite interested in being able to show the direction an entity is facing. If I want to maintain this aestheric, I need to introduce a marker to show direction.
In this case, `@` is the entity and `?` is a torch they are holding, representing them as facing east.
```
.........................
.........................
.........................
..........@?.............
.........................
.........................
```

There is another option of doing what Dwarf Fortress does and showing additional information in the same square by alternating which tile is shown every second or so. I really don't like this approach for my project.

### Gameplay
In order to stop the game becoming realtime, I need to tie everything to a `tick`, which all clients, entities and the server share. After looking up similar projects, I found `Isleward` which uses a very pleasing queuing system. This allows a player to enter multiple things into an action queue, have them represented on screen, and as the game ticks it will take things from that queue.

Combine the "holy trinity" of MMORPG classes: tank, dps, and healer, with the three categories of ranged interaction (as I see them): melee, ranged, aoe.
- melee tank
- ranged tank
- aoe tank
- melee dps
- ranged dps
- aoe dps
- melee healer
- ranged healer
- aoe healer

How do these work? No idea.
