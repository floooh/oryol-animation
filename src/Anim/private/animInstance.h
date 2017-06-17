#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::animInstance
    @ingroup _priv
    @brief internal animInstance class
*/
#include "Resource/ResourceBase.h"
#include "Anim/private/animSequencer.h"
#include "Core/Containers/Slice.h"

namespace Oryol {

struct AnimLibrary;
struct AnimSkeleton;

namespace _priv {

class animInstance : public ResourceBase {
public:
    /// the shared animation library
    AnimLibrary* library = nullptr;
    /// the shared skeleton (optional)
    AnimSkeleton* skeleton = nullptr;
    /// anim sequencer to keep track to active anim jobs
    animSequencer sequencer;
    /// anim evaluation result (only valid for active instances) 
    Slice<float> samples;
    /// skeleton evaluation result as 4x3 transposed matrices (only valid for active instances)
    Slice<float> skinMatrices;

    /// clear the object
    void clear() {
        library = nullptr;
        skeleton = nullptr;
        samples = Slice<float>();
        skinMatrices = Slice<float>();
    }
};

} // namespace _priv
} // namespace Oryol
