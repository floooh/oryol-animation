//------------------------------------------------------------------------------
//  animMgr.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animMgr.h"
#include "Core/Memory/Memory.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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

    this->animSetup = setup;
    this->isValid = true;
    this->resContainer.Setup(setup.ResourceLabelStackCapacity, setup.ResourceRegistryCapacity);
    this->libPool.Setup(resTypeLib, setup.MaxNumLibs);
    this->skelPool.Setup(resTypeSkeleton, setup.MaxNumSkeletons);
    this->instPool.Setup(resTypeInstance, setup.MaxNumInstances);
    this->clipPool.SetAllocStrategy(0, 0);  // disable reallocation
    this->clipPool.Reserve(setup.ClipPoolCapacity);
    this->curvePool.SetAllocStrategy(0, 0); // disable reallocation
    this->curvePool.Reserve(setup.CurvePoolCapacity);
    this->matrixPool.SetAllocStrategy(0, 0);
    this->matrixPool.Reserve(setup.MatrixPoolCapacity);
    this->activeInstances.SetAllocStrategy(0, 0);
    this->activeInstances.Reserve(setup.MaxNumActiveInstances);
    this->skinMatrixInfo.InstanceInfos.Reserve(setup.MaxNumActiveInstances);
    const int numValues = setup.KeyPoolCapacity + setup.SamplePoolCapacity;
    this->valuePool = (float*) Memory::Alloc(numValues * sizeof(float));
    this->keys = Slice<float>(this->valuePool, numValues, 0, setup.KeyPoolCapacity);
    this->samples = Slice<float>(this->valuePool, numValues, setup.KeyPoolCapacity, setup.SamplePoolCapacity);
    this->skinMatrixTableStride = setup.SkinMatrixTableWidth * 4;
    const int skinMatrixPoolNumFloats = this->skinMatrixTableStride * setup.SkinMatrixTableHeight;
    const int skinMatrixPoolSize = skinMatrixPoolNumFloats * sizeof(float);
    this->skinMatrixPool = (float*) Memory::Alloc(skinMatrixPoolSize);
    Memory::Clear(this->skinMatrixPool, skinMatrixPoolSize);
    this->skinMatrixTable = Slice<float>(this->skinMatrixPool, skinMatrixPoolNumFloats);
    this->skinMatrixInfo.SkinMatrixTable = this->skinMatrixTable.begin();
}

