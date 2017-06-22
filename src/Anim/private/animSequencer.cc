//------------------------------------------------------------------------------
//  animSequencer.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animSequencer.h"
#include <float.h>
#include <math.h>

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
animSequencer::add(double curTime, AnimJobId jobId, const AnimJob& job, double clipDuration) {
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
static float fadeWeight(float w0, float w1, double t, double t0, double t1) {
    // compute a fade-in or fade-out mixing weight
    double dt = t1 - t0;
    if ((dt > -0.000001) && (dt < 0.000001)) {
        return w0;  // just make sure we don't divide by zero
    } 
    float rt = (float) ((t - t0) / (t1 - t0));
    if (rt < 0.0f) rt = 0.0f;
    else if (rt > 1.0f) rt = 1.0f;
    return w0 + rt*(w1-w0);
}

//------------------------------------------------------------------------------
static int clampKeyIndex(int keyIndex, int clipNumKeys) {
    // FIXME: handle clamp vs loop here
    o_assert_dbg(clipNumKeys > 0);
    keyIndex %= clipNumKeys;
    if (keyIndex < 0) {
        keyIndex += clipNumKeys;
    }
    return keyIndex;
}

//------------------------------------------------------------------------------
bool
animSequencer::eval(const AnimLibrary* lib, double curTime, float* sampleBuffer, int numSamples) {

    // for each item which crosses the current play time...
    // FIXME: currently items are evaluated even if they are culled
    // completely by higher priority items
    int numProcessedItems = 0;
    for (const auto& item : this->items) {
        // skip current item if it isn't valid or doesn't cross the play cursor
        if (!item.valid || (item.absStartTime > curTime) || (item.absEndTime <= curTime)) {
            continue;
        }
        const AnimClip& clip = lib->Clips[item.clipIndex];

        // compute sampling parameters
        int key0 = 0;
        int key1 = 0;
        float keyPos = 0.0f;
        if (clip.Length > 0) {
            o_assert_dbg(clip.KeyDuration > 0.0f);
            const double clipTime = curTime - item.absStartTime;
            key0 = int(clipTime / clip.KeyDuration);
            keyPos = (clipTime - (key0 * clip.KeyDuration)) / clip.KeyDuration;
            key0 = clampKeyIndex(key0, clip.Length);
            key1 = clampKeyIndex(key0 + 1, clip.Length);
        }

        // only sample, or sample and mix with previous track?
        const float* src0 = clip.Keys.Empty() ? nullptr : &(clip.Keys[key0 * clip.KeyStride]);
        const float* src1 = clip.Keys.Empty() ? nullptr : &(clip.Keys[key1 * clip.KeyStride]);
        float* dst = sampleBuffer;
        #if ORYOL_DEBUG
        const float* dstEnd = dst + numSamples;
        #endif
        float s0, s1, v0, v1;
        if (0 == numProcessedItems) {
            // first processed track, only need to sample, not mix with previous track
            for (const auto& curve : clip.Curves) {
                const int num = curve.NumValues;
                if (curve.Static) {
                    if (num >= 1) *dst++ = curve.StaticValue[0];
                    if (num >= 2) *dst++ = curve.StaticValue[1];
                    if (num >= 3) *dst++ = curve.StaticValue[2];
                    if (num >= 4) *dst++ = curve.StaticValue[3];
                }
                else {
                    // NOTE: simply use linear interpolation for quaternions,
                    // just assume they are close together
                    o_assert_dbg(src0 && src1);
                    if (num >= 1) { v0=*src0++; v1=*src1++; *dst++=v0+(v1-v0)*keyPos; }
                    if (num >= 2) { v0=*src0++; v1=*src1++; *dst++=v0+(v1-v0)*keyPos; }
                    if (num >= 3) { v0=*src0++; v1=*src1++; *dst++=v0+(v1-v0)*keyPos; }
                    if (num >= 4) { v0=*src0++; v1=*src1++; *dst++=v0+(v1-v0)*keyPos; }
                }
            }
        }
        else {
            // evaluate track and mix with previous sampling+mixing result
            // FIXME: may need to do proper quaternion slerp when mixing
            // rotation curves
            float weight = item.mixWeight;
            if (curTime < item.absFadeInTime) {
                weight = fadeWeight(0.0f, weight, curTime, item.absStartTime, item.absFadeInTime);
            }
            else if (curTime > item.absFadeOutTime) {
                weight = fadeWeight(weight, 0.0f, curTime, item.absFadeOutTime, item.absEndTime);
            }
            for (const auto& curve : clip.Curves) {
                const int num = curve.NumValues;
                if (curve.Static) {
                    if (num >= 1) { s0=*dst; s1=curve.StaticValue[0]; *dst++=s0+(s1-s0)*weight; }
                    if (num >= 2) { s0=*dst; s1=curve.StaticValue[1]; *dst++=s0+(s1-s0)*weight; }
                    if (num >= 3) { s0=*dst; s1=curve.StaticValue[2]; *dst++=s0+(s1-s0)*weight; }
                    if (num >= 4) { s0=*dst; s1=curve.StaticValue[2]; *dst++=s0+(s1-s0)*weight; }
                }
                else {
                    o_assert_dbg(src0 && src1);
                    if (num >= 1) {
                        v0=*src0++; v1=*src1++;
                        s0=*dst; s1=v0+(v1-v0)*keyPos;
                        *dst++=s0+(s1-s0)*weight;
                    }
                    if (num >= 2) {
                        v0=*src0++; v1=*src1++;
                        s0=*dst; s1=v0+(v1-v0)*keyPos;
                        *dst++=s0+(s1-s0)*weight;
                    }
                    if (num >= 3) {
                        v0=*src0++; v1=*src1++;
                        s0=*dst; s1=v0+(v1-v0)*keyPos;
                        *dst++=s0+(s1-s0)*weight;
                    }
                    if (num >= 4) {
                        v0=*src0++; v1=*src1++;
                        s0=*dst; s1=v0+(v1-v0)*keyPos;
                        *dst++=s0+(s1-s0)*weight;
                    }
                }
            }
        }
        o_assert_dbg(dst == dstEnd);
        #if ORYOL_DEBUG
        if (src0 && src1) {
            o_assert_dbg(src0 == (&(clip.Keys[key0 * clip.KeyStride]) + clip.KeyStride));
            o_assert_dbg(src1 == (&(clip.Keys[key1 * clip.KeyStride]) + clip.KeyStride));
        }
        #endif
        numProcessedItems++;
    }
    return numProcessedItems > 0;
}

} // namespace _priv
} // namespace Oryol
