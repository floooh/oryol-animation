#pragma once
//------------------------------------------------------------------------------
#include "Core/Types.h"
#include "Core/String/StringAtom.h"
#include "Core/Containers/Array.h"

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSetup
    @ingroup Anim
    @brief setup parameters for Anim module
*/
class AnimSetup {
public:
    /// the global key buffer size, in bytes
    int KeyBufferSize = 2 * 1024 * 1024;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimKeyFormat
    @ingroup Anim
    @brief format of anim keys
*/
class AnimKeyFormat {
public:
    enum Code {
        Float,      ///< generic scalar
        Float2,     ///< genecic 2D vector
        Float3,     ///< generic 3D vector
        Float4,     ///< generic 4D vector
        Quaternion  ///< a rotation key

        InvalidAnimKeyFormat = 0xFFFFFFFF;
    };
};

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimCurveSetup
    @ingroup Anim
    @brief setup parameters for an animation curve (part of AnimClipSetup)
*/
class AnimCurveSetup {
public:
    /// default constructor
    AnimCurveSetup();
    /// construct from values
    AnimCurveSetup(AnimKeyFormat::Code fmt, bool constant=false, float x=0.0f, float y=0.0f, float z=0.0f, float w=0.0f):
        Format(fmt),
        Constant(constant),
        Value[0](x), Value[1](y), Value[2](z), Value[3](w)
    { };

    /// format of anim keys in curve
    AnimKeyFormat::Code Format = AnimKeyFormat::InvalidAnimKeyFormat;
    /// true if the curve is constant
    bool Constant = false;
    /// optional constant- or default-value of curve
    StaticArray<float, 4> Value;
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
    /// number of keys in a curve
    int NumKeys = 0;
    /// animation curves description
    Array<AnimCurveSetup> Curves;
};

} // namespace Oryol
