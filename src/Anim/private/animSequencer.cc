//------------------------------------------------------------------------------
//  animSequencer.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animSequencer.h"
#include <float.h>

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
static void
checkValidateItem(animSequencer::item& item) {
    if (item.absStartTime >= item.absEndTime) {
        item.valid = false;
    }
}

//------------------------------------------------------------------------------
bool
animSequencer::add(double curTime, AnimJobId jobId, const AnimJob& job, float clipDuration) {
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
                checkValidateItem(curItem);
            }
        } 
    }
    return true;
}

//------------------------------------------------------------------------------
static void
checkStopItem(double curTime, bool allowFadeOut, animSequencer::item& item) {
    if (curTime < item.absStartTime) {
        // the item is in the future, can mark it as invalid
        item.valid = false;
    }
    else if (curTime < item.absEndTime) {
        // the item overlaps curTime, clamp the end time 
        if (allowFadeOut) {
            double fadeDuration = item.absEndTime - item.absFadeOutTime;
            item.absFadeOutTime = curTime;
            item.absEndTime = curTime + fadeDuration;
        }
        else {
            item.absFadeOutTime = curTime;
            item.absEndTime = curTime;
        }
        checkValidateItem(item);
    }
}

//------------------------------------------------------------------------------
void
animSequencer::stop(double curTime, AnimJobId jobId, bool allowFadeOut) {
    for (auto& curItem : this->items) {
        if (curItem.id == jobId) {
            checkStopItem(curTime, allowFadeOut, curItem);
            break;
        }
    }
}

//------------------------------------------------------------------------------
void
animSequencer::stopTrack(double curTime, int trackIndex, bool allowFadeOut) {
    for (auto& curItem : this->items) {
        if (curItem.trackIndex == trackIndex) {
            checkStopItem(curTime, allowFadeOut, curItem);
        }
    }
}

//------------------------------------------------------------------------------
void
animSequencer::stopAll(double curTime, bool allowFadeOut) {
    for (auto& curItem : this->items) {
        checkStopItem(curTime, allowFadeOut, curItem);
    }
}

//------------------------------------------------------------------------------
void
animSequencer::garbageCollect(double curTime) {
    // remove all invalid items, and items where absEndTime is < curTime
    for (int i = this->items.Size() - 1; i >= 0; i--) {
        const auto& item = this->items[i];
        if (!item.valid || (item.absEndTime < curTime)) {
            this->items.Erase(i);
        }
    }
}

//------------------------------------------------------------------------------
void
animSequencer::eval(double curTime, float* sampleBuffer, int numSamples) {

}

} // namespace _priv
} // namespace Oryol
