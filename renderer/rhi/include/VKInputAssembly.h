#pragma once
#include <vcruntime_new.h>
#include <vector>
#include "VKDefine.h"

namespace raum::rhi {
class VertexBuffer;
class IndexBuffer;

using VertexBuffers = std::vector<VertexBuffer*>;
using IndexBuffers = std::vector<IndexBuffer*>;

class InputAssembly {
public:
    InputAssembly() = delete;
    InputAssembly(const InputAssembly&) = delete;
    InputAssembly& operator=(const InputAssembly&) = delete;
    InputAssembly(InputAssembly&&) = delete;

    explicit InputAssembly(const VertexBuffers& vbs, const IndexBuffers& ibs);
    explicit InputAssembly(VertexBuffers&& vbs, IndexBuffers&& ibs);
    ~InputAssembly();

    IAType type() { return _iaType; }

private:
    IAType _iaType;
    VertexBuffers _vbs;
    IndexBuffers _ibs;
};

} // namespace raum::rhi
