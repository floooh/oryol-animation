#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/String/StringAtom.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/Map.h"
#include "Resource/ResourceBase.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

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
    /// max number of anim instances
    int MaxNumInstances = 128;
    /// max overall number of anim clips
    int ClipPoolCapacity = MaxNumLibs * 64;
    /// max overall number of anim curves
    int CurvePoolCapacity = ClipPoolCapacity * 256;
    /// max number of float keys
    int KeyPoolCapacity = 4 * 1024 * 1024;
    /// max number of the skeleton matrix pool (2 matrices per bone)
    int MatrixPoolCapacity = 1024;
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
    @brief describe an animation clip, part of AnimLibSetup
*/
struct AnimClipSetup {
    /// name of the anim clip (must be unique in library)
    StringAtom Name;
    /// the length of the clip in number of keys
    int Length = 0;
    /// the time duration from one key to next in seconds
    float KeyDuration = 1.0f / 25.0f;
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
    /// the name of the anim library
    StringAtom Name;
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
    /// a name for the skeleton
    StringAtom Name;
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
    static AnimInstanceSetup FromLibrary(Id libId);

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
    /// stride in float (according to format)
    int Stride = 0;
    /// is the curve static? (no actual keys in key pool)
    bool Static = false;
    /// the static value if the curve has no keys
    glm::vec4 StaticValue;
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
    float KeyDuration = 1.0f / 25.0f;
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
    /// name of the library
    StringAtom Name;
    /// stride of per-instance samples samples in number of floats
    int SampleStride = 0;
    /// access to all clips in the library
    Slice<AnimClip> Clips;
    /// array view over all curves of all clips
    Slice<AnimCurve> Curves;
    /// array view over all keys of all clips
    Slice<float> Keys;
    /// map clip names to clip indices
    Map<StringAtom, int> ClipIndexMap;

    /// clear the object
    void clear() {
        Name.Clear();
        SampleStride = 0;
        Clips = Slice<AnimClip>();
        Curves = Slice<AnimCurve>();
        Keys = Slice<float>();
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
    /// name of the skeleton
    StringAtom Name;
    /// number of bones in the skeleton
    int NumBones = 0;
    /// the bind pose matrices (non-inverse)
    Slice<glm::mat4> BindPose;
    /// the inverse bind pose matrices
    Slice<glm::mat4> InvBindPose;
    /// this is the range of all matrices (BindPose and InvBindPose)
    Slice<glm::mat4> Matrices;
    /// the parent bone indices (-1 if a root bone)
    StaticArray<int16_t, AnimConfig::MaxNumSkeletonBones> ParentIndices;

    /// clear the object
    void clear() {
        Name.Clear();
        NumBones = 0;
        BindPose = Slice<glm::mat4>();
        InvBindPose = Slice<glm::mat4>();
        Matrices = Slice<glm::mat4>();
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
    int ClipIndex = InvalidIndex;
    /// the track index for priority blending (lower tracks have higher priority)
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

} // namespace Oryol
