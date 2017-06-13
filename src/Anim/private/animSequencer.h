#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::animSequencer
    @ingroup _priv
    @brief a 'track sequencer' for animation priority blending
*/
#include "Anim/AnimTypes.h"
#include "Resource/Id.h"
#include "Core/Containers/InlineArray.h"

namespace Oryol {
namespace _priv {

class animSequencer {
public:
    /// a track item for evaluating an anim job
    struct item {
        /// this item's id
        AnimJobId id = 0;
        /// a valid flag, invalid items need to be removed during 'gc'
        bool valid = true;
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
    /// room for enqueued items
    InlineArray<item, maxItems> items;

    /// enqueue a new anim job, return false if queue is full, or job was dropped
    bool add(double curTime, AnimJobId jobId, const AnimJob& job, float clipDuration);
    /// stop a job, this will just set the end time to the current time
    void stop(double curTime, AnimJobId jobId, bool allowFadeOut);
    /// stop a track, this will set the end time of jobs overlapping curTime, and invalidate future jobs
    void stopTrack(double curTime, int trackIndex, bool allowFadeOut);
    /// stop all jobs
    void stopAll(double curTime, bool allowFadeOut);
    /// remove invalid and expired items
    void garbageCollect(double curTime);
    /// evaluate (sample and mix) the animation into sampleBuffer
    void eval(double curTime, float* sampleBuffer, int numSamples);
};

} // namespace _priv
} // namespace Oryol
