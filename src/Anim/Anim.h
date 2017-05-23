#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::Anim
    @ingroup Anim
    @brief animation system facade
*/
#include "Anim/AnimTypes.h"
#include "Resource/ResourceLabel.h"

namespace Oryol {

class Anim {
public:
    /// setup the animation module
    static void Setup(const AnimSetup& setup);
    /// discard the animation module
    static void Discard();
    /// check if animation module is setup
    static bool IsValid() const;

    /// get the original AnimSetup object
    static const class AnimSetup& AnimSetup();

    /// generate new resource label and push on label stack
    static ResourceLabel PushResourceLabel();
    /// push explicit resource label on label stack
    static void PushResourceLabel(ResourceLabel label);
    /// pop resource label from label stack
    static ResourceLabel PopResourceLabel();

    /// create a resource object without associated data
    template<class SETUP> static Id CreateResource(const SETUP& setup);
    /// create a resource object with associated data
    template<class SETUP> static Id CreateResource(const SetupAndData<SETUP>& setupAndData);
    /// create a resource object with associated data
    template<class SETUP> static Id CreateResource(const SETUP& setup, const Buffer& data);
    /// create a resource object with raw pointer to associated data
    template<class SETUP> static Id CreateResource(const SETUP& setup, const void* data, int size);
    /// lookup a resource Id by Locator
    static Id LookupResource(const Locator& locator);
    /// destroy one or several resources by matching label
    static void DestroyResources(ResourceLabel label);
};



} // namespace Oryol

