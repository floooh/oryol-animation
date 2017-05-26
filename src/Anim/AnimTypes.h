#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/String/StringAtom.h"
#include "Core/Containers/StaticArray.h"
#include "Resource/ResourceBase.h"
#include <functional>

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSetup
    @ingroup Anim
    @brief setup parameters for Anim module
*/
class AnimSetup {
public:
    /// max number of anim clips
    int MaxNumClips = 64;
    /// max number of anim curves over all clips
    int MaxNumCurves = MaxNumClips * 256;
    /// max number of float keys in key pool
    int MaxNumKeys = MaxNumCurves * 64;     // 64*64*256*sizeof(float) = 4 MByte
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
class AnimCurveFormat {
public:
    enum Enum {
        Static,     ///< a 'flat' curve, no keys, only a static value
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
            case Static:        return 0;
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
    @class Oryol::AnimCurve
    @ingroup Anim
    @brief describes a curve in a clip
*/
class AnimCurve {
public:
    /// the curve format
    AnimCurveFormat::Enum Format = AnimCurveFormat::Invalid;
    /// the static value if the curve has no keys
    StaticArray<float, 4> StaticValue;

    /// internal: index of first key in key pool (rel to clip)
    int keyIndex = InvalidIndex;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClipSetup
    @ingroup Anim
    @brief setup parameters for animation clips
*/
class AnimClip;
class AnimClipSetup {
public:
    /// the clip name
    StringAtom Name;
    /// overall number of curves in the clip
    int NumCurves = 0;
    /// number of keys per curve
    int NumKeys = 0;
    /// key duration in seconds (all keys in clip must have same duration)
    float KeyDuration = 1.0f / 25.0f;
    /// function will be called for each curve to setup AnimCurve struct
    std::function<void(const AnimClip& clip, int curveIndex, AnimCurve& curve)> InitCurve;
    /// function will be called once to fill the clip's key value table
    std::function<void(const AnimClip& clip, const ArrayView<AnimCurve>& curves, ArrayView<float>& keys)> InitKeys;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClip
    @ingroup Anim
    @brief describes an animation clip
*/
class AnimClip : public ResourceBase {
public:
    /// name of the clip
    StringAtom Name;
    /// number of curves in the clip
    int NumCurves = 0;
    /// number of keys per curve
    int NumKeys = 0;
    /// key duration in seconds (default is 1/25)
    float KeyDuration = 1.0f / 25.0f;
    /// stride in number of floats in key pool
    int KeyStride = 0;

    /// internal: index of first curve in curve pool
    int curveIndex = InvalidIndex;
    /// internal: index of first key in clip pool
    int keyIndex = InvalidIndex;
    /// internal: number of keys in key pool
    int numPoolKeys = 0;
    /// internal: reset to initial state
    void clear() {
        Name.Clear();
        NumCurves = 0;
        NumKeys = 0;
        KeyDuration = 1.0f / 25.0f;
        KeyStride = 0;
        curveIndex = InvalidIndex;
        keyIndex = InvalidIndex;
        numPoolKeys = 0;
    };
};

} // namespace Oryol
