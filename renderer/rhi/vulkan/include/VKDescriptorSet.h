#pragma once
#include "RHIDescriptorSet.h"
#include "VKDefine.h"
namespace raum::rhi {
class DescriptorSet : public RHIDescriptorSet {
public:
	
	VkDescriptorSet descriptorSet() const { return _descriptorSet; }

private:
    VkDescriptorSet _descriptorSet;

};
}