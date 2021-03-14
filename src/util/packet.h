#pragma once

struct packet_t
{
    // Buffer Data
    // 0 - 3 = Header
    // 4 - 1023 = Body ("Data")
    std::array<uint8_t, 1024> buffer{{0}};

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

    uint8_t get_type()
    {
        return ref<uint8_t>(2);
    }

    void set_type(uint8_t type)
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

    // TODO: Move these into child classes, inheriting from this
    uint32_t& position_x()
    {
        return ref<uint32_t>(4);
    }

    uint32_t& position_y()
    {
        return ref<uint32_t>(8);
    }

    uint8_t& position_facing()
    {
        return ref<uint8_t>(12);
    }
};