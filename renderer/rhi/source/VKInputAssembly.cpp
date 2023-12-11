#include "VKInputAssembly.h"
namespace raum::rhi {
InputAssembly::InputAssembly(const VertexBuffers& vbs, const IndexBuffers& ibs) : _iaType(IAType::VB_IB) {
    _vbs = vbs;
    _ibs = ibs;
}

InputAssembly::InputAssembly(VertexBuffers&& vbs, IndexBuffers&& ibs) : _iaType(IAType::VB_IB) {
    _vbs = std::move(vbs);
    _ibs = std::move(ibs);
}

} // namespace raum::rhi