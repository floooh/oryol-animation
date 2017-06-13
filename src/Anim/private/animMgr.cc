//------------------------------------------------------------------------------
//  animMgr.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animMgr.h"
#include "Core/Memory/Memory.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
animMgr::~animMgr() {
    o_assert_dbg(!this->isValid);
}

//------------------------------------------------------------------------------
void
animMgr::setup(const AnimSetup& setup) {
    o_assert_dbg(!this->isValid);

    this->isValid = true;
    this->resContainer.Setup(setup.ResourceLabelStackCapacity, setup.ResourceRegistryCapacity);
    this->libPool.Setup(resTypeLib, setup.MaxNumLibs);
    this->skelPool.Setup(resTypeSkeleton, setup.MaxNumSkeletons);
    this->clipPool.SetAllocStrategy(0, 0);  // disable reallocation
    this->clipPool.Reserve(setup.ClipPoolCapacity);
    this->curvePool.SetAllocStrategy(0, 0); // disable reallocation
    this->curvePool.Reserve(setup.CurvePoolCapacity);
    this->matrixPool.SetAllocStrategy(0, 0);
    this->matrixPool.Reserve(setup.MatrixPoolCapacity);
    this->valuePool = (float*) Memory::Alloc(setup.KeyPoolCapacity * sizeof(float));
    this->keys = Slice<float>(this->valuePool, setup.KeyPoolCapacity);
}

//------------------------------------------------------------------------------
void
animMgr::discard() {
    o_assert_dbg(this->isValid);
    o_assert_dbg(this->valuePool);

    this->destroy(ResourceLabel::All);
    this->resContainer.Discard();
    this->skelPool.Discard();
    this->libPool.Discard();
    o_assert_dbg(this->clipPool.Empty());
    o_assert_dbg(this->curvePool.Empty());
    o_assert_dbg(this->matrixPool.Empty());
    this->keys = Slice<float>();
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

    // create a new lib
    resId = this->libPool.AllocId();
    AnimLibrary& lib = this->libPool.Assign(resId, ResourceState::Setup);
    lib.Name = libSetup.Name;
    lib.SampleStride = 0;
    for (auto fmt : libSetup.CurveLayout) {
        lib.SampleStride += AnimCurveFormat::Stride(fmt);
    }
    lib.ClipIndexMap.Reserve(libSetup.Clips.Size());
    const int curvePoolIndex = this->curvePool.Size();
    const int clipPoolIndex = this->clipPool.Size();
    int clipKeyIndex = this->numKeys;
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
        clip.Curves = this->curvePool.MakeSlice(curveIndex, clipSetup.Curves.Size());
        const int clipNumKeys = clip.KeyStride * clip.Length;
        if (clipNumKeys > 0) {
            clip.Keys = this->keys.MakeSlice(clipKeyIndex, clipNumKeys);
            clipKeyIndex += clipNumKeys;
        }
    }
    o_assert_dbg(clipKeyIndex == (this->numKeys + libNumKeys));
    lib.Keys = this->keys.MakeSlice(this->numKeys, libNumKeys);
    this->numKeys += libNumKeys;
    lib.Curves = this->curvePool.MakeSlice(curvePoolIndex, libSetup.Clips.Size() * libSetup.CurveLayout.Size());
    lib.Clips = this->clipPool.MakeSlice(clipPoolIndex, libSetup.Clips.Size());

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
            case resTypeSkeleton:
                this->destroySkeleton(id);
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
        lib->clear();
    }
    this->libPool.Unassign(id);
}

