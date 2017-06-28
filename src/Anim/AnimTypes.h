#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/String/StringAtom.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/InlineArray.h"
#include "Core/Containers/Map.h"
#include "Resource/ResourceBase.h"
#include "Resource/Locator.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimConfig
    @ingroup Anim
    @brief animation system config constants
*/
struct AnimConfig {
    /// max number of bones in a skeleton
    static const int MaxNumSkeletonBones = 256;
    /// max number of curves in a clip
    static const int MaxNumCurvesInClip = MaxNumSkeletonBones * 3;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSetup
    @ingroup Anim
    @brief setup parameters for Anim module
*/
struct AnimSetup {
    /// max number of anim libraries
    int MaxNumLibs = 16;
    /// max number of skeleton
    int MaxNumSkeletons = 16;
    /// max overall number of anim instances
    int MaxNumInstances = 128;
    /// max number of active instances per frame
    int MaxNumActiveInstances = 128;
    /// max overall number of anim clips
    int ClipPoolCapacity = MaxNumLibs * 64;
    /// max overall number of anim curves
    int CurvePoolCapacity = ClipPoolCapacity * 256;
    /// max capacity of the (input) key pool in number of floats
    int KeyPoolCapacity = 4 * 1024 * 1024;
    /// max number of samples for visible instances (in number of floats)
    int SamplePoolCapacity = 4 * 1024 * 1024;
    /// max number of the skeleton matrix pool (2 matrices per bone)
    int MatrixPoolCapacity = 1024;
    /// skinning-matrix table width in number of vec4's
    int SkinMatrixTableWidth = 1024;
    /// skinning-matrix table height
    int SkinMatrixTableHeight = 64;
    /// initial resource label stack capacity
    int ResourceLabelStackCapacity = 256;
    /// initial resource registry capacity
    int ResourceRegistryCapacity = 256;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurveFormat
    @ingroup Anim
    @brief format of anim keys
*/
struct AnimCurveFormat {
    enum Enum {
        Float,      ///< 1 key, linear interpolation
        Float2,     ///< 2 keys, linear interpolation
        Float3,     ///< 3 keys, linear interpolation
        Float4,     ///< 4 keys, linear interpolation
        Quaternion, ///< 4 keys, spherical interpolation
        Invalid,
    };

    /// return number of floats for a format
    static int Stride(AnimCurveFormat::Enum fmt) {
        switch (fmt) {
            case Float:         return 1;
            case Float2:        return 2;
            case Float3:        return 3;
            case Float4:        return 4;
            case Quaternion:    return 4;
            default:            return 0;
        }
    }
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurveSetup
    @ingroup Anim
    @brief describe a single anim curve in an anim clip
*/
struct AnimCurveSetup {
    /// true if the curve is actually a single value
    bool Static = false;
    /// the default value of the curve
    glm::vec4 StaticValue;
    
    /// default constructor
    AnimCurveSetup() { };
    /// construct from values
    AnimCurveSetup(bool isStatic, float x, float y, float z, float w):
        Static(isStatic), StaticValue(x, y, z, w) { };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClipSetup
    @ingroup Anim
    @brief describe an animation clip, part of AnimLibrarySetup
*/
struct AnimClipSetup {
    /// name of the anim clip (must be unique in library)
    StringAtom Name;
    /// the length of the clip in number of keys
    int Length = 0;
    /// the time duration from one key to next in seconds
    double KeyDuration = 1.0 / 25.0;
    /// a description of each curve in the clip
    Array<AnimCurveSetup> Curves;

    /// default constructor
    AnimClipSetup() { };
    /// construct from values
    AnimClipSetup(const StringAtom& name, int len, float dur, const Array<AnimCurveSetup>& curves):
        Name(name), Length(len), KeyDuration(dur), Curves(curves) { };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimLibrarySetup
    @ingroup Anim
    @brief describe an animation library

    An animation library is a collection of compatible clips (clip 
    with the same anim curve layout).
*/
struct AnimLibrarySetup {
    /// resource locator for sharing
    class Locator Locator = Locator::NonShared();
    /// number and format of curves (must be identical for all clips)
    Array<AnimCurveFormat::Enum> CurveLayout;
    /// the anim clips in the library
    Array<AnimClipSetup> Clips;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimBoneSetup
    @ingroup Anim
    @brief setup params for an animation bone
*/
struct AnimBoneSetup {
    /// the bone's name
    StringAtom Name;
    /// index of parent joint, InvalidIndex if a root joint
    int16_t ParentIndex = InvalidIndex;
    /// the normal (non-inverse) bind pose matrix in model space
    glm::mat4 BindPose;
    /// the inverse bind pose matrix in model space
    glm::mat4 InvBindPose;

    /// default constructor
    AnimBoneSetup() { };
    /// construct from params
    AnimBoneSetup(const StringAtom& name, int parentIndex, const glm::mat4& bindPose, const glm::mat4& invBindPose):
        Name(name), ParentIndex(parentIndex), BindPose(bindPose), InvBindPose(invBindPose) { };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSkeletonSetup
    @ingroup Anim
    @brief setup params for a character skeleton
*/
struct AnimSkeletonSetup {
    /// locator for resource sharing
    class Locator Locator = Locator::NonShared();
    /// the skeleton bones
    Array<AnimBoneSetup> Bones; 
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimInstanceSetup
    @ingroup Anim
    @brief setup params for an animation instance
*/
struct AnimInstanceSetup {
    /// create AnimInstanceSetup from AnimLibrary Id
    static AnimInstanceSetup FromLibrary(Id libId) {
        AnimInstanceSetup setup;
        setup.Library = libId;
        return setup;
    };
    /// create AnimInstanceSetup from AnimLibrary and AnimSkeleton Id
    static AnimInstanceSetup FromLibraryAndSkeleton(Id libId, Id skelId) {
        AnimInstanceSetup setup;
        setup.Library = libId;
        setup.Skeleton = skelId;
        return setup;
    };

