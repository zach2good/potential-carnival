#pragma once

enum PACKET_TYPE
{
    // Client -> Server (0 - 49)
    HEARTBEAT_PING = 0,
    LOGIN_REQUEST,

    // Server -> Client (50 - 99)
    HEARTBEAT_PONG = 50,
    LOGIN_REPLY,

    // Shared (100+)
    POSITION_UPDATE = 100,
    CHAT_MESSAGE,
};

struct packet_t
{
    // Buffer Data
    // 0 - 3 = Header
    // 0: Client ident 1
    // 1: Client ident 2 // TODO: Replace with sequence num
    // 2: Packet type
    // 3: Data size

    // 4 - 1023 = Body ("Data")
    std::array<uint8_t, 1024> buffer{ { 0 } };

    template <typename T>
    T& ref(std::size_t index)
    {
        return ref<T>(buffer.data(), index);
    }

    template <typename T, typename U>
    T& ref(U* buffer_ptr, std::size_t index)
    {
        return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(buffer_ptr) + index);
    }

    // Sender Information
    asio::ip::udp::endpoint sender_endpoint;

    // Methods
    uint16_t get_client_ident()
    {
        return ref<uint16_t>(0);
    }

    void set_client_ident(uint16_t ident)
    {
        ref<uint16_t>(0) = ident;
    }

    PACKET_TYPE get_type()
    {
        return static_cast<PACKET_TYPE>(ref<uint8_t>(2));
    }

    void set_type(PACKET_TYPE type)
    {
        ref<uint8_t>(2) = type;
    }

    uint8_t get_size()
    {
        return ref<uint8_t>(3);
    }

    void set_size(uint8_t size)
    {
        ref<uint8_t>(3) = size;
    }

    // TODO: Better validation than this
    bool has_valid_header()
    {
        return get_client_ident() > 0;
    }

    uint64_t get_full_client_ident()
    {
        auto     address      = sender_endpoint.address();
        uint32_t address_uint = static_cast<uint32_t>(address.to_v4().to_uint());
        uint16_t port         = static_cast<uint16_t>(sender_endpoint.port());

        // 64 bit ident key
        // Full ident: client_ident (uint16_t) + address_int (uint32_t) + port (uint16_t)
        uint64_t full_ident = ((uint64_t)get_client_ident() << 48) + ((uint64_t)address_uint << 32) + (uint64_t)port;

        return full_ident;
    }
};

struct heartbeat_ping_packet : public packet_t
{
    heartbeat_ping_packet()
    {
        set_type(HEARTBEAT_PING);
    }
};

struct heartbeat_pong_packet : public packet_t
{
    heartbeat_pong_packet()
    {
        set_type(HEARTBEAT_PONG);
    }
};

struct login_request_packet : public packet_t
{
    login_request_packet(std::string name)
    {
        set_type(LOGIN_REQUEST);
        ref<uint8_t>(4) = name.size();
        for (int i = 0; i < name.size(); ++i)
        {
            ref<uint8_t>(5 + i) = name[i];
        }
    }

    std::string name()
    {
        return std::string(buffer.data() + 5, buffer.data() + 5 + ref<uint8_t>(4));
    }
};

struct login_reply_packet : public packet_t
{
    login_reply_packet(bool success, uint16_t ident)
    {
        set_type(LOGIN_REPLY);
        ref<bool>(4)     = success;
        ref<uint16_t>(5) = ident;
    }

    bool& success()
    {
        return ref<bool>(4);
    }

    uint16_t& ident()
    {
        return ref<uint16_t>(5);
    }
};

struct position_update_packet : public packet_t
{
    position_update_packet(uint32_t _x, uint32_t _y, uint8_t _f)
    {
        set_type(POSITION_UPDATE);
        x() = _x;
        y() = _y;
        f() = _f;
    }

    uint32_t& x()
    {
        return ref<uint32_t>(4);
    }

    uint32_t& y()
    {
        return ref<uint32_t>(8);
    }

    uint8_t& f()
    {
        return ref<uint8_t>(12);
    }
};

struct chat_message_packet : public packet_t
{
    chat_message_packet(std::string message)
    {
        set_type(CHAT_MESSAGE);
        ref<uint8_t>(4) = message.size();
        for (int i = 0; i < message.size(); ++i)
        {
            ref<uint8_t>(5 + i) = message[i];
        }
    }

    std::string message()
    {
        return std::string(buffer.data() + 5, buffer.data() + 5 + ref<uint8_t>(4));
    }
};
