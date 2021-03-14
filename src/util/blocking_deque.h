#pragma once

#include <deque>
#include <mutex>

namespace util
{
    template <typename T>
    class blocking_deque
    {
    public:
        blocking_deque()
        : m_deque()
        {
        }

        T& at(std::size_t index)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_deque.at(index);
        }

        std::size_t size()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_deque.size();
        }

        bool empty()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_deque.empty();
        }

        T& front()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_deque.front();
        }

        T& back()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_deque.back();
        }

        void emplace_front(T&& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_deque.emplace_front(item);
        }

        void emplace_back(T&& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_deque.emplace_back(item);
        }

        void pop_front()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_deque.pop_front();
        }

        void pop_back()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_deque.pop_back();
        }

    private:
        std::deque<T> m_deque;
        std::mutex    m_mutex;
    };
} // namespace util