#include "BearLibTerminal.h"
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iostream>
#include <string>
#include <thread>

#include "asio.hpp"

enum Direction
{
    Right = 0,
    Down  = 1,
    Left  = 2,
    Up    = 3,
};

enum ItemType
{
    WEAPON,
    HAT,
    INVALID_ITEMTYPE,
};

struct Item
{
    ItemType    type;
    std::string ch;
    std::string color;
};

struct Char
{
    int  x;
    int  y;
    int  facing;
    Item weapon;
    Item hat;
};

void print_char(Char& c)
{
    // Char Layers
    // Behind:  10
    // Capes:   11
    // Body:    12
    // Armour:  13
    // Hats:    14
    // Weapons: 15
    // MagicFX: 16
    // Front:   17

    // Body
    terminal_layer(12);
    terminal_color(color_from_name("white"));
    terminal_print(c.x, c.y, "@");

    // Hats
    terminal_layer(14);
    terminal_print_ext(c.x, c.y, 0, 0, 0, "[color=red][offset=0,-8]^");

    // Weapons
    terminal_layer(15);
    switch (c.facing)
    {
        case 0: // Right
            terminal_print_ext(c.x, c.y, 0, 0, 0, "[color=grey][offset=16,0]/");
            break;
        case 1: // Down
            // If facing Down, draw the weapon in front of the char
            terminal_layer(17);

            terminal_print_ext(c.x, c.y, 0, 0, 0, "[color=grey][offset=10,5]/");
            break;
        case 2: // Left
            terminal_print_ext(c.x, c.y, 0, 0, 0, "[color=grey][offset=-16,0]\\");
            break;
        case 3: // Up
            // If facing Up, draw the weapon behind the char
            terminal_layer(10);

            terminal_print_ext(c.x, c.y, 0, 0, 0, "[color=grey][offset=0,-8]/");
            break;
    }
}

void cast_spell(int x, int y, Direction dir)
{
}

struct Move
{
    int dx;
    int dy;
    // HACK:
    Direction facing;
};

struct socket_t
{
    socket_t(std::string address_str, int port)
    : socket(io_context)
    , endpoint(asio::ip::address::from_string(address_str), port)
    {
        socket.open(asio::ip::udp::v4());
        socket.non_blocking(true);
    }

    ~socket_t()
    {
        close();
    }

    void close()
    {
        socket.close();
    }

    void send(std::vector<char> buffer)
    {
        socket.send_to(asio::buffer(buffer.data(), buffer.size()), endpoint);
    }

    void send_heartbeat(Char& ch)
    {
        send({ 0x00, 0x00, 0x00, 0x00 });
    }

    void send_register_char(Char& ch)
    {
        send({ 0x00, 0x00, 0x01, 0x00 });
    }

    void send_pos_update(Char& ch)
    {
        send({ 0x00, 0x00, 0x02, 0x00 });
    }

    std::vector<char> listen()
    {
        asio::ip::udp::endpoint sender;
        std::vector<char>       buffer(1024);
        asio::error_code        error             = asio::error::would_block;
        std::size_t             bytes_transferred = 0;
        while (error == asio::error::would_block)
        {
            bytes_transferred = socket.receive_from(asio::buffer(buffer), sender, 0, error);
        }
        buffer.resize(bytes_transferred);
        return buffer;
    }

    asio::io_context        io_context;
    asio::ip::udp::socket   socket;
    asio::ip::udp::endpoint endpoint;
};

int main()
{
    using namespace std::chrono_literals;

    // Data
    const int   fps       = 30;
    const float g_pi      = 3.141592654f;
    float       angle     = 0.0f;
    int         ticks     = 0;
    auto        last_tick = std::chrono::high_resolution_clock::now().time_since_epoch();

    std::array<int, 80 * 25> world{ '.' };

    terminal_open();
    terminal_set("window: size=80x25; font: ./PressStart2P-Regular.ttf, size=16");
    terminal_composition(TK_ON);

    Char c{ 10, 10, 0, { WEAPON, "/", "grey" }, { HAT, "^", "red" } };

    std::deque<Move> move_queue;

    int heartbeat = 0;

    // Networking
    socket_t    socket("127.0.0.1", 4444);
    bool        socket_listening = true;
    std::thread socket_thread([&]() {
        while (socket_listening)
        {
            auto reply = socket.listen();
            if (!reply.empty())
            {
                if (reply.size() == 4 && reply[0] + reply[1] + reply[2] + reply[3] == 0)
                {
                    ++heartbeat;
                }
            }
        }
    });

    bool closing = false;
    while (!closing)
    {
        // Clear
        terminal_clear();

        // Handle Input
        if (terminal_has_input())
        {
            auto code = terminal_read();
            if (code == TK_ESCAPE)
            {
                closing = true;
            }

            switch (code)
            {
                case TK_W: // Up
                {
                    move_queue.push_back({ 0, -1, Up });
                }
                break;
                case TK_S: // Down
                {
                    move_queue.push_back({ 0, 1, Down });
                }
                break;
                case TK_A: // Left
                {
                    move_queue.push_back({ -1, 0, Left });
                }
                break;
                case TK_D: // Right
                {
                    move_queue.push_back({ 1, 0, Right });
                }
                break;
                case TK_Q: // Clear move_queue
                {
                    move_queue.clear();
                }
                break;
                case TK_SPACE: // Spell
                {
                    cast_spell(c.x, c.y, (Direction)c.facing);
                }
                break;
                default:
                break;
            }
        }

        // Handle ticks
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        if (now - last_tick > 400ms)
        {
            socket.send_heartbeat(c);

            last_tick = now;
            ++ticks;

            // Tickable Game State
            if (!move_queue.empty())
            {
                auto move = move_queue.front();
                move_queue.pop_front();

                c.x += move.dx;
                c.y += move.dy;
                c.facing = move.facing;

                socket.send_pos_update(c);
            }
        }

        // Handle Rendering
        /*
        // World (80 * 25)
        terminal_layer(2);
        terminal_color("grey");
        for (size_t i = 0; i < world.size(); i++)
        {
            // HACK: I forget the correct maths for this and can't be bothered to figure it out
            terminal_print(i % 80, i / 80, ".");
        }
        */

        // BG Effects
        //

        // Entities
        print_char(c);

        // Move Queue
        terminal_layer(10);
        int startx = c.x;
        int starty = c.y;
        for (auto& move : move_queue)
        {
            startx += move.dx;
            starty += move.dy;
            terminal_print(startx, starty, ".");
        }

        // FG Effects (16)
        terminal_layer(16);
        int symbols = 12;
        int radius  = 2;
        for (int i = 0; i < symbols; i++)
        {
            float angle_delta = 2.0f * g_pi / symbols;
            float dx          = std::cos(angle + i * angle_delta) * radius * terminal_state(TK_CELL_WIDTH);
            float dy          = std::sin(angle + i * angle_delta) * radius * terminal_state(TK_CELL_WIDTH) - 4;
            terminal_color(i % 2 ? "cyan" : "orange");
            terminal_put_ext(c.x, c.y, dx, dy, 'a' + i, nullptr);
        }
        angle += 2.0f * g_pi / (2.0f * fps);

        // GUI
        terminal_color(color_from_name("white"));
        terminal_printf(1, 1, "X: %i, Y: %i, Ticks: %i, q.size(): %i, heartbeat: %i", c.x, c.y, ticks, move_queue.size(), heartbeat);

        // Submit
        terminal_refresh();

        // Sleep
        terminal_delay(1000 / fps);
    }
    socket_listening = false;
    socket.close();
    socket_thread.join();

    terminal_close();
}
