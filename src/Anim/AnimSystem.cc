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
AnimSystem::Setup(const AnimSystemSetup& setup) {
    o_assert_dbg(!this->isValid);
    o_assert_dbg(setup.AnimKeyBufferSize > 0);
    o_assert_dbg(setup.MaxAnimCurves > 0);
    o_assert_dbg(setup.MaxAnimClips > 0);
    o_assert_dbg(setup.MaxAnimLibs > 0);

    this->keyBuffer.Reserve(setup.AnimKeyBufferSize);

    // reserve max size and prevent growing
    this->curves.Reserve(setup.MaxAnimCurves);
    this->curves.SetAllocStrategy(0, 0);
    this->clips.Reserve(setup.MaxAnimClips);
    this->clips.SetAllocStrategy(0, 0);
    this->libs.Reserve(setup.MaxAnimLibs);
    this->libs.SetAllocStrategy(0, 0);
    this->libNameMap.Reserve(setup.MaxAnimLibs);
    this->libNameMap.SetAllocStrategy(0, 0);
    this->libFreeSlots.Reserve(setup.MaxAnimLibs);
    this->libFreeSlots.SetAllocStrategy(0, 0);
    for (uint16_t i = 0; i < setup.MaxAnimLibs; i++) {
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

} // namespace Oryol


