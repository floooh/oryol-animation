//------------------------------------------------------------------------------
//  AnimSystem.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "AnimSystem.h"

namespace Oryol {

//------------------------------------------------------------------------------
AnimSystem::~AnimSystem() {
    if (this->isValid) {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
void
AnimSystem::Setup(const AnimSystemSetup& setup_) {
    o_assert_dbg(!this->isValid);
    this->setup = setup_;

    o_assert_dbg(this->setup.KeyBufferSize > 0);
    o_assert_dbg(this->setup.SampleBufferSize > 0);
    o_assert_dbg(this->setup.NumSampleBuffers > 0);
    o_assert_dbg(this->setup.MaxAnimCurves > 0);
    o_assert_dbg(this->setup.MaxAnimClips > 0);
    o_assert_dbg(this->setup.MaxAnimLibs > 0);

    this->setup = setup_;
    this->keyBuffer.Reserve(this->setup.KeyBufferSize);
    this->sampleBuffer.Reserve(this->setup.SampleBufferSize * this->setup.NumSampleBuffers);

    // reserve max size and prevent growing
    this->curves.Reserve(this->setup.MaxAnimCurves);
    this->curves.SetAllocStrategy(0, 0);
    this->clips.Reserve(this->setup.MaxAnimClips);
    this->clips.SetAllocStrategy(0, 0);
    this->libs.Reserve(this->setup.MaxAnimLibs);
    this->libs.SetAllocStrategy(0, 0);
    this->libNameMap.Reserve(this->setup.MaxAnimLibs);
    this->libNameMap.SetAllocStrategy(0, 0);
    this->libFreeSlots.Reserve(this->setup.MaxAnimLibs);
    this->libFreeSlots.SetAllocStrategy(0, 0);
    for (uint16_t i = 0; i < this->setup.MaxAnimLibs; i++) {
        this->libs.Add();
        this->libFreeSlots.Enqueue(i);
    }
    this->isValid = true;
}

//------------------------------------------------------------------------------
void
AnimSystem::Discard() {
    o_assert_dbg(this->isValid);

    this->keyBuffer.Clear();
    this->curves.Clear();
    this->clips.Clear();
    this->libs.Clear();
    this->libFreeSlots.Clear();
    this->isValid = false;
}

//------------------------------------------------------------------------------
void
AnimSystem::AddKeys(const uint8_t* data, int byteSize) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(data && (byteSize > 0));
    o_assert_dbg((this->keyBuffer.Size() + byteSize) <= this->keyBuffer.Capacity());

    this->keyBuffer.Add(data, byteSize);
}

//------------------------------------------------------------------------------
void
AnimSystem::AddCurve(const AnimCurve& curve) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(curve.KeyOffset != InvalidIndex);
    o_assert_dbg(curve.IpolType != AnimIpolType::Invalid);
    o_assert_dbg(curve.Format != AnimKeyFormat::Invalid);

    this->curves.Add(curve);
}

//------------------------------------------------------------------------------
void
AnimSystem::AddClip(const AnimClip& clip) {
    o_assert(this->isValid);
    o_assert_dbg(clip.StartCurveIndex != InvalidIndex);
    o_assert_dbg(clip.NumCurves > 0);
    o_assert_dbg(clip.NumKeys > 0);
    o_assert_dbg(clip.KeyStride > 0);
    o_assert_dbg(clip.KeyDuration > 0);

    this->clips.Add(clip);
}

//------------------------------------------------------------------------------
Id
AnimSystem::AddLibrary(const StringAtom& name, AnimLibrary&& lib) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(!this->libNameMap.Contains(name));
    o_assert_dbg(lib.StartClipIndex != InvalidIndex);
    o_assert_dbg(lib.NumClips > 0);
    o_assert_dbg(lib.StartCurveIndex != InvalidIndex);
    o_assert_dbg(lib.NumCurves > 0);
    o_assert_dbg(lib.StartKeyOffset != InvalidIndex);
    o_assert_dbg(lib.KeyRangeSize > 0);

    const uint16_t slotIndex = this->libFreeSlots.Dequeue();
    auto& slot = this->libs[slotIndex];
    slot = std::move(lib);
    slot.name = name;
    slot.id = Id(this->uniqueStamp++, slotIndex, 0);
    this->libNameMap.Add(name, slot.id);
    return slot.id;
}

//------------------------------------------------------------------------------
Id
AnimSystem::LookupLibrary(const StringAtom& name) {
    o_assert_dbg(this->isValid);
    int index = this->libNameMap.FindIndex(name);
    if (InvalidIndex != index) {
        return this->libNameMap.ValueAtIndex(index);
    }
    else {
        return Id::InvalidId();
    }
}

