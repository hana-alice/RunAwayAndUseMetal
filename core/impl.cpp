#include <exec/static_thread_pool.hpp>

#include "utils/containers.h"

namespace raum {

TrackedResource* getGlobalTrackedResource() {
    static TrackedResource* res = new TrackedResource("Global Tracked Resource", std::pmr::get_default_resource());
    return res;
}

exec::static_thread_pool& getIOThreadPool() {
    static exec::static_thread_pool ioThreadPool((std::thread::hardware_concurrency() + 1) / 2);
    return ioThreadPool;
}

}