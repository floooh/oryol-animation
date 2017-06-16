//------------------------------------------------------------------------------
//  Anim.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Anim.h"
#include "Anim/private/animMgr.h"
#include "Core/Memory/Memory.h"

namespace Oryol {

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

} // namespace Oryol
