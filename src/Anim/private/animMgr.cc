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
    this->libPool.Setup(resTypeLib, setup.MaxNumLibs);
    this->clipPool.SetAllocStrategy(0, 0);  // disable reallocation
    this->clipPool.Reserve(setup.MaxNumClips);
    this->curvePool.SetAllocStrategy(0, 0); // disable reallocation
    this->curvePool.Reserve(setup.MaxNumCurves);
    const int numValues = setup.MaxNumKeys + setup.MaxNumSamples;
    this->valuePool = (float*) Memory::Alloc(numValues * sizeof(float));
    this->keys = ArrayView<float>(this->valuePool, 0, setup.MaxNumKeys);
    this->samples = ArrayView<float>(this->valuePool, setup.MaxNumKeys, setup.MaxNumSamples);
}

//------------------------------------------------------------------------------
void
animMgr::discard() {
    o_assert_dbg(this->isValid);
    o_assert_dbg(this->valuePool);

    this->destroy(ResourceLabel::All);
    this->resContainer.Discard();
    this->libPool.Discard();
    o_assert_dbg(this->clipPool.Empty());
    o_assert_dbg(this->curvePool.Empty());
    this->keys = ArrayView<float>();
    this->samples = ArrayView<float>();
    Memory::Free(this->valuePool);
    this->valuePool = nullptr;
    this->isValid = false;
}

//------------------------------------------------------------------------------
Id
animMgr::createLibrary(const AnimLibrarySetup& libSetup) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(libSetup.Name.IsValid());
    o_assert_dbg(!libSetup.CurveLayout.Empty());
    o_assert_dbg(!libSetup.Clips.Empty());

    // check if lib already exists
    Id resId = this->resContainer.registry.Lookup(libSetup.Name);
    if (resId.IsValid()) {
        o_assert_dbg(resId.Type == resTypeLib);
        return resId;
    }

    // before creating new lib, validate setup params and check against pool limits
    if ((this->clipPool.Size() + libSetup.Clips.Size()) >= this->clipPool.Capacity()) {
        o_warn("Anim: clip pool exhausted!\n");
        return Id::InvalidId();
    }
    if ((this->curvePool.Size() + libSetup.CurveLayout.Size()) >= this->curvePool.Capacity()) {
        o_warn("Anim: curve pool exhausted!\n");
        return Id::InvalidId();
    }
    int libNumKeys = 0;
    for (const auto& clipSetup : libSetup.Clips) {
        if (clipSetup.Curves.Size() != libSetup.CurveLayout.Size()) {
            o_warn("Anim: curve number mismatch in clip '%s'!\n", clipSetup.Name.AsCStr());
            return Id::InvalidId();
        }
        for (int i = 0; i < clipSetup.Curves.Size(); i++) {
            if (!clipSetup.Curves[i].Static) {
                libNumKeys += clipSetup.Length * AnimCurveFormat::Stride(libSetup.CurveLayout[i]); 
            }
        }
    }
    if ((this->numKeys + libNumKeys) >= this->keys.Size()) {
        o_warn("Anim: key pool exhausted!\n");
        return Id::InvalidId();
    }
    int libSampleStride = 0;
    for (auto fmt : libSetup.CurveLayout) {
        libSampleStride += AnimCurveFormat::Stride(fmt);
    }
    const int libNumSamples = libSampleStride * libSetup.MaxNumInstances;
    if ((this->numSamples + libNumSamples) >= this->samples.Size()) {
        o_warn("Anim: sample pool exhausted!\n");
        return Id::InvalidId();
    }

    // create a new lib
    resId = this->libPool.AllocId();
    AnimLibrary& lib = this->libPool.Assign(resId, ResourceState::Setup);
    lib.Name = libSetup.Name;
    lib.MaxNumInstances = libSetup.MaxNumInstances;
    lib.SampleStride = libSampleStride;
    lib.ClipIndexMap.Reserve(libSetup.Clips.Size());
    const int curvePoolIndex = this->curvePool.Size();
    const int clipPoolIndex = this->clipPool.Size();
    int clipKeyIndex = 0;
    for (const auto& clipSetup : libSetup.Clips) {
        lib.ClipIndexMap.Add(clipSetup.Name, this->clipPool.Size());
        this->clipPool.Add();
        AnimClip& clip = this->clipPool.Back();
        clip.Name = clipSetup.Name;
        clip.Length = clipSetup.Length;
        clip.KeyDuration = clipSetup.KeyDuration;
        const int curveIndex = this->curvePool.Size();
        for (int curveIndex = 0; curveIndex < clipSetup.Curves.Size(); curveIndex++) {
            const auto& curveSetup = clipSetup.Curves[curveIndex];
            this->curvePool.Add();
            AnimCurve& curve = this->curvePool.Back();
            curve.Static = curveSetup.Static;
            curve.Format = libSetup.CurveLayout[curveIndex];
            curve.StaticValue = curveSetup.StaticValue;
            if (!curve.Static) {
                curve.KeyIndex = clip.KeyStride;
                curve.Stride = AnimCurveFormat::Stride(curve.Format);
                clip.KeyStride += curve.Stride;
            }
        }
        clip.Curves = this->curvePool.View(curveIndex, clipSetup.Curves.Size());
        const int clipNumKeys = clip.KeyStride * clip.Length;
        if (clipNumKeys > 0) {
            clip.Keys = this->keys.View(clipKeyIndex, clipNumKeys);
            clipKeyIndex += clipNumKeys;
        }
    }
    o_assert_dbg(clipKeyIndex == libNumKeys);
    lib.Keys = this->keys.View(this->numKeys, libNumKeys);
    this->numKeys += libNumKeys;
    lib.Samples = this->samples.View(this->numSamples, libNumSamples);
    this->numSamples += libNumSamples;
    lib.Curves = this->curvePool.View(curvePoolIndex, libSetup.Clips.Size() * libSetup.CurveLayout.Size());
    lib.Clips = this->clipPool.View(clipPoolIndex, libSetup.Clips.Size());

    // initialize clips with their default values
    for (auto& clip : lib.Clips) {
        for (int row = 0; row < clip.Length; row++) {
            int offset = row * clip.KeyStride;
            for (const auto& curve : clip.Curves) {
                for (int i = 0; i < curve.Stride; i++) {
                    clip.Keys[offset++] = curve.StaticValue[i];
                }
            }
        }
    }

    this->resContainer.registry.Add(libSetup.Name, resId, this->resContainer.PeekLabel());
    this->libPool.UpdateState(resId, ResourceState::Valid);
    return resId;
}