//------------------------------------------------------------------------------
Id
animMgr::createSkeleton(const AnimSkeletonSetup& setup) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(setup.Name.IsValid());
    o_assert_dbg(!setup.Bones.Empty());

    // check if skeleton already exists
    Id resId = this->resContainer.registry.Lookup(setup.Name);
    if (resId.IsValid()) {
        o_assert_dbg(resId.Type == resTypeSkeleton);
        return resId;
    }
    
    // check if resource limits are reached
    if ((this->matrixPool.Size() + setup.Bones.Size()*2) >= this->matrixPool.Capacity()) {
        o_warn("Anim: matrix pool exhausted!\n");
        return Id::InvalidId();
    }
    
    // create new skeleton
    resId = this->skelPool.AllocId();
    AnimSkeleton& skel = this->skelPool.Assign(resId, ResourceState::Setup);
    skel.Name = setup.Name;
    skel.NumBones = setup.Bones.Size();
    const int matrixPoolIndex = this->matrixPool.Size();
    for (const auto& bone : setup.Bones) {
        this->matrixPool.Add(glm::inverse(bone.InvBindPose));
    }
    for (const auto& bone : setup.Bones) {
        this->matrixPool.Add(bone.InvBindPose);
    }
    skel.Matrices = this->matrixPool.MakeSlice(matrixPoolIndex, skel.NumBones * 2);
    skel.BindPose = skel.Matrices.MakeSlice(0, skel.NumBones);
    skel.InvBindPose = skel.Matrices.MakeSlice(skel.NumBones, skel.NumBones);
    for (int i = 0; i < skel.NumBones; i++) {
        skel.ParentIndices[i] = setup.Bones[i].ParentIndex;
    }

    // register the new resource, and done
    this->resContainer.registry.Add(setup.Name, resId, this->resContainer.PeekLabel());
    this->skelPool.UpdateState(resId, ResourceState::Valid);
    return resId;
}

//------------------------------------------------------------------------------
AnimSkeleton*
animMgr::lookupSkeleton(const Id& resId) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(resId.Type == resTypeSkeleton);
    return this->skelPool.Lookup(resId);
}

//------------------------------------------------------------------------------
void
animMgr::destroySkeleton(const Id& id) {
    AnimSkeleton* skel = this->skelPool.Lookup(id);
    if (skel) {
        this->removeMatrices(skel->Matrices);
        skel->clear();
    }
    this->skelPool.Unassign(id);
}

//------------------------------------------------------------------------------
void
animMgr::removeKeys(Slice<float> range) {
    o_assert_dbg(this->valuePool);
    if (range.Empty()) {
        return;
    }
    const int numKeysToMove = this->numKeys - (range.Offset() + range.Size());
    if (numKeysToMove > 0) {
        Memory::Move(range.end(), (void*)range.begin(), numKeysToMove * sizeof(float)); 
    }
    this->numKeys -= range.Size();
    o_assert_dbg(this->numKeys >= 0);

    // fix the key array views in libs and clips
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid()) {
            lib.Keys.FillGap(range.Offset(), range.Size());
        }
    }
    for (auto& clip : this->clipPool) {
        clip.Keys.FillGap(range.Offset(), range.Size());
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeCurves(Slice<AnimCurve> range) {
    this->curvePool.EraseRange(range.Offset(), range.Size()); 

    // fix the curve array views in libs and clips
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid()) {
            lib.Curves.FillGap(range.Offset(), range.Size());
        }
    }
    for (auto& clip : this->clipPool) {
        clip.Curves.FillGap(range.Offset(), range.Size());
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeClips(Slice<AnimClip> range) {
    this->clipPool.EraseRange(range.Offset(), range.Size());

    // fix the clip array views in libs
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->libPool.LastAllocSlot; slotIndex++) {
        AnimLibrary& lib = this->libPool.slots[slotIndex];
        if (lib.Id.IsValid()) {
            lib.Clips.FillGap(range.Offset(), range.Size());
        }
    }
}

//------------------------------------------------------------------------------
void
animMgr::removeMatrices(Slice<glm::mat4> range) {
    this->matrixPool.EraseRange(range.Offset(), range.Size());

    // fix the skeleton matrix slices
    for (Id::SlotIndexT slotIndex = 0; slotIndex <= this->skelPool.LastAllocSlot; slotIndex++) {
        AnimSkeleton& skel = this->skelPool.slots[slotIndex];
        if (skel.Id.IsValid()) {
            skel.BindPose.FillGap(range.Offset(), range.Size());
            skel.InvBindPose.FillGap(range.Offset(), range.Size());
        }
    }
}

} // namespace _priv
} // namespace Oryol
