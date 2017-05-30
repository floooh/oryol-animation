#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::animTracker
    @ingroup _priv
    @brief a 'track sequencer' for animation priority blending
*/
#include "Anim/AnimTypes.h"
#include "Resource/Id.h"
#include "Core/Containers/StaticArray.h"

namespace Oryol {
namespace _priv {

class animTracker {
public:
    /// a track item for evaluating an anim job
    struct item {
        /// the animation instance id
        Id instance;
        /// a 'unique id' for the item
        AnimJob::Id itemId = AnimJob::InvalidId;
        /// the anim clip index
        int clipIndex = InvalidIndex;
        /// the track index (lower number means higher priority)
        int trackIndex = 0;
        /// overall mixing weight
        float mixWeight = 1.0f;
        /// the absolute start time (including fade-in)
        double absStartTime = 0.0;
        /// the absolute end time (including fade-out)
        double absEndTime = 0.0;
        /// the absolute time when fade-in is ends
        double absFadeInTime = 0.0;
        /// the absolute time when fade-out starts
        double absFadeOutTime = 0.0;
    };
    /// max number of items that can be queued
    static const int maxItems = 16;
    /// currently enqueued items
    int numItems = 0;
    /// room for enqueued items
    StaticArray<item, maxItems> items;

    /// enqueue a new anim job
    void enqueue(double curTime, uint32_t itemId, const AnimJob& job);
    /// remove an anim job
    void remove(const Id& id);
};

} // namespace _priv
} // namespace Oryol
