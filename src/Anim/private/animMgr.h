#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::animMgr
    @ingroup _priv
    @brief resource container of the Anim module
*/
#include "Resource/ResourceContainerBase.h"

namespace Oryol {
namespace _priv {

class animMgr {
public:
    /// setup the anim mgr
    void setup(const AnimSetup& setup);
    /// discard the anim mgr
    void discard();

    /// create an anim resource object
    template<class SETUP> create(const AnimClipSetup& setup, const void* data=nullptr, int size=0);
    /// immediately destroy anim resources by label
    void destroy(const ResourceLabel& label);

    /// lookup animClip object 
    animClip* lookupClip(const Id& resId);

    ResourceContainerBase resContainer;
}

} // namespace _priv
} // namespace Oryol
