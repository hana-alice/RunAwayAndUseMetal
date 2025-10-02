#pragma once

#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>
#include <exec/any_sender_of.hpp>

namespace raum {
template <class... Ts>
    using any_sender_of =
        typename exec::any_receiver_ref<stdexec::completion_signatures<Ts...>>::template any_sender<>;

    exec::static_thread_pool& getIOThreadPool();
}