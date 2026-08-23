#pragma once

#include <algorithm>
#include <vector>
#include <functional>

template<class T>
class EventSource
{
public:
    using EventFunction = std::function<void(const T&, void* sender)>;
private:
    struct EventHandler
    {
        size_t ID = 0;
        EventFunction Handler;
    };

    std::vector<EventHandler> EventHandlers;

public:
    inline void Add(EventFunction handler, size_t id = size_t(-1))
    {
        EventHandlers.emplace_back(EventHandler{ id, handler });
    }

    inline void Remove(size_t id)
    {
        std::remove_if(EventHandlers.begin(), EventHandlers.End(), [id](const EventHandler& handler) { return handler.ID == id; });
    }

    inline void Invoke(const T& value, void* sender = nullptr)
    {
        for (auto& handler : EventHandlers)
        {
            handler.Handler(value, sender);
        }
    }
};