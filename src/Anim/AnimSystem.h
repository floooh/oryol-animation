#pragma once
//------------------------------------------------------------------------------
#include "Anim/Types.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/String/StringAtom.h"
#include "Resource/Id.h"

namespace Oryol {

//------------------------------------------------------------------------------
/**
    @class Oryol::AnimSystemSetup
    @ingroup Anim
    @brief setup parameters for an AnimSystem object
*/
class AnimSystemSetup {
public:
    /// size of global anim key buffer (in bytes)
    int AnimKeyBufferSize = 5 * 1024 * 1024;
    /// max number of AnimLibrary objects
    int MaxAnimLibs = 16;
    /// max number of AnimClip objects
    int MaxAnimClips = MaxAnimLibs * 16;
    /// max number of AnimCurve objects
    int MaxAnimCurves = MaxAnimClips * 128;
};

//------------------------------------------------------------------------------
/** 
    @defgroup Anim Anim
    @brief animation helper classes

    @class Oryol::AnimSystem
    @ingroup Anim
    @brief a collection of animation resources
*/
class AnimSystem {
public:
    /// destructor
    ~AnimSystem();

    /// setup the AnimSystem object
    void Setup(const AnimSystemSetup& setup);
    /// discard the AnimSystem object
    void Discard();
    /// return true if the AnimSystem object has been setup
    bool IsValid() const;

    /// add key data to key buffer, return byte offset of first key
    int AddKeys(const void* data, int size);
    /// add AnimCurve object and return it's slot index
    int AddCurve(const AnimCurve& curve);
    /// add AnimClip object and return it's slot index
    int AddClip(const AnimClip& clip);

    /// add AnimLibrary object and return it's slot index
    Id AddLibrary(const StringAtom& name, const AnimLibrary& lib);
    /// remove library by Id (also removes its keys, curves and clips)
    void RemLibrary(const Id& id);
    /// find anim library index by name, return InvalidIndex if not found
    Id LookupLibrary(const StringAtom& name);
    /// get pointer to library by Id (may return nullptr!)
    AnimLibrary* Library(const Id& id) const;

private:
    uint8_t* keyBuffer = nullptr;
    const uint8_t* keyBufferEnd = nullptr;
    Array<AnimCurve> curves;
    Array<AnimClip> clips;
    Array<AnimLibrary> libs;
    Queue<uint16_t> libFreeSlots;
    bool isValid = false;
};

} // namespace Oryol



