#pragma once
#include <memory>
#include "RHIDefine.h"
#include "RHIDevice.h"
namespace raum::rhi {

RAUM_API RHIDevice* loadRHI(API api);
RAUM_API void unloadRHI(RHIDevice*);

} // namespace raum::rhi