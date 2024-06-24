#pragma once
#include "RHIDefine.h"
namespace raum::asset {
	auto generateBRDFLUT(rhi::CommandBufferPtr cmdBuffer, rhi::DevicePtr device) -> std::tuple<rhi::ImagePtr, rhi::ImageViewPtr>;
}