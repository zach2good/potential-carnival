#pragma once

#include "packet.h"

class udp_port_watcher_t
{
public:
    udp_port_watcher_t(short port)
    : m_socket(m_io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
    , m_read_strand(m_io_context)
    , m_write_strand(m_io_context)
    , m_running(true)
    {
        try
        {
            m_socket.set_option(asio::socket_base::reuse_address(true));

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
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    virtual ~udp_port_watcher_t()
    {
        m_io_context.stop();
        m_ioc_worker.join();

        m_running = false;
        m_packet_worker.join();
    }

    bool get_running()
    {
        return m_running;
    }

    void set_running(bool running)
    {
        m_running = running;
    }

protected:
    virtual void on_packet_received(std::shared_ptr<packet_t>&& packet) = 0;

    // Deques
    util::blocking_deque<std::shared_ptr<packet_t>> m_rx_deque;
    util::blocking_deque<std::shared_ptr<packet_t>> m_tx_deque;

private:
    void do_receive()
    {
        auto packet = std::make_shared<packet_t>();
        m_socket.async_receive_from(
            asio::buffer(packet->buffer), packet->sender_endpoint,
            asio::bind_executor(m_read_strand,
                                std::bind(&udp_port_watcher_t::handle_receive, this,
                                          packet, std::placeholders::_1)));
    }

    void handle_receive(std::shared_ptr<packet_t> packet, std::error_code ec)
    {
        if (ec)
        {
            std::cout << ec.message() << "\n";
        }
        else
        {
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
                asio::buffer(packet->buffer),
                packet->sender_endpoint,
                asio::bind_executor(m_write_strand,
                                    std::bind(&udp_port_watcher_t::handle_send, this,
                                              std::placeholders::_1)));
        }
        else
        {
            m_write_strand.post([&]() { do_send(); });
        }
    }

    void handle_send(std::error_code ec)
    {
        if (ec)
        {
            std::cout << ec.message() << "\n";
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

            on_packet_received(std::move(packet));
        }
    }

    // Networking
    asio::io_context         m_io_context;
    asio::ip::udp::socket    m_socket;
    asio::io_context::strand m_read_strand;
    asio::io_context::strand m_write_strand;

    // Threading
    std::atomic<bool> m_running;
    std::thread       m_ioc_worker;
    std::thread       m_packet_worker;
};
