//------------------------------------------------------------------------------
//  animTracker.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animTracker.h"
#include <float.h>

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
bool
animTracker::add(double curTime, AnimJobId jobId, const AnimJob& job, float clipDuration) {
    if (this->numItems == maxItems) {
        // no more free job slots
        return false;
    }

    // find insertion position
    double absStartTime = curTime + job.StartTime;
    int insertIndex;
    for (insertIndex = 0; insertIndex < this->numItems; insertIndex++) {
        const auto& curItem = this->items[insertIndex];
        if (!curItem.valid) {
            continue;
        }
        else if (job.TrackIndex < curItem.trackIndex) {
            break;
        }
        else if ((job.TrackIndex==curItem.trackIndex) && (absStartTime>curItem.absStartTime)) {
            break;
        }
    }
    if (insertIndex < this->numItems) {
        // make room for the new item 
        for (int i = this->numItems; i > insertIndex; i--) {
            this->items[i] = this->items[i-1];
        }
    }
    this->numItems++;
    o_assert_dbg(this->numItems <= maxItems);

    // insert new anim job item
    item& newItem = this->items[insertIndex];
    newItem.id = jobId;
    newItem.valid = true;
    newItem.clipIndex = job.ClipIndex;
    newItem.trackIndex = job.TrackIndex;
    newItem.mixWeight = job.MixWeight;
    newItem.absStartTime = absStartTime;
    newItem.absFadeInTime = absStartTime + job.FadeIn;
    if (job.Duration > 0.0f) {
        if (job.DurationIsLoopCount) {
            newItem.absEndTime = absStartTime + job.Duration*clipDuration;
        }
        else {
            newItem.absEndTime = absStartTime + job.Duration;
        }
        newItem.absFadeOutTime = newItem.absEndTime - job.FadeOut;
    }
    else {
        // infinite duration
        newItem.absEndTime = DBL_MAX;
        newItem.absFadeOutTime = DBL_MAX;
    }

    // fix the start and end times for all jobs on the same
    // track that overlap with the new job, this may lead to
    // some jobs becoming infinitly short, these will no longer
    // take part in mixing, and will eventually be removed
    // when the play cursor passes over them
    for (int i = 0; i < this->numItems; i++) {
        if (i == insertIndex) {
            // this is the item that was just inserted
            continue;
        }
        else {
            auto& curItem = this->items[i];
            if (newItem.trackIndex == curItem.trackIndex) {
                // clamp against end of new item
                if (curItem.absStartTime <= newItem.absFadeOutTime) {
                    double dur = curItem.absFadeInTime - curItem.absStartTime;
                    curItem.absStartTime = newItem.absFadeOutTime;
                    curItem.absFadeInTime = curItem.absStartTime + dur; 
                }
                // clamp against start of new item
                if (curItem.absEndTime >= newItem.absFadeInTime) {
                    double dur = curItem.absEndTime - curItem.absFadeOutTime;
                    curItem.absEndTime = newItem.absFadeInTime;
                    curItem.absFadeOutTime = curItem.absEndTime - dur; 
                }
                // if both sides were clipped, it means the current item
                // is completely 'obscured' and needs to be removed
                // during the next 'garbage collection' 
                if (curItem.absStartTime >= curItem.absEndTime) {
                    curItem.valid = false;
                    curItem.absStartTime = 0.0;
                    curItem.absEndTime = 0.0;
                }
            }
        } 
    }
    return true;
}

//------------------------------------------------------------------------------
void
animTracker::garbageCollect(double curTime) {

}

//------------------------------------------------------------------------------
void
animTracker::eval(double curTime, float* sampleBuffer, int numSamples) {

}

} // namespace _priv
} // namespace Oryol