//------------------------------------------------------------------------------
const AnimLibrary*
AnimSystem::Library(const Id& id) const {
    o_assert_dbg(this->isValid);
    if (this->libs[id.SlotIndex].id == id) {
        return &(this->libs[id.SlotIndex]);
    }
    else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
void
AnimSystem::RemLibrary(const Id& id) {
    o_assert_dbg(this->isValid);
    o_assert(this->libs[id.SlotIndex].id == id);

    AnimLibrary& lib = this->libs[id.SlotIndex];
    StringAtom name = lib.name;
    const int keyOffset = lib.StartKeyOffset;
    const int keySize = lib.KeyRangeSize;
    const int curveIndex = lib.StartCurveIndex;
    const int numCurves = lib.NumCurves;
    const int clipIndex = lib.StartClipIndex;
    const int numClips = lib.NumClips;
    this->libNameMap.Erase(name);
    this->libFreeSlots.Enqueue(id.SlotIndex);
    lib = AnimLibrary();

    // need to remove keys, curves, clips belonging to the
    // anim lib, and move up the remaining objects to close the gaps
    this->keyBuffer.Remove(keyOffset, keySize);
    for (int i = 0; i < numCurves; i++) {
        this->curves.Erase(curveIndex);
    }
    for (int i = 0; i < numClips; i++) {
        this->clips.Erase(clipIndex);
    }

    // and finally fix up all the start indices/offsets
    for (auto& curve : this->curves) {
        o_assert_dbg(curve.KeyOffset != keyOffset);
        if (curve.KeyOffset > keyOffset) {
            curve.KeyOffset -= keySize;
            o_assert_dbg(curve.KeyOffset >= 0);
        }
    }
    for (auto& clip : this->clips) {
        o_assert_dbg(clip.StartCurveIndex != curveIndex);
        if (clip.StartCurveIndex > curveIndex) {
            clip.StartCurveIndex -= numCurves;
            o_assert_dbg(clip.StartCurveIndex >= 0);
        }
    }
    for (auto& lib : this->libs) {
        if (lib.id.IsValid()) {
            o_assert_dbg(lib.StartClipIndex != clipIndex);
            o_assert_dbg(lib.StartCurveIndex != curveIndex);
            o_assert_dbg(lib.StartKeyOffset != keyOffset);
            if (lib.StartClipIndex > clipIndex) {
                lib.StartClipIndex -= numClips;
                o_assert_dbg(lib.StartClipIndex >= 0);
            }
            if (lib.StartCurveIndex > curveIndex) {
                lib.StartCurveIndex -= numCurves;
                o_assert_dbg(lib.StartCurveIndex >= 0);
            }
            if (lib.StartKeyOffset > keyOffset) {
                lib.StartKeyOffset -= keySize;
                o_assert_dbg(lib.StartKeyOffset >= 0);
            }
        }
    }
}

//------------------------------------------------------------------------------
static int
wrapKeyIndex(int keyIndex, int numKeys, AnimWrapMode::Code wrapMode) {
    o_assert_dbg(numKeys > 0);
    if (AnimWrapMode::Loop == wrapMode) {
        keyIndex %= numKeys;
        o_assert_dbg((keyIndex >= 0) && (keyIndex < numKeys));
    }
    else {
        if (keyIndex < 0) {
            keyIndex = 0;
        }
        else if (keyIndex >= numKeys) {
            keyIndex = numKeys - 1;
        }
    }
    return keyIndex;
}

//------------------------------------------------------------------------------
static int
computeInbetweenTicks(int64_t time, int numKeys, int keyDuration, AnimWrapMode::Code wrapMode) {
    o_assert_dbg((numKeys > 0) && (keyDuration > 0));
    const int clipDuration = numKeys * keyDuration;
    if (AnimWrapMode::Loop == wrapMode) {
        time %= clipDuration;
        o_assert_dbg((time >= 0) && (time < clipDuration));
    }
    else {
        if (time < 0) {
            time = 0;
        }
        else if (time > clipDuration) {
            time = clipDuration;
        }
    }
    int inbetweenTicks = time % keyDuration;
    return inbetweenTicks;
}

//------------------------------------------------------------------------------
static void
unpack(AnimKeyFormat::Code fmt, const uint8_t*& srcPtr, float*& dstPtr) {
    switch (fmt) {
        case AnimKeyFormat::Float4:
            *dstPtr++ = *(float*)srcPtr; srcPtr+=4;
            // fallthrough
        case AnimKeyFormat::Float3:
            *dstPtr++ = *(float*)srcPtr; srcPtr+=4;
            // fallthrough
        case AnimKeyFormat::Float2:
            *dstPtr++ = *(float*)srcPtr; srcPtr+=4;
            // fallthrough
        case AnimKeyFormat::Float:
            *dstPtr++ = *(float*)srcPtr; srcPtr+=4;
            break;
        case AnimKeyFormat::Byte4N:
            *dstPtr++ = (float(*srcPtr)/255.0f)*2.0f-1.0f; srcPtr++;
            *dstPtr++ = (float(*srcPtr)/255.0f)*2.0f-1.0f; srcPtr++;
            *dstPtr++ = (float(*srcPtr)/255.0f)*2.0f-1.0f; srcPtr++;
            *dstPtr++ = (float(*srcPtr)/255.0f)*2.0f-1.0f; srcPtr++;
            break;
        case AnimKeyFormat::Short4N:
            *dstPtr++ = (float(*(uint16_t*)srcPtr)/32767.0f)*2.0f-1.0f; srcPtr+=2;
            *dstPtr++ = (float(*(uint16_t*)srcPtr)/32767.0f)*2.0f-1.0f; srcPtr+=2;
            // fallthrough
        case AnimKeyFormat::Short2N:
            *dstPtr++ = (float(*(uint16_t*)srcPtr)/32767.0f)*2.0f-1.0f; srcPtr+=2;
            *dstPtr++ = (float(*(uint16_t*)srcPtr)/32767.0f)*2.0f-1.0f; srcPtr+=2;
            break;
        default:
            // should not happen
            break;
    }
}

//------------------------------------------------------------------------------
static void
unpack_float(AnimKeyFormat::Code fmt, const float*& srcPtr, float*& dstPtr) {
    switch (fmt) {
        case AnimKeyFormat::Float4:
        case AnimKeyFormat::Byte4N:
        case AnimKeyFormat::Short4N:
            *dstPtr++ = *srcPtr++;
            // fallthrough
        case AnimKeyFormat::Float3:
            *dstPtr++ = *srcPtr++;
            // fallthrough
        case AnimKeyFormat::Float2:
        case AnimKeyFormat::Short2N:
            *dstPtr++ = *srcPtr++;
            // fallthrough
        case AnimKeyFormat::Float:
            *dstPtr++ = *srcPtr++;
            break;
        default:
            // should not happen
            break;
    }
}

//------------------------------------------------------------------------------
void
AnimSystem::Sample(int clipIndex, int64_t time, AnimSampleMode::Code sampleMode, AnimWrapMode::Code wrapMode, int dstSampleBufferIndex) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(clipIndex < this->clips.Size());
    o_assert_range(dstSampleBufferIndex, this->setup.NumSampleBuffers);

    // FIXME: add range checks for access to keybuffer and samplebuffer!

    const AnimClip& clip = this->clips[clipIndex];
    const auto* curve = &(this->curves[clip.StartCurveIndex]);
    const uint8_t* keyPtr = this->keyBuffer.Data();
    float* dstPtr = (float*)(this->sampleBuffer.Data() + dstSampleBufferIndex * this->setup.SampleBufferSize);
    const int keyIndex0 = wrapKeyIndex(time / clip.KeyDuration, clip.NumKeys, wrapMode);
    if (AnimSampleMode::Step == sampleMode) {
        for (int i = 0; i < clip.NumCurves; i++) {
            if (curve[i].IpolType == AnimIpolType::Constant) {
                const float* srcPtr = &(curve[i].Constant[0]);
                unpack_float(curve[i].Format, srcPtr, dstPtr);
            }
            else {
                const uint8_t* srcPtr = keyPtr + curve[i].KeyOffset + keyIndex0 * clip.KeyStride;
                unpack(curve[i].Format, srcPtr, dstPtr);
            }
        }
    }
    else {
        const int keyIndex1 = wrapKeyIndex(keyIndex0 + 1, clip.NumKeys, wrapMode);
        const int inbetweenTicks = computeInbetweenTicks(time, clip.NumKeys, clip.KeyDuration, wrapMode);
        const float weight = float(inbetweenTicks) / float(clip.KeyDuration);
        float* dstPtr = (float*)(this->sampleBuffer.Data() + dstSampleBufferIndex * this->setup.SampleBufferSize);
        float unpackBuffer[8];
        for (int i = 0; i < clip.NumCurves; i++) {
            if (curve[i].IpolType == AnimIpolType::Constant) {
                const float* srcPtr = &(curve[i].Constant[0]);
                unpack_float(curve[i].Format, srcPtr, dstPtr);
            }
            else {
                float* sample0 = &(unpackBuffer[0]);
                float* sample1 = &(unpackBuffer[4]);
                const uint8_t* src0Ptr = keyPtr + curve[i].KeyOffset + keyIndex0 * clip.KeyStride;
                const uint8_t* src1Ptr = keyPtr + curve[i].KeyOffset + keyIndex1 * clip.KeyStride;
                unpack(curve[i].Format, src0Ptr, sample0);
                unpack(curve[i].Format, src1Ptr, sample1);
                // FIXME: use simple linear interpolation for quaternions, assumes
                // that rotation keys are close to each other!
                switch (curve[i].Format) {
                    float s0;
                    case AnimKeyFormat::Float4:
                    case AnimKeyFormat::Byte4N:
                    case AnimKeyFormat::Short4N:
                        s0 = *sample0++; *dstPtr++ = s0 + (*sample1++ - s0) * weight;
                        // fallthrough
                    case AnimKeyFormat::Float3:
                        s0 = *sample0++; *dstPtr++ = s0 + (*sample1++ - s0) * weight;
                        // fallthrough
                    case AnimKeyFormat::Float2:
                    case AnimKeyFormat::Short2N:
                        s0 = *sample0++; *dstPtr++ = s0 + (*sample1++ - s0) * weight;
                        // fallthrough
                    case AnimKeyFormat::Float:
                        s0 = *sample0++; *dstPtr++ = s0 + (*sample1++ - s0) * weight;
                        break;
                    default:
                        // should not happen
                        break;
                }
            }
        }
    }
}

} // namespace Oryol


