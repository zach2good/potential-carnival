#include <asio.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include "blocking_deque.h"
#include "packet.h"
#include "udp_port_watcher.h"
#include "util.h"

enum class EVENT_TYPE_IN : uint8_t
{
    Heartbeat         = 0x00,
    RegisterCharacter = 0x01,
    PositionUpdate    = 0x02,
    Invalid,
};

struct position_t
{
    uint32_t x;
    uint32_t y;
    uint8_t  f;
};

struct character_t
{
    std::string name;
    position_t pos;
};

struct session_t
{
    uint64_t                                       ident;
    asio::ip::udp::endpoint                        endpoint;
    std::chrono::high_resolution_clock::time_point last_update;
    std::shared_ptr<character_t>                   character;
};

class server : public udp_port_watcher_t
{
public:
    server(short port)
    : udp_port_watcher_t(port)
    {
        std::cout << "Server running...\n";
    }

    virtual ~server()
    {
    }

    void on_packet_received(std::shared_ptr<packet_t>&& packet) override
    {
        // Retrieve session using ident key
        auto session = get_or_create_session(packet->get_full_client_ident());

        // TODO: Breakout into handlers
        switch (packet->get_type())
        {
            case HEARTBEAT_PING:
            {
                packet->set_type(HEARTBEAT_PONG);
                m_tx_deque.emplace_back(std::move(packet));
                return;
            }
            break;
            case LOGIN_REQUEST:
            {
                auto request = std::static_pointer_cast<login_request_packet>(packet);
                auto name = request->name();

                auto character = std::make_shared<character_t>();
                character->name = name;

                session->character = character;

                // TODO: Generate sane client ident keys
                uint16_t ident = 4444;

                auto reply = std::make_shared<login_reply_packet>(true, ident);
                reply->sender_endpoint = packet->sender_endpoint;
                m_tx_deque.emplace_back(std::move(reply));
                return;
            }
            case POSITION_UPDATE:
            {
                auto update = std::static_pointer_cast<position_update_packet>(packet);
                if (session->character)
                {
                    session->character->pos = { update->x(), update->y(), update->f() };
                }
                return;
            }
            break;
            case CHAT_MESSAGE:
            {
                auto chat = std::static_pointer_cast<chat_message_packet>(packet);
                auto message = chat->message();
                std::cout << message << "\n";
                m_tx_deque.emplace_back(std::move(chat));
                return;
            }
            break;
            default:
                break;
        }

        std::cout << "Unable to turn packet into event " << packet->sender_endpoint.address().to_string() << " " << packet->sender_endpoint.port() << "\n";
    }

    std::shared_ptr<session_t> get_or_create_session(uint64_t ident)
    {
        std::shared_ptr<session_t> session;
        if (m_sessions.find(ident) != m_sessions.end())
        {
            session = m_sessions.at(ident);
        }
        else
        {
            session           = std::make_shared<session_t>();
            m_sessions[ident] = session;

            session->ident = ident;
        }

        session->last_update = std::chrono::high_resolution_clock::now();

        return session;
    }

private:
    // Sessions
    std::unordered_map<uint64_t, std::shared_ptr<session_t>> m_sessions;

    // Characters
    std::unordered_map<uint64_t, std::shared_ptr<character_t>> m_characters;
};
