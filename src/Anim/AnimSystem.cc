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
    o_assert_dbg(nullptr == this->keyBuffer);

    this->keyBuffer = (uint8_t*) Memory::Alloc(setup.AnimKeyBufferSize);
    this->keyBufferEnd = this->keyBuffer + setup.AnimKeyBufferSize;
    this->curves.Reserve(setup.MaxAnimCurves);
    this->curves.SetAllocStrategy(0, 0);
    this->clips.Reserve(setup.MaxAnimClips);
    this->clips.SetAllocStrategy(0, 0);
    this->libs.Reserve(setup.MaxAnimLibs);
    this->libs.SetAllocStrategy(0, 0);
    this->libFreeSlots.Reserve(setup.MaxAnimLibs);
    for (uint16_t i = 0; i < setup.MaxAnimLibs; i++) {
        this->libFreeSlots.Enqueue(i);
    }
    this->isValid = true;
}

//------------------------------------------------------------------------------
void
AnimSystem::Discard() {

}

} // namespace Oryol


