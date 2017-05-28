#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::Anim
    @ingroup Anim
    @brief animation system facade
*/
#include "Anim/AnimTypes.h"
#include "Resource/ResourceLabel.h"
#include "Resource/Locator.h"

namespace Oryol {

class Anim {
public:
    /// setup the animation module
    static void Setup(const AnimSetup& setup);
    /// discard the animation module
    static void Discard();
    /// check if animation module is setup
    static bool IsValid();
    /// get the original AnimSetup object
    static const struct AnimSetup& AnimSetup();

    /// generate new resource label and push on label stack
    static ResourceLabel PushLabel();
    /// push explicit resource label on label stack
    static void PushLabel(ResourceLabel label);
    /// pop resource label from label stack
    static ResourceLabel PopLabel();

    /// create an anim resource object
    template<class SETUP> static Id Create(const SETUP& setup);
    /// lookup an resource id by name 
    static Id Lookup(const Locator& name);
    /// destroy one or several anim resources by label
    static void Destroy(ResourceLabel label);

    /// access an animation library
    static const AnimLibrary& Library(Id id);
    /// lookup a clip index by name
    static int ClipIndex(Id libId, const StringAtom& clipName);
};

} // namespace Oryol

