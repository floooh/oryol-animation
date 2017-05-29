#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/String/StringAtom.h"
#include "Core/Containers/Slice.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/Map.h"
#include "Resource/ResourceBase.h"

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSetup
    @ingroup Anim
    @brief setup parameters for Anim module
*/
struct AnimSetup {
    /// max number of anim libraries
    int MaxNumLibs = 16;
    /// max overall number of anim clips
    int MaxNumClips = MaxNumLibs * 64;
    /// max overall number of anim curves
    int MaxNumCurves = MaxNumClips * 256;
    /// max number of animation instances
    int MaxNumInstances = MaxNumLibs * 128;
    /// max number of float keys
    int MaxNumKeys = 4 * 1024 * 1024; 
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
            case Invalid:       return 0;
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
    StaticArray<float, 4> StaticValue;
    
    /// default constructor
    AnimCurveSetup() { };
    /// construct from values
    AnimCurveSetup(bool isStatic, float x=0.0f, float y=0.0f, float z=0.0f, float w=0.0f) {
        Static = isStatic;
        StaticValue[0] = x; StaticValue[1] = y; StaticValue[2] = z; StaticValue[3] = w;
    };
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
    Slice<AnimCurveSetup> Curves;

    /// default constructor
    AnimClipSetup() { };
    /// construct from values
    AnimClipSetup(const StringAtom& name, int len, float dur, const Slice<AnimCurveSetup>& curves):
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
    Slice<AnimCurveFormat::Enum> CurveLayout;
    /// the anim clips in the library
    Slice<AnimClipSetup> Clips;
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
    StaticArray<float, 4> StaticValue;
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
    @class Oryol::AnimInstanceSetup
    @ingroup Anim
    @brief setup params for an animation instance
*/
struct AnimInstanceSetup {
    // FIXME
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimInstance
    @ingroup Anim
    @brief an animation instance 
*/
struct AnimInstance {
    // FIXME
};

} // namespace Oryol
