#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/Containers/Map.h"
#include "Core/String/StringAtom.h"
#include "Resource/Id.h"
#include "glm/vec4.hpp"

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimKeyFormat
    @ingroup Anim
    @brief animation key format
*/
struct AnimKeyFormat {
    /// animation curve format enum
    enum Code {
        Float4,         ///< 4-component float
        Float3,         ///< 3-component float
        Float2,         ///< 2-component float
        Float,          ///< single component float
        Byte4N,         ///< signed bytes (-1.0..+1.0), mapped to (-128..127)
        Short4N,        ///< 4 signed shorts (-1.0..+1.0), mapped to (-32768..32767)
        Short2N,        ///< 2 signed shorts (-1.0..+1.0), mapped to (-32768..32767)

        Num,
        Invalid,
    };

    /// get byte size of an anim key
    static int ByteSize(Code c) {
        switch (c) {
            case Float:     return 4;
            case Float2:    return 8;
            case Float3:    return 12;
            case Float4:    return 16;
            case Byte4N:    return 4;
            case Short2N:   return 4;
            case Short4N:   return 8;
            default:
                o_error("AnimKeyFormat::ByteSize(): invalid value!\n");
                return 0;
        }
    }
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimIpolType
    @ingroup Anim
    @brief how to perform interpolation between keys
*/
struct AnimIpolType {
    /// animation interpolation type
    enum Code {
        Constant,       ///< whole curve is one constant value, no interpolation
        Linear,         ///< linear vector data
        Spherical,      ///< keys are quaternions, use spherical interpolation

        Invalid,
    };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimWrapMode
    @ingroup Anim
    @brief define anim curve behaviour when samples outside key range
*/
struct AnimWrapMode {
    /// animation sampling wrap mode
    enum Code {
        Clamp,      ///< clamp animation to first/last key
        Loop,       ///< loop animation

        Invalid,
    };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSampleMode
    @ingroup Anim
    @brief how the animation curve is sampled
*/
struct AnimSampleMode {
    /// animation sample mode
    enum Code {
        Step,       ///< hard steps, no inbetween values
        Linear,     ///< linear interpolation for inbetween values
    };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurve
    @ingroup Anim
    @brief an AnimCurve describes an array of keys along the time-line
*/
struct AnimCurve {
    /// the interpolation type (constant vs linear vs spherical)
    AnimIpolType::Code IpolType = AnimIpolType::Invalid;
    /// format of animation keys
    AnimKeyFormat::Code Format = AnimKeyFormat::Invalid;
    /// constant key value (if AnimIpolType is Constant)
    float Constant[4] = { };
    /// byte-offset of first key in key buffer
    int KeyOffset = InvalidIndex;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClip
    @ingroup Anim
    @brief group of related AnimCurves
*/
struct AnimClip {
    /// index of first AnimCurve in global anim curve pool
    int StartCurveIndex = InvalidIndex;
    /// number of AnimCurves in clip
    int NumCurves = 0;
    /// number of keys (same for all anim curves)
    int NumKeys = 0;
    /// byte-stride between keys
    int KeyStride = 0;
    /// the key duration in 'anim ticks'
    int KeyDuration = 0;
};

//------------------------------------------------------------------------------
/**
    @class AnimLibrary
    @ingroup Anim
    @brief group of related AnimClips
*/
struct AnimLibrary {
    /// index of first clip in global anim clip pool
    int StartClipIndex = InvalidIndex;
    /// number of clips in anim pool
    int NumClips = 0;
    /// index of first curve in global anim curve pool
    int StartCurveIndex = InvalidIndex;
    /// overall number of anim curves in the lib
    int NumCurves = 0;
    /// byte-offset of first anim key in global key buffer
    int StartKeyOffset = InvalidIndex;
    /// byte-size of key range in global key buffer
    int KeyRangeSize = 0;
    /// map clip names to clip indices (relative to StartClipIndex)
    Map<StringAtom, int> ClipNameMap;
    /// resource id of AnimLibrary, written when lib is added to AnimSystem
    Id id;
    /// name of the AnimLibrary, written when lib is added to AnimSystem
    StringAtom name;
};

} // namespace Oryol
