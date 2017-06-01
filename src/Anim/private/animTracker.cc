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
    if (this->items.Full()) {
        // no more free job slots
        return false;
    }

    // find insertion position
    double absStartTime = curTime + job.StartTime;
    int insertIndex;
    int numItems = this->items.Size();
    for (insertIndex = 0; insertIndex < numItems; insertIndex++) {
        const auto& curItem = this->items[insertIndex];
        if (!curItem.valid) {
            // skip invalid items
            continue;
        }
        else if (job.TrackIndex > curItem.trackIndex) {
            // skip items on smaller track indices
            continue;
        }
        else if ((job.TrackIndex==curItem.trackIndex) && (absStartTime>curItem.absStartTime)) {
            // skip items on same track with smaller start time
            continue;
        }
        else {
            // current item has higher track index, or is on same track 
            // with higher or equal start time, insert the new job right before 
            break;
        }
    }
    item newItem;
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
    this->items.Insert(insertIndex, newItem);

    // fix the start and end times for all jobs on the same
    // track that overlap with the new job, this may lead to
    // some jobs becoming infinitly short, these will no longer
    // take part in mixing, and will eventually be removed
    // when the play cursor passes over them
    numItems = this->items.Size();
    for (int i = 0; i < numItems; i++) {
        if (i == insertIndex) {
            // this is the item that was just inserted
            continue;
        }
        else {
            auto& curItem = this->items[i];
            if (curItem.valid && (newItem.trackIndex == curItem.trackIndex)) {
                // if before the new item, check overlap and trim the end if yes
                if ((i < insertIndex) && (curItem.absEndTime >= newItem.absFadeInTime)) {
                    curItem.absEndTime = newItem.absFadeInTime;
                    curItem.absFadeOutTime = newItem.absStartTime;
                }
                // if after the new item, check overlap and trim start if yes
                if ((i > insertIndex) && (curItem.absStartTime <= newItem.absFadeOutTime)) {
                    curItem.absStartTime = newItem.absFadeOutTime;
                    curItem.absFadeInTime = newItem.absEndTime;
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
