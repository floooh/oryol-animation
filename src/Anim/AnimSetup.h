#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSystemSetup
    @brief setup parameters for the AnimSystem
*/
#include "Core/Types.h"

namespace Oryol {

class AnimSetup {
public:
    /// size of global anim key buffer (in bytes)
    int AnimKeyBufferSize = 5 * 1024 * 1024;
    /// max number of AnimPool objects
    int MaxAnimPools = 16;
    /// max number of AnimClip objects
    int MaxAnimClips = MaxAnimPools * 16;
    /// max number of AnimCurve objects
    int MaxAnimCurves = MaxAnimClips * 128;
};

} // namespace Oryol


