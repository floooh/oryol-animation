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
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClipSetup
    @ingroup Anim
    @brief setup parameters for animation clips
*/
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
    /// callback to setup an anim curve in clip
    std::function<void(int curveIndex, AnimCurve& curve)> InitCurve;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurveFormat
    @ingroup Anim
    @brief format of anim keys
*/
class AnimCurveFormat {
public:
    enum Code {
        Static,     ///< a 'flat' curve, no keys, only a static value
        Float,      ///< 1 key, linear interpolation
        Float2,     ///< 2 keys, linear interpolation
        Float3,     ///< 3 keys, linear interpolation
        Float4,     ///< 4 keys, linear interpolation
        Quaternion  ///< 4 keys, spherical interpolation

        Invalid,
    };
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
    AnimCurveFormat::Code Format = AnimCurveFormat::Invalid;
    /// the static value if the curve has no keys
    StaticArray<float, 4> StaticValue;

    /// internal: index of first key in key pool (rel to clip)
    int keyIndex = InvalidIndex;
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

    /// internal: index of first curve in curve pool
    int curveIndex = InvalidIndex;
    /// internal: index of first key in clip pool
    int keyIndex = InvalidIndex;
    /// internal: stride between keys of same curve
    int keyStride = 0;
}

} // namespace Oryol
