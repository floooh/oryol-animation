#pragma once
//------------------------------------------------------------------------------
#include "Anim/Types.h"
#include "Core/Containers/Buffer.h"
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
    void Setup(const AnimSystemSetup& setup = AnimSystemSetup());
    /// discard the AnimSystem object
    void Discard();
    /// return true if the AnimSystem object has been setup
    bool IsValid() const;

    /// get current key buffer size in number of bytes
    int KeyBufferSize() const;
    /// access to keybuffer
    const Buffer& KeyBuffer() const;
    /// get number of anim curves
    int NumCurves() const;
    /// get anim curve at index
    const AnimCurve& Curve(int index) const;
    /// get number of anim clips
    int NumClips() const;
    /// get anim clip at index
    const AnimClip& Clip(int index) const;

    /// add key data to key buffer, return byte offset of first key
    void AddKeys(const uint8_t* data, int byteSize);
    /// add AnimCurve object and return it's slot index
    void AddCurve(const AnimCurve& curve);
    /// add AnimClip object and return it's slot index
    void AddClip(const AnimClip& clip);

    /// add AnimLibrary object and return it's slot index
    Id AddLibrary(const StringAtom& name, AnimLibrary&& lib);
    /// find anim library index by name, return InvalidIndex if not found
    Id LookupLibrary(const StringAtom& name);
    /// get pointer to library by Id (may return nullptr!)
    const AnimLibrary* Library(const Id& id) const;
    /// remove library by Id (also removes its keys, curves and clips)
    void RemLibrary(const Id& id);

private:
    Buffer keyBuffer;
    Array<AnimCurve> curves;
    Array<AnimClip> clips;
    Array<AnimLibrary> libs;
    Map<StringAtom, Id> libNameMap;
    Queue<uint16_t> libFreeSlots;
    Id::UniqueStampT uniqueStamp = 0;
    bool isValid = false;
};

//------------------------------------------------------------------------------
inline int
AnimSystem::KeyBufferSize() const {
    return this->keyBuffer.Size();
}

//------------------------------------------------------------------------------
inline int
AnimSystem::NumCurves() const {
    return this->curves.Size();
}

//------------------------------------------------------------------------------
inline const AnimCurve&
AnimSystem::Curve(int index) const {
    return this->curves[index];
}

//------------------------------------------------------------------------------
inline int
AnimSystem::NumClips() const {
    return this->clips.Size();
}

//------------------------------------------------------------------------------
inline const AnimClip&
AnimSystem::Clip(int index) const {
    return this->clips[index];
}

//------------------------------------------------------------------------------
inline const Buffer&
AnimSystem::KeyBuffer() const {
    return this->keyBuffer;
}

} // namespace Oryol
