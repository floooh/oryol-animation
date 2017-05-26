//------------------------------------------------------------------------------
//  animMgr.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animMgr.h"
#include "Core/Memory/Memory.h"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
void
animMgr::setup(const AnimSetup& setup) {
    o_assert_dbg(!this->isValid);

    this->isValid = true;
    this->resContainer.Setup(setup.ResourceLabelStackCapacity, setup.ResourceRegistryCapacity);
    this->clipPool.Setup(resTypeClip, setup.MaxNumClips);
    this->curvePool.SetAllocStrategy(0, 0); // disable growing
    this->curvePool.Reserve(setup.MaxNumCurves);
    this->maxKeys = setup.MaxNumKeys;
    this->numKeys = 0;
    this->keyPool = (float*) Memory::Alloc(this->maxKeys * sizeof(float));
}

//------------------------------------------------------------------------------
void
animMgr::discard() {
    o_assert_dbg(this->isValid);
    o_assert_dbg(this->keyPool);

    this->destroy(ResourceLabel::All);
    this->resContainer.Discard();
    this->clipPool.Discard();
    this->curvePool.Clear();
    Memory::Free(this->keyPool);
    this->keyPool = nullptr;
    this->isValid = false;
}

//------------------------------------------------------------------------------
Id
animMgr::createClip(const AnimClipSetup& setup) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(setup.Name.IsValid());
    o_assert_dbg(setup.NumCurves > 0);
    o_assert_dbg(setup.NumKeys > 0);
    o_assert_dbg(setup.KeyDuration > 0.0f);
    o_assert_dbg(setup.InitCurve);
    o_assert_dbg(setup.InitKeys);

    // if clip with same name already exists, return the same clip
    Id resId = this->resContainer.registry.Lookup(setup.Name);
    if (resId.IsValid()) {
        o_assert_dbg(resId.Type == resTypeClip);
        return resId;
    }

    // check if there's enough room in the curve pool
    if ((this->curvePool.Size() + setup.NumCurves) >= this->curvePool.Capacity()) {
        o_warn("Anim: curve pool exhausted!\n");
        return Id::InvalidId();
    }

    // named clip didn't exist, create one
    resId = this->clipPool.AllocId();
    AnimClip& clip = this->clipPool.Assign(resId, ResourceState::Setup);
    clip.Name = setup.Name;
    clip.NumCurves = setup.NumCurves;
    clip.NumKeys = setup.NumKeys;
    clip.KeyDuration = setup.KeyDuration;
    clip.curveIndex = this->curvePool.Size();

    // initialize the anim curves
    clip.KeyStride = 0;
    for (int curveIndex = 0; curveIndex < clip.NumCurves; curveIndex++) {
        this->curvePool.Add();
        AnimCurve& curve = this->curvePool.Back();
        setup.InitCurve(clip, curveIndex, curve);
        o_assert_dbg(curve.Format != AnimCurveFormat::Invalid);
        if (curve.Format != AnimCurveFormat::Static) {
            curve.keyIndex = clip.KeyStride;
            clip.KeyStride += AnimCurveFormat::Stride(curve.Format);
        }
        else {
            curve.keyIndex = InvalidIndex;
        }
    }

    // reserve and initialize keys in key pool
    const int numKeysInClip = clip.KeyStride * clip.NumKeys;
    if ((this->numKeys + numKeysInClip) >= this->maxKeys) {
        o_warn("Anim: key pool exhausted!");
        this->destroyClip(resId);
        return Id::InvalidId();
    }
    clip.keyIndex = this->numKeys;
    clip.numPoolKeys = numKeysInClip;
    this->numKeys += numKeysInClip;
    ArrayView<AnimCurve> curves = this->curvePool.View(clip.curveIndex, clip.NumCurves);
    ArrayView<float> keys(&this->keyPool[clip.keyIndex], clip.numPoolKeys);
    setup.InitKeys(clip, curves, keys);

    this->resContainer.registry.Add(setup.Name, resId, this->resContainer.PeekLabel());
    this->clipPool.UpdateState(resId, ResourceState::Valid);
    return resId;
}

//------------------------------------------------------------------------------
AnimClip*
animMgr::lookupClip(const Id& resId) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(resId.Type == resTypeClip);
    return this->clipPool.Lookup(resId);
}

//------------------------------------------------------------------------------
void
animMgr::destroy(const ResourceLabel& label) {
    o_assert_dbg(this->isValid);
    Array<Id> ids = this->resContainer.registry.Remove(label);
    for (const Id& id : ids) {
        switch (id.Type) {
            case resTypeClip:
                this->destroyClip(id);
                break;
            // FIXME: more types?
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::destroyClip(const Id& id) {
    AnimClip* clip = this->clipPool.Lookup(id);
    if (clip) {
        if ((clip->keyIndex != InvalidIndex) && (clip->numPoolKeys > 0)) {
            this->removeKeys(clip->keyIndex, clip->numPoolKeys);
        }
        if ((clip->curveIndex != InvalidIndex) && (clip->NumCurves > 0)) {
            this->removeCurves(clip->curveIndex, clip->NumCurves);
        }
        clip->clear();
    }
    this->clipPool.Unassign(id);
}

//------------------------------------------------------------------------------
void
animMgr::removeKeys(int keyIndex, int numKeys) {
    o_assert_dbg(this->keyPool);
    o_assert_dbg((keyIndex >= 0) && ((keyIndex + numKeys) <= this->numKeys));;

    const float* moveFrom = this->keyPool + keyIndex + numKeys;
    float* moveTo = this->keyPool + keyIndex;
    const int numKeysToMove = this->numKeys - (keyIndex + numKeys);
    if (numKeysToMove > 0) {
        Memory::Move(moveFrom, moveTo, numKeysToMove * sizeof(float)); 
    }

    // fix up the anim clip start key indices
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->clipPool.LastAllocSlot; slotIndex++) {
        AnimClip& clip = this->clipPool.slots[slotIndex];
        if (clip.Id.IsValid() && (InvalidIndex != clip.keyIndex)) {
            if (clip.keyIndex > keyIndex) {
                clip.keyIndex -= numKeys;
                o_assert_dbg(clip.keyIndex >= 0);
            }
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeCurves(int curveIndex, int numCurves) {
    o_assert_dbg((curveIndex >= 0) && ((curveIndex + numCurves) <= this->curvePool.Size()));
    this->curvePool.EraseRange(curveIndex, numCurves); 

    // fix up the anim clip curve indices
    for (Id::SlotIndexT slotIndex = 0; slotIndex < this->clipPool.LastAllocSlot; slotIndex++) {
        AnimClip& clip = this->clipPool.slots[slotIndex];
        if (clip.Id.IsValid() && (InvalidIndex != clip.curveIndex)) {
            if (clip.curveIndex > curveIndex) {
                clip.curveIndex -= numCurves;
                o_assert_dbg(clip.curveIndex >= 0);
            }
        }
    }
}

} // namespace _priv
} // namespace Oryol