//------------------------------------------------------------------------------
void
animMgr::discard() {
    o_assert_dbg(this->isValid);
    o_assert_dbg(this->valuePool);
    o_assert_dbg(this->skinMatrixPool);

    this->destroy(ResourceLabel::All);
    this->resContainer.Discard();
    this->instPool.Discard();
    this->skelPool.Discard();
    this->libPool.Discard();
    o_assert_dbg(this->clipPool.Empty());
    o_assert_dbg(this->curvePool.Empty());
    o_assert_dbg(this->matrixPool.Empty());
    this->activeInstances.Clear();
    this->keys.Reset();
    this->samples.Reset();
    this->skinMatrixTable.Reset();
    Memory::Free(this->skinMatrixPool);
    this->skinMatrixPool = nullptr;
    Memory::Free(this->valuePool);
    this->valuePool = nullptr;
    this->isValid = false;
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
            case resTypeInstance:
                this->destroyInstance(id);
                break;
            default:
                o_assert2_dbg(false, "animMgr::destroy: unknown resource type\n");
                break;
        }
    }
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
    if ((this->clipPool.Size() + libSetup.Clips.Size()) > this->clipPool.Capacity()) {
        o_warn("Anim: clip pool exhausted!\n");
        return Id::InvalidId();
    }
    if ((this->curvePool.Size() + libSetup.CurveLayout.Size()) > this->curvePool.Capacity()) {
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
    if ((this->numKeys + libNumKeys) > this->keys.Size()) {
        o_warn("Anim: key pool exhausted!\n");
        return Id::InvalidId();
    }

    // create a new lib
    resId = this->libPool.AllocId();
    AnimLibrary& lib = this->libPool.Assign(resId, ResourceState::Setup);
    lib.Name = libSetup.Name;
    lib.SampleStride = 0;
    for (AnimCurveFormat::Enum fmt : libSetup.CurveLayout) {
        lib.CurveLayout.Add(fmt);
    }
    for (auto fmt : libSetup.CurveLayout) {
        lib.SampleStride += AnimCurveFormat::Stride(fmt);
    }
    lib.ClipIndexMap.Reserve(libSetup.Clips.Size());
    const int curvePoolIndex = this->curvePool.Size();
    const int clipPoolIndex = this->clipPool.Size();
    int clipKeyIndex = this->numKeys;
    for (const auto& clipSetup : libSetup.Clips) {
        lib.ClipIndexMap.Add(clipSetup.Name, this->clipPool.Size());
        AnimClip& clip = this->clipPool.Add();
        clip.Name = clipSetup.Name;
        clip.Length = clipSetup.Length;
        clip.KeyDuration = clipSetup.KeyDuration;
        const int curveIndex = this->curvePool.Size();
        for (int curveIndex = 0; curveIndex < clipSetup.Curves.Size(); curveIndex++) {
            const auto& curveSetup = clipSetup.Curves[curveIndex];
            AnimCurve& curve = this->curvePool.Add();
            curve.Static = curveSetup.Static;
            curve.Format = libSetup.CurveLayout[curveIndex];
            curve.NumValues = AnimCurveFormat::Stride(curve.Format);
            curve.StaticValue = curveSetup.StaticValue;
            if (!curve.Static) {
                curve.KeyIndex = clip.KeyStride;
                curve.KeyStride = AnimCurveFormat::Stride(curve.Format);
                clip.KeyStride += curve.KeyStride;
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
                for (int i = 0; i < curve.KeyStride; i++) {
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
    if ((this->matrixPool.Size() + setup.Bones.Size()*2) > this->matrixPool.Capacity()) {
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
        this->matrixPool.Add(bone.BindPose);
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
Id
animMgr::createInstance(const AnimInstanceSetup& setup) {
    o_assert_dbg(setup.Library.IsValid());
    
    Id resId = this->instPool.AllocId();
    animInstance& inst = this->instPool.Assign(resId, ResourceState::Setup);
    o_assert_dbg((inst.library == nullptr) && (inst.skeleton == nullptr));
    inst.library = this->lookupLibrary(setup.Library);
    o_assert_dbg(inst.library);
    if (setup.Skeleton.IsValid()) {
        inst.skeleton = this->lookupSkeleton(setup.Skeleton);
        o_assert_dbg(inst.skeleton);
    }
    this->resContainer.registry.Add(Locator::NonShared(), resId, this->resContainer.PeekLabel());
    this->instPool.UpdateState(resId, ResourceState::Valid);
    return resId;
}

//------------------------------------------------------------------------------
animInstance*
animMgr::lookupInstance(const Id& resId) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(resId.Type == resTypeInstance);
    return this->instPool.Lookup(resId);
}

//------------------------------------------------------------------------------
void
animMgr::destroyInstance(const Id& id) {
    animInstance* inst = this->instPool.Lookup(id);
    if (inst) {
        inst->clear();
    }
    this->instPool.Unassign(id);
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

//------------------------------------------------------------------------------
void
animMgr::writeKeys(AnimLibrary* lib, const uint8_t* ptr, int numBytes) {
    o_assert_dbg(lib && ptr && numBytes > 0);
    o_assert_dbg(lib->Keys.Size()*sizeof(float) == numBytes);
    Memory::Copy(ptr, lib->Keys.begin(), numBytes);
}

//------------------------------------------------------------------------------
void
animMgr::newFrame() {
    o_assert_dbg(!this->inFrame);
    for (animInstance* inst : this->activeInstances) {
        inst->samples.Reset();
        inst->skinMatrices.Reset();
    }
    this->activeInstances.Clear();
    this->numSamples = 0;
    this->curSkinMatrixTableX = 0;
    this->curSkinMatrixTableY = 0;
    this->inFrame = true;
    this->skinMatrixInfo.SkinMatrixTableByteSize = 0;
    this->skinMatrixInfo.InstanceInfos.Clear();
}

//------------------------------------------------------------------------------
bool
animMgr::addActiveInstance(animInstance* inst) {
    o_assert_dbg(inst && inst->library);
    o_assert_dbg(this->inFrame);
    
    // check if resource limits are reached for this frame
    if (this->activeInstances.Size() == this->activeInstances.Capacity()) {
        // MaxNumActiveInstances reached
        return false;
    }
    if ((this->numSamples + inst->library->SampleStride) > this->samples.Size()) {
        // no more room in samples pool
        return false;
    }
    if (inst->skeleton) {
        if (((this->curSkinMatrixTableX + (inst->skeleton->NumBones*3)) > this->animSetup.SkinMatrixTableWidth) &&
            ((this->curSkinMatrixTableY + 1) > this->animSetup.SkinMatrixTableHeight))
        {
            // not enough room in the skin matrix table
            return false;
        }
    }
    this->activeInstances.Add(inst);

    // assign the samples slice
    inst->samples = this->samples.MakeSlice(this->numSamples, inst->library->SampleStride);
    this->numSamples += inst->library->SampleStride;

    // assign the skin matrix slice
    if (inst->skeleton) {
        // each skeleton bones in the skin matrix table takes up 4*3 floats for a
        // transposed 4x3 matrix:
        //
        // |x0 x1 x2 x3|y0 y1 y2 y3|z0 z1 z2 z3|
        //
        // each "pixel" in the skin matrix table is 4 floats
        //
        if ((this->curSkinMatrixTableX + (inst->skeleton->NumBones*3)) > this->animSetup.SkinMatrixTableWidth) {
            // doesn't fit into current skin matrix table row, start a new row
            this->curSkinMatrixTableX = 0;
            this->curSkinMatrixTableY++;
        }
        // one 'pixel' in the skin matrix table is a vec4
        const int offset = this->curSkinMatrixTableY*this->skinMatrixTableStride + this->curSkinMatrixTableX * 4;
        inst->skinMatrices = this->skinMatrixTable.MakeSlice(offset, inst->skeleton->NumBones * 3 * 4);

        // update skinMatrixInfo
        this->skinMatrixInfo.SkinMatrixTableByteSize = (this->curSkinMatrixTableY+1)*this->skinMatrixTableStride * 4;
        auto& info = this->skinMatrixInfo.InstanceInfos.Add();
        info.Instance = inst->Id;
        const float halfPixelX = 0.5f / float(this->animSetup.SkinMatrixTableWidth);
        const float halfPixelY = 0.5f / float(this->animSetup.SkinMatrixTableHeight);
        info.ShaderInfo.x = (float(this->curSkinMatrixTableX)/float(this->animSetup.SkinMatrixTableWidth)) + halfPixelX;
        info.ShaderInfo.y = (float(this->curSkinMatrixTableY)/float(this->animSetup.SkinMatrixTableHeight)) + halfPixelY;
        info.ShaderInfo.z = float(this->animSetup.SkinMatrixTableWidth);

        // advance to next skin matrix table position
        this->curSkinMatrixTableX += inst->skeleton->NumBones * 3;
    }
    return true;
}

//------------------------------------------------------------------------------
void
animMgr::evaluate(double frameDur) {
    o_assert_dbg(this->inFrame);
    // first evaluate all active animations
    for (animInstance* inst : this->activeInstances) {
        inst->sequencer.garbageCollect(this->curTime);
        inst->sequencer.eval(inst->library, this->curTime, inst->samples.begin(), inst->samples.Size());
    }
    // compute the skinning matrices for all active instances
    for (animInstance* inst : this->activeInstances) {
        if (inst->skeleton) {
            this->genSkinMatrices(inst);
        }
    }
    this->curTime += frameDur;
    this->inFrame = false;
}

//------------------------------------------------------------------------------
void
animMgr::genSkinMatrices(animInstance* inst) {
    o_assert_dbg(inst && inst->skeleton);
    // FIXME: unwrap the glm math code
    const auto& parentIndices = inst->skeleton->ParentIndices;
    const auto& invBindPose = inst->skeleton->InvBindPose;
    const auto& smp = inst->samples;
    float tx, ty, tz;
    float sx, sy, sz;
    float qx, qy, qz, qw, qxx, qyy, qzz, qxz, qxy, qyz, qwx, qwy, qwz;
    glm::mat4 mx;
    float* m = &mx[0][0];
    for (int boneIndex=0, i=0, j=0; boneIndex<inst->skeleton->NumBones; boneIndex++) {

        // translate, rotate (from quaternion) and scale
        tx=smp[i++]; ty=smp[i++]; tz=smp[i++];
        qx=smp[i++]; qy=smp[i++]; qz=smp[i++]; qw=smp[i++];
        sx=smp[i++]; sy=smp[i++]; sz=smp[i++];
        qxx=qx*qx; qyy=qy*qy; qzz=qz*qz;
        qxz=qx*qz; qxy=qx*qy; qyz=qy*qz;
        qwx=qw*qx; qwy=qw*qy; qwz=qw*qz;
        m[0]=sx*(1.0f-2.0f*(qyy+qzz)); m[1]=sx*(2.0f*(qxy+qwz)); m[2]=sx*(2.0f*(qxz-qwy));
        m[4]=sy*(2.0f*(qxy-qwz)); m[5]=sy*(1.0f-2.0f*(qxx+qzz)); m[6]=sy*(2.0f*(qyz+qwx));
        m[8]=sz*(2.0f*(qxz+qwy)); m[9]=sz*(2.0f*(qyz-qwx)); m[10]=sz*(1.0f-2.0f*(qxx+qyy));
        m[12]=tx; m[13]=ty; m[14]=tz;

        const int16_t parentIndex = parentIndices[boneIndex];
        if (-1 != parentIndex) {
            mx = this->poseMatrices[parentIndex] * mx;
        }
        this->poseMatrices[boneIndex] = mx;

        // skin matrix is animated pose matrix multiplied by inverse bind pose matrix
        glm::mat4 skin_m = mx * invBindPose[boneIndex];

        // write xxxz, yyyy, zzzz
        inst->skinMatrices[j++] = skin_m[0][0]; // xxxx
        inst->skinMatrices[j++] = skin_m[1][0];
        inst->skinMatrices[j++] = skin_m[2][0];
        inst->skinMatrices[j++] = skin_m[3][0];

        inst->skinMatrices[j++] = skin_m[0][1]; // yyyy
        inst->skinMatrices[j++] = skin_m[1][1];
        inst->skinMatrices[j++] = skin_m[2][1];
        inst->skinMatrices[j++] = skin_m[3][1];

        inst->skinMatrices[j++] = skin_m[0][2]; // zzzz
        inst->skinMatrices[j++] = skin_m[1][2];
        inst->skinMatrices[j++] = skin_m[2][2];
        inst->skinMatrices[j++] = skin_m[3][2];
    }
}

//------------------------------------------------------------------------------
AnimJobId
animMgr::play(animInstance* inst, const AnimJob& job) {
    inst->sequencer.garbageCollect(this->curTime);
    AnimJobId jobId = ++this->curAnimJobId;
    const auto& clip = inst->library->Clips[job.ClipIndex];
    double clipDuration = clip.KeyDuration * clip.Length;
    if (inst->sequencer.add(this->curTime, jobId, job, clipDuration)) {
        return jobId;
    }
    else {
        return InvalidAnimJobId;
    }
}

//------------------------------------------------------------------------------
void
animMgr::stop(animInstance* inst, AnimJobId jobId, bool allowFadeOut) {
    inst->sequencer.stop(this->curTime, jobId, allowFadeOut);
    inst->sequencer.garbageCollect(this->curTime);
}

//------------------------------------------------------------------------------
void
animMgr::stopTrack(animInstance* inst, int trackIndex, bool allowFadeOut) {
    inst->sequencer.stopTrack(this->curTime, trackIndex, allowFadeOut);
    inst->sequencer.garbageCollect(this->curTime);
}

//------------------------------------------------------------------------------
void
animMgr::stopAll(animInstance* inst, bool allowFadeOut) {
    inst->sequencer.stopAll(this->curTime, allowFadeOut);
    inst->sequencer.garbageCollect(this->curTime);
}

} // namespace _priv
} // namespace Oryol
