#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/Containers/Map.h"
#include "Core/String/StringAtom.h"
#include "glm/vec4.h"

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimKeyFormat
    @ingroup Anim
    @brief animation key format
*/
class AnimKeyFormat {
public:
    /// animation curve format enum
    enum Code {
        Float,          ///< single component float
        Float2,         ///< 2-component float
        Float3,         ///< 3-component float
        Float4,         ///< 4-component float
        Byte4N,         ///< signed bytes (-1.0..+1.0), mapped to (-128..127)
        Short2N,        ///< 2 signed shorts (-1.0..+1.0), mapped to (-32768..32767)
        Short4N,        ///< 4 signed shorts (-1.0..+1.0), mapped to (-32768..32767)

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
class AnimIpolType {
public:
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
class AnimWrapMode {
public:
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
class AnimSampleMode {
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
class AnimCurve {
public:
    /// the interpolation type (constant vs linear vs spherical)
    AnimIpolType::Code IpolType = AnimIpolType::Invalid;
    /// format of animation keys
    AnimKeyFormat::Code Format = AnimKeyFormat::Invalid;
    /// constant key value (if AnimIpolType is Constant)
    glm::vec4 Constant;
    /// byte-offset of first key in key buffer
    int KeyOffset = InvalidIndex;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimClip
    @ingroup Anim
    @brief group of related AnimCurves
*/
class AnimClip {
public:
    /// index of first AnimCurve in global anim curve pool
    int StartCurveIndex = InvalidIndex;
    /// number of AnimCurves in clip
    int NumCurves = 0;
    /// number of keys (same for all anim curves)
    int NumKeys = 0;
    /// byte-stride between keys
    int KeyStride = 0;
};

//------------------------------------------------------------------------------
/**
    @class AnimLibrary
    @ingroup Anim
    @brief group of related AnimClips
*/
class AnimLibrary {
    /// index of first clip in global anim clip pool
    int StartClipIndex = InvalidIndex;
    /// number of clips in anim pool
    int NumClips = 0;
    /// map clip names to clip indices (relative to StartClipIndex)
    Map<StringAtom, int> ClipNameMap;
};

} // namespace Oryol
