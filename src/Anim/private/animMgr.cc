//------------------------------------------------------------------------------
//  animMgr.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "animMgr.h"
#include "Core/Memory/Memory.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstring>

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
    this->clipPool.SetFixedCapacity(setup.ClipPoolCapacity);
    this->curvePool.SetFixedCapacity(setup.CurvePoolCapacity);
    this->matrixPool.SetFixedCapacity(setup.MatrixPoolCapacity);
    this->activeInstances.SetFixedCapacity(setup.MaxNumActiveInstances);
    this->skinMatrixInfo.InstanceInfos.SetFixedCapacity(setup.MaxNumActiveInstances);
    this->keyPool = (int16_t*) Memory::Alloc(setup.KeyPoolCapacity * sizeof(int16_t));
    this->samplePool = (float*) Memory::Alloc(setup.SamplePoolCapacity * sizeof(float));
    this->keys = Slice<int16_t>(this->keyPool, setup.KeyPoolCapacity, 0, setup.KeyPoolCapacity);
    this->samples = Slice<float>(this->samplePool, setup.SamplePoolCapacity, 0, setup.SamplePoolCapacity);
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
    o_assert_dbg(this->keyPool);
    o_assert_dbg(this->samplePool);
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
    Memory::Free(this->keyPool);
    this->keyPool = nullptr;
    Memory::Free(this->samplePool);
    this->samplePool = nullptr;
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
    o_assert_dbg(libSetup.Locator.HasValidLocation());
    o_assert_dbg(!libSetup.CurveLayout.Empty());
    o_assert_dbg(!libSetup.Clips.Empty());

    // check if lib already exists
    Id resId = this->resContainer.registry.Lookup(libSetup.Locator);
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
    lib.Locator = libSetup.Locator;
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
            for (int i = 0; i < 4; i++) {
                curve.StaticValue[i] = curveSetup.StaticValue[i];
                // premultiply magnitude for 16-bit signed unpacking
                curve.Magnitude[i] = curveSetup.Magnitude[i] / 32767.0f;
            }
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
    /*
    FIXME FIXME FIXME
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
    */

    this->resContainer.registry.Add(libSetup.Locator, resId, this->resContainer.PeekLabel());
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
    o_assert_dbg(setup.Locator.HasValidLocation());
    o_assert_dbg(!setup.Bones.Empty());

    // check if skeleton already exists
    Id resId = this->resContainer.registry.Lookup(setup.Locator);
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
    skel.Locator = setup.Locator;
    skel.NumBones = setup.Bones.Size();
    const int matrixPoolIndex = this->matrixPool.Size();
    for (const auto& bone : setup.Bones) {
        this->matrixPool.Add(glm::mat4x3(bone.BindPose));
    }
    for (const auto& bone : setup.Bones) {
        this->matrixPool.Add(glm::mat4x3(bone.InvBindPose));
    }
    skel.Matrices = this->matrixPool.MakeSlice(matrixPoolIndex, skel.NumBones * 2);
    skel.BindPose = skel.Matrices.MakeSlice(0, skel.NumBones);
    skel.InvBindPose = skel.Matrices.MakeSlice(skel.NumBones, skel.NumBones);
    for (int i = 0; i < skel.NumBones; i++) {
        skel.ParentIndices[i] = setup.Bones[i].ParentIndex;
    }

    // register the new resource, and done
    this->resContainer.registry.Add(setup.Locator, resId, this->resContainer.PeekLabel());
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
animMgr::removeKeys(Slice<int16_t> range) {
    o_assert_dbg(this->keyPool);
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
animMgr::removeMatrices(Slice<glm::mat4x3> range) {
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
    o_assert_dbg(lib->Keys.Size()*sizeof(int16_t) == numBytes);
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
    // garbage-collect anim jobs in all active instances
    for (animInstance* inst : this->activeInstances) {
        inst->sequencer.garbageCollect(this->curTime);
    }
    // evaluate animation of all active instances
    for (animInstance* inst : this->activeInstances) {
        inst->sequencer.eval(inst->library, this->curTime, inst->samples.begin(), inst->samples.Size());
    }
    // compute the skinning matrices for all active instances (which have skeletons)
    for (animInstance* inst : this->activeInstances) {
        if (inst->skeleton) {
            this->genSkinMatrices(inst);
        }
    }
    this->curTime += frameDur;
    this->inFrame = false;
}

//------------------------------------------------------------------------------
static void
mx_mul4x3(const float* m1, const float* m2, float* m) {
    m[0]  = m1[0]*m2[0] + m1[3]*m2[1] + m1[6] *m2[2];
    m[1]  = m1[1]*m2[0] + m1[4]*m2[1] + m1[7] *m2[2];
    m[2]  = m1[2]*m2[0] + m1[5]*m2[1] + m1[8] *m2[2];
    m[3]  = m1[0]*m2[3] + m1[3]*m2[4] + m1[6] *m2[5];
    m[4]  = m1[1]*m2[3] + m1[4]*m2[4] + m1[7] *m2[5];
    m[5]  = m1[2]*m2[3] + m1[5]*m2[4] + m1[8] *m2[5];
    m[6]  = m1[0]*m2[6] + m1[3]*m2[7] + m1[6] *m2[8];
    m[7]  = m1[1]*m2[6] + m1[4]*m2[7] + m1[7] *m2[8];
    m[8]  = m1[2]*m2[6] + m1[5]*m2[7] + m1[8] *m2[8];
    m[9]  = m1[0]*m2[9] + m1[3]*m2[10] + m1[6] *m2[11] + m1[9];
    m[10] = m1[1]*m2[9] + m1[4]*m2[10] + m1[7] *m2[11] + m1[10];
    m[11] = m1[2]*m2[9] + m1[5]*m2[10] + m1[8] *m2[11] + m1[11];
}

//------------------------------------------------------------------------------
static void
mx_mul4x3_transpose(const float* m1, const float* m2, float* m) {
    m[0]  = m1[0]*m2[0] + m1[3]*m2[1]  + m1[6] *m2[2];
    m[1]  = m1[0]*m2[3] + m1[3]*m2[4]  + m1[6] *m2[5];
    m[2]  = m1[0]*m2[6] + m1[3]*m2[7]  + m1[6] *m2[8];
    m[3]  = m1[0]*m2[9] + m1[3]*m2[10] + m1[6] *m2[11] + m1[9];
    m[4]  = m1[1]*m2[0] + m1[4]*m2[1]  + m1[7] *m2[2];
    m[5]  = m1[1]*m2[3] + m1[4]*m2[4]  + m1[7] *m2[5];
    m[6]  = m1[1]*m2[6] + m1[4]*m2[7]  + m1[7] *m2[8];
    m[7]  = m1[1]*m2[9] + m1[4]*m2[10] + m1[7] *m2[11] + m1[10];
    m[8]  = m1[2]*m2[0] + m1[5]*m2[1]  + m1[8] *m2[2];
    m[9]  = m1[2]*m2[3] + m1[5]*m2[4]  + m1[8] *m2[5];
    m[10] = m1[2]*m2[6] + m1[5]*m2[7]  + m1[8] *m2[8];
    m[11] = m1[2]*m2[9] + m1[5]*m2[10] + m1[8] *m2[11] + m1[11];
}

//------------------------------------------------------------------------------
static void
mx_copy(const float* src, float* dst) {
    for (int i = 0; i < 12; i++) {
        dst[i] = src[i];
    }
}

//------------------------------------------------------------------------------
void
animMgr::genSkinMatrices(animInstance* inst) {
    o_assert_dbg(inst && inst->skeleton);
    const int32_t* parentIndices = &inst->skeleton->ParentIndices[0];
    // pointer to skeleton's inverse bind pose matrices
    const float* invBindPose = &(inst->skeleton->InvBindPose[0][0][0]);
    // output are transposed 4x3 matrices ready for upload to GPU
    float* outSkinMatrices = &(inst->skinMatrices[0]);
    // input samples (result of animation evaluation)
    const float* smp = &(inst->samples[0]);

    float m0[12], m1[12];
    float tmpBoneMatrices[AnimConfig::MaxNumSkeletonBones][12];
    const int numBones = inst->skeleton->NumBones;
    for (int boneIndex=0; boneIndex<numBones; boneIndex++, smp+=10, outSkinMatrices+=12) {

        // samples bone translate, rotate (quat), scale to matrix
        float tx=smp[0]; float ty=smp[1]; float tz=smp[2];
        float qx=smp[3]; float qy=smp[4]; float qz=smp[5]; float qw=smp[6];
        float sx=smp[7]; float sy=smp[8]; float sz=smp[9];
        float qxx=qx*qx; float qyy=qy*qy; float qzz=qz*qz;
        float qxz=qx*qz; float qxy=qx*qy; float qyz=qy*qz;
        float qwx=qw*qx; float qwy=qw*qy; float qwz=qw*qz;
        m0[0]=sx*(1.0f-2.0f*(qyy+qzz)); m0[1]=sx*(2.0f*(qxy+qwz));      m0[2]=sx*(2.0f*(qxz-qwy));
        m0[3]=sy*(2.0f*(qxy-qwz));      m0[4]=sy*(1.0f-2.0f*(qxx+qzz)); m0[5]=sy*(2.0f*(qyz+qwx));
        m0[6]=sz*(2.0f*(qxz+qwy));      m0[7]=sz*(2.0f*(qyz-qwx));      m0[8]=sz*(1.0f-2.0f*(qxx+qyy));
        m0[9]=tx;                       m0[10]=ty;                      m0[11]=tz;

        // multiply with parent bone matrix
        const int32_t parentIndex = parentIndices[boneIndex];
        const float* m;
        if (-1 != parentIndex) {
            mx_mul4x3(&tmpBoneMatrices[parentIndex][0], m0, m1);
            m = m1;
        }
        else {
            m = m0;
        }
        mx_copy(m, &tmpBoneMatrices[boneIndex][0]);

        // multiply with inverse bind pose matrix into transposed skin matrix
        mx_mul4x3_transpose(m, &invBindPose[boneIndex * 12], outSkinMatrices);
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
