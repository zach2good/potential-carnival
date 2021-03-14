#include "BearLibTerminal.h"
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iostream>
#include <string>
#include <thread>

#include "asio.hpp"
#include "blocking_deque.h"
#include "packet.h"
#include "udp_port_watcher.h"
#include "util.h"

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
    uint32_t x;
    uint32_t y;
    uint8_t  f;

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
    switch (c.f)
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
    Direction facing;
};

class connection_to_server_t : public udp_port_watcher_t
{
public:
    connection_to_server_t(const char* address, short port)
    : udp_port_watcher_t(port + 1)
    , server_endpoint(asio::ip::address::from_string(address), port)
    {
    }

    ~connection_to_server_t() override = default;

    void on_packet_received(std::shared_ptr<packet_t>&& packet) override
    {
        if (packet->get_type() == LOGIN_REPLY)
        {
            auto reply = std::static_pointer_cast<login_reply_packet>(packet);
            if (reply->success())
            {
                ident = reply->ident();
            }
        }
        else if (packet->get_type() == CHAT_MESSAGE)
        {
            auto reply = std::static_pointer_cast<chat_message_packet>(packet);
            chat_messages.emplace_back(reply->message());
        }
    }

    void send(std::shared_ptr<packet_t>&& packet)
    {
        if (ident)
        {
            packet->set_client_ident(ident);
        }
        packet->sender_endpoint = server_endpoint;
        m_tx_deque.emplace_back(std::move(packet));
    }

    template <typename T, typename... Args>
    void send(Args&&... args)
    {
        auto packet = std::make_shared<T>(std::forward<Args>(args)...);
        send(std::move(packet));
    }

    void send_heartbeat(Char& c)
    {
        send<heartbeat_ping_packet>();
    }

    void send_login_request(Char& c)
    {
        send<login_request_packet>("Raguza");
    }

    void send_pos_update(Char& c)
    {
        send<position_update_packet>(c.x, c.y, c.f);
    }

    void send_chat_message(std::string message)
    {
        send<chat_message_packet>(message);
    }

    // Chat
    util::blocking_deque<std::string> chat_messages;

private:
    asio::ip::udp::endpoint server_endpoint;

    // Ident + Session tracking
    uint16_t ident;
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

    // Networking
    connection_to_server_t connection("127.0.0.1", 4444);

    // TODO: State machine
    // Login -> Download data -> Gameplay
    connection.send_login_request(c);

    connection.send_chat_message("Hello!");
    for (int i = 0; i < 10; ++i)
    {
        connection.send_chat_message(std::to_string(i));
    }

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
                    move_queue.push_back({ Up });
                }
                break;
                case TK_S: // Down
                {
                    move_queue.push_back({ Down });
                }
                break;
                case TK_A: // Left
                {
                    move_queue.push_back({ Left });
                }
                break;
                case TK_D: // Right
                {
                    move_queue.push_back({ Right });
                }
                break;
                case TK_Q: // Clear move_queue
                {
                    move_queue.clear();
                }
                break;
                case TK_SPACE: // Spell
                {
                    cast_spell(c.x, c.y, (Direction)c.f);
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
            connection.send_heartbeat(c);

            last_tick = now;
            ++ticks;

            // Tickable Game State
            if (!move_queue.empty())
            {
                auto move = move_queue.front();
                move_queue.pop_front();

                c.f = move.facing;
                switch (move.facing)
                {
                    case Direction::Right:
                    {
                        c.x += 1;
                        c.y += 0;
                    }
                    break;
                    case Direction::Down:
                    {
                        c.x += 0;
                        c.y += 1;
                    }
                    break;
                    case Direction::Left:
                    {
                        c.x += -1;
                        c.y += 0;
                    }
                    break;
                    case Direction::Up:
                    {
                        c.x += 0;
                        c.y += -1;
                    }
                    break;
                }

                connection.send_pos_update(c);
            }
        }

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
            //startx += move.dx;
            //starty += move.dy;
            //terminal_print(startx, starty, ".");
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
        terminal_printf(1, 1, "X: %i, Y: %i, Ticks: %i, q.size(): %i", c.x, c.y, ticks, move_queue.size());

        auto& messages = connection.chat_messages;
        std::size_t size = std::min(messages.size(), 5U);
        for (int i = 0; i < size; ++i)
        {
            terminal_color(color_from_argb(255 - (i * 40), 255, 255, 255));
            auto message = messages.at(messages.size() - 1 - i);
            terminal_printf(1, 22 - i, "> %s", message.c_str());
        }

        // Submit
        terminal_refresh();

        // Sleep
        terminal_delay(1000 / fps);
    }

    terminal_close();
}
