#pragma once
#include <stdint.h>
#include <functional>
#include <vector>
namespace raum::framework {

enum class InputEventType : uint32_t {
    MOUSE,
    KEYBOARD,
    WINDOW_RESIZE,
    WINDOW_CLOSE,
};

enum class EventType : uint8_t {
    INPUT,
    OUTPUT,
};

class Event {
public:
    Event() = delete;
    Event(EventType type);

    EventType eventType() const {
        return _evtType;
    }

protected:
    EventType _evtType;
};

class EventMaker;

class InputEvent : public Event {
public:
    InputEvent() = delete;

    InputEvent(InputEventType type);

    InputEventType type() const {
        return _type;
    }

private:
    InputEventType _type;
};

class OutputEvent : public Event {
public:
    OutputEvent();
};


template <typename... Args>
struct Pack {};

template <InputEventType type, typename... Args>
struct EventTag {
    static constexpr InputEventType EventType = type;
    using CallbackType = std::function<void(Args...)>;
    using Params = std::tuple<Args...>;
};

// typedef  EventTag<InputEventType::MOUSE, uint32_t, uint32_t, uint32_t> MouseButtonEventTag;

// https://indii.org/blog/is-type-instantiation-of-template/
template <class T, template <class...> class U>
inline constexpr bool is_instance_of_v = std::false_type{};

template <template <class...> class U, class... Vs>
inline constexpr bool is_instance_of_v<U<Vs...>, U> = std::true_type{};

template <class T, template <InputEventType, class...> class U>
inline constexpr bool is_instance_of_event_tag = std::false_type{};

template <template <InputEventType, class...> class U, InputEventType type, class... Vs>
inline constexpr bool is_instance_of_event_tag<U<type, Vs...>, U> = std::true_type{};

template <typename Tag>
concept TagInst = requires { is_instance_of_event_tag<Tag, EventTag>; };


template <typename Tag>
    requires TagInst<Tag>
class EventListener {
public:
    using ListenFunc = typename Tag::CallbackType;

    EventListener();

    void add(const ListenFunc& func);

    void invoke(typename Tag::Params&& params);

    void remove();

private:
    ListenFunc _callback;
};

template <typename Tag>
    requires TagInst<Tag>
class EventDispatcher {
public:
    static EventDispatcher* get() {
        static EventDispatcher evtDispatcher;
        return &evtDispatcher;
    }

    void broadcast(Tag::Params&& args) {
        for (auto& l : _listeners) {
            l->invoke(std::forward<typename Tag::Params>(args));
        }
    }

    void push(EventListener<Tag>* listener) {
        _listeners.emplace_back(listener);
    }

    void remove(EventListener<Tag>* listener) {
        for (auto iter = _listeners.begin(); iter != _listeners.end(); ++iter) {
            if (*iter == listener) {
                _listeners.erase(iter);
                return;
            }
        }
    }

private:
    EventDispatcher() {}

    std::list<EventListener<Tag>*> _listeners;
};

template <typename Tag>
    requires TagInst<Tag>
EventListener<Tag>::EventListener() {
    EventDispatcher<Tag>::get()->push(this);
}

template <typename Tag>
    requires TagInst<Tag>
void EventListener<Tag>::remove() {
    EventDispatcher<Tag>::get()->remove(this);
}

template <typename Tag>
    requires TagInst<Tag>
void EventListener<Tag>::add(const ListenFunc& func) {
    _callback = func;
}

template <typename Tag>
    requires TagInst<Tag>
void EventListener<Tag>::invoke(typename Tag::Params&& params) {
    if(_callback) {
        std::apply(_callback, params);
    }
}

} // namespace raum::framework