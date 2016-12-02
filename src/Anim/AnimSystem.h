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
    int KeyBufferSize = 5 * 1024 * 1024;
    /// size of a sampling/mixing buffer
    int SampleBufferSize = 16 * 1024;
    /// number of sample/mixing buffers
    int NumSampleBuffers = 2;
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
    int AddKeys(const void* data, int byteSize);
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

    /// perform a sampling operation into one of the sample buffers
    void Sample(int clipIndex, int64_t time, AnimSampleMode::Code sampleMode, AnimWrapMode::Code wrapMode, int dstSampleBufferIndex);
    /// perform a mixing operation between 2 sampled clips
    void Mix(int clip0Index, int clip1Index, float weight0, float weight1, int srcSampleBufferIndex0, int srcSampleBufferIndex1, int dstSampleBufferIndex);

    /// read access to internal sample buffer
    const float* SampleBuffer(int bufferIndex) const;

private:
    AnimSystemSetup setup;
    Buffer keyBuffer;
    Buffer sampleBuffer;
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

//------------------------------------------------------------------------------
inline const float*
AnimSystem::SampleBuffer(int bufferIndex) const {
    o_assert_range_dbg(bufferIndex, this->setup.NumSampleBuffers);
    return (const float*) (this->sampleBuffer.Data() + bufferIndex * this->setup.SampleBufferSize);
}

} // namespace Oryol
