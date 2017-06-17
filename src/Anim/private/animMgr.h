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

    /// write animition library keys
    void writeKeys(AnimLibrary* lib, const uint8_t* ptr, int numBytes);

    /// begin a new frame, resets the active instances
    void newFrame();
    /// add an active instance for the current frame
    bool addActiveInstance(animInstance* inst);
    /// evaluate all active instances, and reset active instance array
    void evaluate(double frameDurationInSeconds);

    /// start an animation on an instance (active or inactive)
    AnimJobId play(animInstance* inst, const AnimJob& job);
    /// stop a specific anim job
    void stop(animInstance* inst, AnimJobId jobId, bool allowFadeOut);
    /// stop all anim jobs on a track
    void stopTrack(animInstance* inst, int trackIndex, bool allowFadeOut);
    /// stop all anim jobs
    void stopAll(animInstance* inst, bool allowFadeOut);

    static const Id::TypeT resTypeLib = 1;
    static const Id::TypeT resTypeSkeleton = 2;
    static const Id::TypeT resTypeInstance = 3;

    bool isValid = false;
    bool inFrame = false;
    double curTime = 0.0;
    uint32_t curAnimJobId = 0;
    ResourceContainerBase resContainer;
    ResourcePool<AnimLibrary> libPool;
    ResourcePool<AnimSkeleton> skelPool;
    ResourcePool<animInstance> instPool;
    Array<AnimClip> clipPool;
    Array<AnimCurve> curvePool;
    Array<glm::mat4> matrixPool;
    Array<animInstance*> activeInstances;
    int numKeys = 0;
    Slice<float> keys;
    int numSamples = 0;
    Slice<float> samples;
    float* valuePool = nullptr;
};

} // namespace _priv
} // namespace Oryol
