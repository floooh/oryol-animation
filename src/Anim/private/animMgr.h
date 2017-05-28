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

    /// destroy one or more resources by label
    void destroy(const ResourceLabel& label);

    /// create an animation library
    Id createLibrary(const AnimLibrarySetup& setup);
    /// lookup pointer to an animation library
    AnimLibrary* lookupLibrary(const Id& resId);
    /// destroy an animation library
    void destroyLibrary(const Id& resId);

    /// remove a range of keys from key pool and fixup indices in curves and clips
    void removeKeys(const ArrayView<float>& keyRange);
    /// remove a range of curves from curve pool, and fixup clips
    void removeCurves(const ArrayView<AnimCurve>& curveRange);
    /// remove a range of clips from clip pool, and fixup libraries
    void removeClips(const ArrayView<AnimClip>& clipRange);

    static const Id::TypeT resTypeLib = 1;
    static const Id::TypeT resTypeInstance = 2;

    bool isValid = false;
    ResourceContainerBase resContainer;
    ResourcePool<AnimLibrary> libPool;
    Array<AnimClip> clipPool;
    Array<AnimCurve> curvePool;
    int numKeys = 0;
    int numSamples = 0;
    ArrayView<float> keys;
    ArrayView<float> samples;
    float* valuePool = nullptr;
};

} // namespace _priv
} // namespace Oryol
