#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::animMgr
    @ingroup _priv
    @brief resource container of the Anim module
*/
#include "Resource/ResourceContainerBase.h"
#include "Resource/ResourcePool.h"

namespace Oryol {
namespace _priv {

class animMgr {
public:
    /// setup the anim mgr
    void setup(const AnimSetup& setup);
    /// discard the anim mgr
    void discard();

    /// create a clip
    Id createClip(const AnimClipSetup& setup);
    /// lookup animClip object 
    animClip* lookupClip(const Id& resId);
    /// immediately destroy anim resources by label
    void destroy(const ResourceLabel& label);

    ResourceContainerBase resContainer;
    ResourcePool<AnimClip> clipPool;
    Array<AnimCurve> curvePool;
    int maxKeys = 0;        // max number of floats in keypool
    int numKeys = 0;        // current number of keys in keypool
    float* keyPool = nullptr;
};

} // namespace _priv
} // namespace Oryol