//------------------------------------------------------------------------------
AnimLibrary*
animMgr::lookupLibrary(const Id& resId) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(resId.Type == resTypeLib);
    return this->libPool.Lookup(resId);
}

//------------------------------------------------------------------------------
void
animMgr::destroy(const ResourceLabel& label) {
    o_assert_dbg(this->isValid);
    Array<Id> ids = this->resContainer.registry.Remove(label);
    for (const Id& id : ids) {
        switch (id.Type) {
            case resTypeLib:
                this->destroyLibrary(id);
                break;
            // FIXME: more types?
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::destroyLibrary(const Id& id) {
    AnimLibrary* lib = this->libPool.Lookup(id);
    if (lib) {
        this->removeClips(lib->Clips);
        this->removeCurves(lib->Curves);
        this->removeKeys(lib->Keys);
        this->removeSamples(lib->Samples);
        lib->clear();
    }
    this->libPool.Unassign(id);
}

//------------------------------------------------------------------------------
void
animMgr::removeKeys(ArrayView<float> range) {
    o_assert_dbg(this->valuePool);
    if (range.Empty()) {
        return;
    }
    const int rangeEnd = range.StartIndex() + range.Size();
    const int numKeysToMove = this->numKeys - rangeEnd;
    if (numKeysToMove > 0) {
        Memory::Move(range.end(), (void*)range.begin(), numKeysToMove * sizeof(float)); 
    }
    this->numKeys -= range.Size();
    o_assert_dbg(this->numKeys >= 0);

    // fix the key array views in libs and clips
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid() && (lib.Keys.StartIndex() >= rangeEnd)) {
            lib.Keys.SetStartIndex(lib.Keys.StartIndex() - range.Size());
        }
    }
    for (auto& clip : this->clipPool) {
        if (clip.Keys.StartIndex() >= rangeEnd) {
            clip.Keys.SetStartIndex(clip.Keys.StartIndex() - range.Size());
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeSamples(ArrayView<float> range) {
    o_assert_dbg(this->valuePool);
    if (range.Empty()) {
        return;
    }
    const int rangeEnd = range.StartIndex() + range.Size();
    const int numSamplesToMove = this->numSamples - rangeEnd;
    if (numSamplesToMove > 0) {
        Memory::Move(range.end(), (void*)range.begin(), numSamplesToMove * sizeof(float));
    }
    this->numSamples -= range.Size();
    o_assert_dbg(this->numSamples >= 0);

    // fix sample array views in libs
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid() && (lib.Samples.StartIndex() >= rangeEnd)) {
            lib.Samples.SetStartIndex(lib.Samples.StartIndex() - range.Size());
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeCurves(ArrayView<AnimCurve> range) {
    this->curvePool.EraseRange(range.StartIndex(), range.Size()); 

    // fix the curve array views in libs and clips
    const int rangeEnd = range.StartIndex() + range.Size();
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid() && (lib.Curves.StartIndex() >= rangeEnd)) {
            lib.Curves.SetStartIndex(lib.Curves.StartIndex() - range.Size());
        }
    }
    for (auto& clip : this->clipPool) {
        if (clip.Curves.StartIndex() >= rangeEnd) {
            clip.Curves.SetStartIndex(clip.Curves.StartIndex() - range.Size());
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeClips(ArrayView<AnimClip> range) {
    this->clipPool.EraseRange(range.StartIndex(), range.Size());

    // fix the clip array views in libs
    const int rangeEnd = range.StartIndex() + range.Size();
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid() && (lib.Clips.StartIndex() >= rangeEnd)) {
            lib.Clips.SetStartIndex(lib.Clips.StartIndex() - range.Size());
        }
    }
}

} // namespace _priv
} // namespace Oryol
