#pragma once
//------------------------------------------------------------------------------
/**
    @defgroup Anim Anim
    @brief generic animation system
    
    @class Oryol::Anim
    @ingroup Anim
    @brief animation system facade
*/
#include "Anim/Core/AnimSetup.h"
#include "Resource/ResourceLabel.h"
#include "Resource/Id.h"
#include "Resource/Core/SetupAndData.h"

namespace Oryol {

class Anim {
public:
    /// setup the Anim module
    static void Setup(const AnimSetup& setup);
    /// discard the Anim module
    static void Discard();
    /// check if Anim module has been setup
    static bool IsValid();

    /// get the AnimSetup parameters
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

    
};

} // namespace Oryol


    
