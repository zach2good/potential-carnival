#include <asio.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <unordered_map>

#include "blocking_deque.h"
#include "util.h"

struct packet_t
{
    std::vector<char>       buffer = std::vector<char>(1024);
    asio::ip::udp::endpoint sender_endpoint;
};

enum class EVENT_TYPE_IN : uint8_t
{
    Heartbeat         = 0x00,
    RegisterCharacter = 0x01,
    PositionUpdate    = 0x02,
    Invalid,
};

struct event_in_t
{
    EVENT_TYPE_IN type;
    uint64_t ident;
    uint8_t size;
};

struct session_t
{
    uint64_t ident;
    std::chrono::high_resolution_clock::time_point last_update;
};

struct position_t
{
    int x;
    int y;
    int facing;
};

struct character_t
{
    position_t pos;
};

class server
{
public:
    server(short port)
    : m_socket(m_io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
    , m_read_strand(m_io_context)
    , m_write_strand(m_io_context)
    , m_running(true)
    {
        m_read_strand.post([&]() { do_receive(); });
        m_write_strand.post([&]() { do_send(); });

        m_ioc_worker = std::thread([&]() {
            m_io_context.run();
        });

        m_packet_worker = std::thread([&]() {
            while (m_running)
            {
                packet_to_event_loop();
            }
        });

        m_event_worker = std::thread([&]() {
          while (m_running)
          {
              event_loop();
          }
        });

        std::cout << "Server running...\n";
    }

    ~server()
    {
        m_io_context.stop();
        m_ioc_worker.join();

        m_running = false;
        m_event_worker.join();
        m_packet_worker.join();
    }

    void do_receive()
    {
        auto packet = std::make_shared<packet_t>();
        m_socket.async_receive_from(
            asio::buffer(packet->buffer), packet->sender_endpoint,
            asio::bind_executor(m_read_strand,
                                std::bind(&server::handle_receive, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2, packet)));
    }

    void handle_receive(std::error_code ec, std::size_t bytesRecvd,
                        std::shared_ptr<packet_t> packet)
    {
        if (ec)
        {
            std::cout << ec.message() << "\n";
        }
        else
        {
            packet->buffer.resize(bytesRecvd);
            m_rx_deque.emplace_back(std::move(packet));
        }

        m_read_strand.post([&]() { do_receive(); });
    }

    void do_send()
    {
        if (!m_tx_deque.empty())
        {
            auto packet = m_tx_deque.front();
            m_tx_deque.pop_front();

            m_socket.async_send_to(
                asio::buffer(packet->buffer.data(), packet->buffer.size()),
                packet->sender_endpoint,
                asio::bind_executor(m_write_strand,
                                    std::bind(&server::handle_send, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2)));
        }
        else
        {
            m_write_strand.post([&]() { do_send(); });
        }
    }

    void handle_send(std::error_code ec, std::size_t bytesRecvd)
    {
        if (ec)
        {
            std::cout << ec.message() << "\n";
            return;
        }

        m_write_strand.post([&]() { do_send(); });
    }

    void packet_to_event_loop()
    {
        if (!m_rx_deque.empty())
        {
            // Take from RX deque
            auto packet = m_rx_deque.front();
            m_rx_deque.pop_front();

            auto event = decode(std::move(packet));
            if (event)
            {
                m_event_in_deque.emplace_back(std::move(event));
            }
        }
    }

    void event_loop()
    {
        if (!m_event_in_deque.empty())
        {
            auto event = m_event_in_deque.front();
            m_event_in_deque.pop_front();
        }
    }

    bool is_running()
    {
        return m_running;
    }

    std::shared_ptr<event_in_t> decode(std::shared_ptr<packet_t>&& packet)
    {
        auto     address      = packet->sender_endpoint.address();
        uint32_t address_uint = static_cast<uint32_t>(address.to_v4().to_uint());
        uint16_t port         = static_cast<uint16_t>(packet->sender_endpoint.port());

        if (packet->buffer.size() >= 4)
        {
            // 32 bit header
            uint16_t client_ident = (packet->buffer[0] << 8) + (packet->buffer[1] & 0x00FF);
            uint8_t  type         = packet->buffer[2];
            uint8_t  size         = packet->buffer[3];

            // TODO: Validate type vs size
            //

            // 64 bit ident key
            // Full ident: client_ident (uint16_t) + address_int (uint32_t) + port (uint16_t)
            uint64_t full_ident = ((uint64_t)client_ident << 48) + ((uint64_t)address_uint << 32) + (uint64_t)port;

            // Retrieve session using ident key
            auto session = get_or_create_session(full_ident);

            // TODO: Breakout into handlers
            switch (static_cast<EVENT_TYPE_IN>(type))
            {
                case EVENT_TYPE_IN::Heartbeat:
                {
                    std::cout << "Heartbeat\n";
                    m_tx_deque.emplace_back(std::move(packet));

                    // TODO: Make this an event
                    return nullptr;
                }
                    break;
                case EVENT_TYPE_IN::RegisterCharacter:
                {
                    std::cout << "RegisterCharacter\n";
                    return nullptr;
                }
                break;
                case EVENT_TYPE_IN::PositionUpdate:
                {
                    std::cout << "PositionUpdate\n";
                    int x = packet->buffer[7];
                    int y = packet->buffer[11];
                    int facing = packet->buffer[12];
                    std::cout << x << " " << y << " " << facing << "\n";
                    return nullptr;
                }
                break;
                default: break;
            }
        }

        std::cout << "Unable to turn packet into event " << address.to_string() << " " << port << "\n";
        return nullptr;
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
            session = std::make_shared<session_t>();
            m_sessions[ident] = session;

            session->ident = ident;
        }

        session->last_update = std::chrono::high_resolution_clock::now();

        return session;
    }

private:
    // Networking
    asio::io_context         m_io_context;
    asio::ip::udp::socket    m_socket;
    asio::io_context::strand m_read_strand;
    asio::io_context::strand m_write_strand;

    // Deques
    util::blocking_deque<std::shared_ptr<packet_t>> m_rx_deque;
    util::blocking_deque<std::shared_ptr<packet_t>> m_tx_deque;
    util::blocking_deque<std::shared_ptr<event_in_t>> m_event_in_deque;

    // Threading
    std::atomic<bool> m_running;
    std::thread       m_ioc_worker;
    std::thread       m_packet_worker;
    std::thread       m_event_worker;

    // Sessions
    std::unordered_map<uint64_t, std::shared_ptr<session_t>> m_sessions;

    // Characters
    std::unordered_map<uint64_t, std::shared_ptr<character_t>> m_characters;
};
