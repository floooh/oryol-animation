//------------------------------------------------------------------------------
//  Anim.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Anim.h"
#include "Anim/private/animMgr.h"
#include "Core/Memory/Memory.h"

namespace Oryol {

using namespace _priv;

namespace {
    struct _state {
        struct AnimSetup animSetup;
        _priv::animMgr mgr;    
    };
    _state* state = nullptr;
}

//------------------------------------------------------------------------------
void
Anim::Setup(const struct AnimSetup& setup) {
    o_assert_dbg(!IsValid());
    state = Memory::New<_state>();
    state->animSetup = setup;
    state->mgr.setup(setup);
}

//------------------------------------------------------------------------------
void
Anim::Discard() {
    o_assert_dbg(IsValid());
    state->mgr.discard();
    Memory::Delete(state);
    state = nullptr;
}

//------------------------------------------------------------------------------
bool
Anim::IsValid() {
    return (nullptr != state);
}

//------------------------------------------------------------------------------
ResourceLabel
Anim::PushLabel() {
    o_assert_dbg(IsValid());
    return state->mgr.resContainer.PushLabel();
}

//------------------------------------------------------------------------------
void
Anim::PushLabel(ResourceLabel label) {
    o_assert_dbg(IsValid());
    state->mgr.resContainer.PushLabel(label);
}

//------------------------------------------------------------------------------
ResourceLabel
Anim::PopLabel() {
    o_assert_dbg(IsValid());
    return state->mgr.resContainer.PopLabel();
}

//------------------------------------------------------------------------------
template<> Id
Anim::Create(const AnimLibrarySetup& setup) {
    o_assert_dbg(IsValid());
    return state->mgr.createLibrary(setup);
}

//------------------------------------------------------------------------------
template<> Id
Anim::Create(const AnimSkeletonSetup& setup) {
    o_assert_dbg(IsValid());
    return state->mgr.createSkeleton(setup);
}

//------------------------------------------------------------------------------
template<> Id
Anim::Create(const AnimInstanceSetup& setup) {
    o_assert_dbg(IsValid());
    return state->mgr.createInstance(setup);
}

//------------------------------------------------------------------------------
Id
Anim::Lookup(const Locator& name) {
    o_assert_dbg(IsValid());
    return state->mgr.resContainer.Lookup(name);
}

//------------------------------------------------------------------------------
void
Anim::Destroy(ResourceLabel label) {
    o_assert_dbg(IsValid());
    return state->mgr.destroy(label);
}

//------------------------------------------------------------------------------
bool
Anim::HasLibrary(const Id& libId) {
    o_assert_dbg(IsValid());
    return nullptr != state->mgr.lookupLibrary(libId);
}

//------------------------------------------------------------------------------
const AnimLibrary&
Anim::Library(const Id& libId) {
    o_assert_dbg(IsValid());
    const AnimLibrary* lib = state->mgr.lookupLibrary(libId);
    if (lib) {
        return *lib;
    }
    else {
        static AnimLibrary dummyLib;
        return dummyLib;
    }
}

//------------------------------------------------------------------------------
void
Anim::WriteKeys(const Id& libId, const uint8_t* ptr, int numBytes) {
    o_assert_dbg(IsValid());
    AnimLibrary* lib = state->mgr.lookupLibrary(libId);
    if (lib) {
        state->mgr.writeKeys(lib, ptr, numBytes);
    }
    else {
        o_warn("Anim::WriteKeys: invalid anim lib id\n");
    }
}

//------------------------------------------------------------------------------
bool
Anim::HasSkeleton(const Id& skelId) {
    o_assert_dbg(IsValid());
    return nullptr != state->mgr.lookupSkeleton(skelId);
}

//------------------------------------------------------------------------------
const AnimSkeleton&
Anim::Skeleton(const Id& skelId) {
    o_assert_dbg(IsValid());
    const AnimSkeleton* skel = state->mgr.lookupSkeleton(skelId);
    if (skel) {
        return *skel;
    }
    else {
        static AnimSkeleton dummySkel;
        return dummySkel;
    }
}

//------------------------------------------------------------------------------
void
Anim::NewFrame() {
    o_assert_dbg(IsValid());
    state->mgr.newFrame();
}

//------------------------------------------------------------------------------
bool
Anim::AddActiveInstance(const Id& instId) {
    o_assert_dbg(IsValid());
    animInstance* inst = state->mgr.lookupInstance(instId);
    if (inst) {
        return state->mgr.addActiveInstance(inst);
    }
    else {
        return false;
    }
}

//------------------------------------------------------------------------------
void
Anim::Evaluate(double frameDurationInSeconds) {
    o_assert_dbg(IsValid());
    state->mgr.evaluate(frameDurationInSeconds);
}

//------------------------------------------------------------------------------
AnimJobId
Anim::Play(const Id& instId, const AnimJob& job) {
    o_assert_dbg(IsValid());
    animInstance* inst = state->mgr.lookupInstance(instId);
    if (inst) {
        return state->mgr.play(inst, job);
    }
    else {
        return InvalidAnimJobId;
    }
}

//------------------------------------------------------------------------------
void
Anim::Stop(const Id& instId, AnimJobId jobId, bool allowFadeOut) {
    o_assert_dbg(IsValid());
    animInstance* inst = state->mgr.lookupInstance(instId);
    if (inst) {
        state->mgr.stop(inst, jobId, allowFadeOut);
    }
}

//------------------------------------------------------------------------------
void
Anim::StopTrack(const Id& instId, int trackIndex, bool allowFadeOut) {
    o_assert_dbg(IsValid());
    animInstance* inst = state->mgr.lookupInstance(instId);
    if (inst) {
        state->mgr.stopTrack(inst, trackIndex, allowFadeOut);
    }
}

//------------------------------------------------------------------------------
void
Anim::StopAll(const Id& instId, bool allowFadeOut) {
    o_assert_dbg(IsValid());
    animInstance* inst = state->mgr.lookupInstance(instId);
    if (inst) {
        state->mgr.stopAll(inst, allowFadeOut);
    }
}

} // namespace Oryol