    /// the AnimLibrary of this instance
    Id Library;
    /// an optional AnimSkeleton if this is an instance
    Id Skeleton;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurve
    @ingroup Anim
    @brief an animation curve (part of a clip) 
*/
struct AnimCurve {
    /// the curve format
    AnimCurveFormat::Enum Format = AnimCurveFormat::Invalid;
    /// number of values (1, 2, 3 or 4)
    int NumValues = 0;
    /// is the curve static? (no actual keys in key pool)
    bool Static = false;
    /// the static value if the curve has no keys
    float StaticValue[4];
    /// stride in float (according to format)
    int KeyStride = 0;
    /// index of the first key in key pool (relative to clip)
    int KeyIndex = InvalidIndex;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClip
    @ingroup Anim
    @brief an animation clip (part of a library)
*/
struct AnimClip {
    /// name of the clip
    StringAtom Name;
    /// the length of the clip in number of keys
    int Length = 0;
    /// the time duration from one key to next
    double KeyDuration = 1.0f / 25.0f;
    /// the stride in floats from one key of a curve to next in key pool
    int KeyStride = 0;
    /// access to the clip's curves
    Slice<AnimCurve> Curves;
    /// access to the clip's 2D key table
    Slice<float> Keys;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimLibrary
    @ingroup Anim
    @brief a collection of clips with the same curve layout
*/
struct AnimLibrary : public ResourceBase {
    /// resource locator (name + sig)
    class Locator Locator;
    /// stride of per-instance samples in number of floats
    int SampleStride = 0;
    /// access to all clips in the library
    Slice<AnimClip> Clips;
    /// array view over all curves of all clips
    Slice<AnimCurve> Curves;
    /// array view over all keys of all clips
    Slice<float> Keys;
    /// map clip names to clip indices
    Map<StringAtom, int> ClipIndexMap;
    /// the curve layout (all clips in the library have the same layout)
    InlineArray<AnimCurveFormat::Enum, AnimConfig::MaxNumCurvesInClip> CurveLayout;

    /// clear the object
    void clear() {
        Locator = Locator::NonShared();
        SampleStride = 0;
        Clips.Reset();
        Curves.Reset();
        Keys.Reset();
        ClipIndexMap.Clear();
    };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSkeleton
    @ingroup Anim
    @brief runtime struct for an animation skeleton
*/
struct AnimSkeleton : public ResourceBase {
    /// resource locator (name + sig)
    class Locator Locator;
    /// number of bones in the skeleton
    int NumBones = 0;
    /// the bind pose matrices (non-inverse)
    Slice<glm::mat4x3> BindPose;
    /// the inverse bind pose matrices
    Slice<glm::mat4x3> InvBindPose;
    /// this is the range of all matrices (BindPose and InvBindPose)
    Slice<glm::mat4x3> Matrices;
    /// the parent bone indices (-1 if a root bone)
    StaticArray<int32_t, AnimConfig::MaxNumSkeletonBones> ParentIndices;

    /// clear the object
    void clear() {
        Locator = Locator::NonShared();
        NumBones = 0;
        BindPose.Reset();
        InvBindPose.Reset();
        Matrices.Reset();
    };
};

//------------------------------------------------------------------------------
/**
    @typedef Oryol::AnimJobId
    @ingroup Anim
    @brief identifies a playing anim job
*/
typedef uint32_t AnimJobId;
static const AnimJobId InvalidAnimJobId = 0xFFFFFFFF;

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimJob
    @ingroup Anim
    @brief describe play parameters for an animation
*/
struct AnimJob {
    /// index of anim clip to play
    int ClipIndex = 0;
    /// the track index for priority blending (higher tracks have higher priority)
    int TrackIndex = 0;
    /// overall weight when mixing with lower-priority track
    float MixWeight = 1.0f; 
    /// start time relative to 'now' in seconds
    float StartTime = 0.0f;
    /// playback duration or loop count (<= 0.0f is infinite)
    float Duration = 0.0f;
    /// true if Duration is loop count, false if Duration is seconds
    bool DurationIsLoopCount = false;
    /// fade-in duration in seconds
    float FadeIn = 0.0f;
    /// fade-out duration in seconds
    float FadeOut = 0.0f;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSkinMatrixInfo
    @ingroup Anim
    @brief the evaluated skinning matrix data
    
    This contains the evaluated skin-matrix information for all
    active AnimInstances in the current frame. The per-instance
    items are in the same order how active AnimInstances had
    been added, but only contains AnimInstances with skeletons.
*/
struct AnimSkinMatrixInfo {
    /// pointer to the skin-matrix table
    const float* SkinMatrixTable = nullptr;
    /// size of valid information in the skin matrix table in bytes
    int SkinMatrixTableByteSize = 0;
    /// per-instance information
    struct InstanceInfo {
        Id Instance;
        glm::vec4 ShaderInfo;   // x: u texcoord, y: v texcoord, z: 1.0/texwidth
    };
    /// one entry per active anim instance
    Array<InstanceInfo> InstanceInfos;
};

} // namespace Oryol
