
#include "VKDefine.h"
#include "RHISampler.h"
namespace raum::rhi {
class Device;
class Sampler: public RHISampler {
public:
    explicit Sampler(const SamplerInfo& samplerInfo, RHIDevice* device);

    const VkSampler sampler() const { return _sampler; }

private:
    ~Sampler();

    VkSampler _sampler;
    Device* _device;
};


}