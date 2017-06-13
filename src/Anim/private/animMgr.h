#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::animMgr
    @ingroup _priv
    @brief resource container of the Anim module
*/
#include "Resource/ResourceContainerBase.h"
#include "Resource/ResourcePool.h"
#include "Anim/AnimTypes.h"
#include "Anim/private/animInstance.h"

namespace Oryol {
namespace _priv {

class animMgr {
public:
    /// destructor
    ~animMgr();

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

    /// create a skeleton
    Id createSkeleton(const AnimSkeletonSetup& setup);
    /// lookup pointer to skeleton
    AnimSkeleton* lookupSkeleton(const Id& resId);
    /// destroy a skeleton
    void destroySkeleton(const Id& resId);

    /// create an animation instance
    Id createInstance(const AnimInstanceSetup& setup);
    /// lookup pointer to an animation instance
    animInstance* lookupInstance(const Id& resId);
    /// destroy an animation instance
    void destroyInstance(const Id& resId);

    /// remove a range of keys from key pool and fixup indices in curves and clips
    void removeKeys(Slice<float> keyRange);
    /// remove a range of curves from curve pool, and fixup clips
    void removeCurves(Slice<AnimCurve> curveRange);
    /// remove a range of clips from clip pool, and fixup libraries
    void removeClips(Slice<AnimClip> clipRange);
    /// remove a range of matrices from the matrix pool
    void removeMatrices(Slice<glm::mat4> matrixRange);

    static const Id::TypeT resTypeLib = 1;
    static const Id::TypeT resTypeSkeleton = 2;
    static const Id::TypeT resTypeInstance = 3;

    bool isValid = false;
    ResourceContainerBase resContainer;
    ResourcePool<AnimLibrary> libPool;
    ResourcePool<AnimSkeleton> skelPool;
    Array<AnimClip> clipPool;
    Array<AnimCurve> curvePool;
    Array<glm::mat4> matrixPool;
    int numKeys = 0;
    Slice<float> keys;
    float* valuePool = nullptr;
};

} // namespace _priv
} // namespace Oryol
