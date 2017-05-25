#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::animMgr
    @ingroup _priv
    @brief resource container of the Anim module
*/
#include "Resource/ResourceContainerBase.h"
#include "Resource/ResourcePool.h"
#include "Anim/AnimTypes.h"

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
    /// lookup an pointer to animClip object 
    AnimClip* lookupClip(const Id& resId);
    /// immediately destroy anim resources by label
    void destroy(const ResourceLabel& label);
    /// destroy an anim clip (called by generic destroy function)
    void destroyClip(const Id& resId);

    /// remove a range of keys from key pool and fixup indices in curves and clips
    void removeKeys(int keyIndex, int numKeys);
    /// remove a range of curves from curve pool, and fixup clips
    void removeCurves(int curveIndex, int numCurves);

    static const Id::TypeT resTypeClip = 1;

    bool isValid = false;
    ResourceContainerBase resContainer;
    ResourcePool<AnimClip> clipPool;
    Array<AnimCurve> curvePool;
    int maxKeys = 0;        // max number of floats in keypool
    int numKeys = 0;        // current number of keys in keypool
    float* keyPool = nullptr;
};

} // namespace _priv
} // namespace Oryol
