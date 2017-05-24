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
    static ResourceLabel PushLabel();
    /// push explicit resource label on label stack
    static void PushLabel(ResourceLabel label);
    /// pop resource label from label stack
    static ResourceLabel PopLabel();

    /// create an anim clip object
    static Id CreateClip(const AnimClipSetup& clip);
    /// lookup a clip id by it's name
    static Id LookupClip(const Locator& name);
    /// destroy one or several clips by matching resource label
    static void DestroyClips(ResourceLabel label);

    /// access an AnimClip 
    static const AnimClip& Clip(Id clip) const;
    /// access an AnimCurve
    static const AnimCurve& Curve(Id clip, int curveIndex) const;

};



} // namespace Oryol

