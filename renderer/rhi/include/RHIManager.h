#pragma once
#include <memory>
#include "RHIDefine.h"
#include "RHIDevice.h"
namespace raum::rhi {

RAUM_API RHIDevice* loadRHI(API api);

} // namespace raum::rhi