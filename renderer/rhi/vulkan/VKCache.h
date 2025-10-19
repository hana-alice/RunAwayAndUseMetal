#pragma once

#include "core/utils/Archive.h"

namespace raum::rhi {

class Device;
class ProgramCache {
public:
    ProgramCache() = default;
    ProgramCache(const ProgramCache&) = delete;
    ProgramCache(ProgramCache&&) = delete;
    ProgramCache& operator=(const ProgramCache&) = delete;
    ProgramCache& operator=(ProgramCache&&) = delete;
    ~ProgramCache() = default;

    explicit ProgramCache(Device* device);

    bool needRegeneratePSO() const { return _needRegeneratePSO; }

    void validate();

    void flush();

private:
    bool _needRegeneratePSO{false};
    Device* _device{nullptr};
};

}