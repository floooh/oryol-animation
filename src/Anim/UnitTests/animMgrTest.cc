//------------------------------------------------------------------------------
//  animMgrTest.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "UnitTest++/src/UnitTest++.h"
#include "Anim/AnimTypes.h"
#include "Anim/private/animMgr.h"

using namespace Oryol;
using namespace Oryol::_priv;

TEST(animMgr) {

    // setup
    AnimSetup setup;
    setup.MaxNumLibs = 4;
    setup.MaxNumClips = 16;
    setup.MaxNumCurves = 128;
    setup.MaxNumKeys = 1024;
    setup.MaxNumSamples = 256;
    setup.ResourceLabelStackCapacity = 16;
    setup.ResourceRegistryCapacity = 24;
    animMgr mgr;
    mgr.setup(setup);
    CHECK(mgr.isValid);
    CHECK(mgr.resContainer.IsValid());
    CHECK(mgr.resContainer.registry.IsValid());
    CHECK(mgr.libPool.IsValid());
    CHECK(mgr.clipPool.Capacity() == 16);
    CHECK(mgr.curvePool.Capacity() == 128);
    CHECK(mgr.keys.Size() == 1024);
    CHECK(mgr.samples.Size() == 256);
    CHECK(mgr.numKeys == 0);
    CHECK(mgr.numSamples == 0);
    CHECK(mgr.valuePool != nullptr);

    // create a library with two clips
    const int numCurves = 3;
    AnimCurveFormat::Enum curveLayout[numCurves] = {
        AnimCurveFormat::Float2,
        AnimCurveFormat::Float3,
        AnimCurveFormat::Float4,
    };
    AnimCurveSetup clip1Curves[numCurves] = {
        { false, 1.0f, 2.0f, 3.0f, 4.0f },
        { false, 5.0f, 6.0f, 7.0f, 8.0f },
        { true,  9.0f, 10.0f, 11.0f, 12.0f }
    };
    AnimCurveSetup clip2Curves[numCurves] = {
        { true, 4.0f, 3.0f, 2.0f, 1.0f },
        { false, 8.0f, 7.0f, 6.0f, 5.0f },
        { true,  12.0f, 11.0f, 10.0f, 9.0f }
    };
    static const int numClips = 2;
    AnimClipSetup clips[numClips] = {
        { "clip1", 10, 0.04f, ArrayView<AnimCurveSetup>(clip1Curves, 0, numCurves) },
        { "clip2", 20, 0.04f, ArrayView<AnimCurveSetup>(clip2Curves, 0, numCurves) },
    };
    AnimLibrarySetup libSetup;
    libSetup.Name = "human";
    libSetup.CurveLayout = ArrayView<AnimCurveFormat::Enum>(curveLayout, 0, numCurves);
    libSetup.Clips = ArrayView<AnimClipSetup>(clips, 0, numClips);

    ResourceLabel l1 = mgr.resContainer.PushLabel();
    Id lib1 = mgr.createLibrary(libSetup);
    mgr.resContainer.PopLabel();
    CHECK(mgr.lookupLibrary(lib1) != nullptr);
    CHECK(mgr.libPool.QueryPoolInfo().NumUsedSlots == 1);
    CHECK(mgr.clipPool.Size() == 2);
    CHECK(mgr.curvePool.Size() == 6);
    CHECK(mgr.numKeys == 110);
    libSetup.Name = "Bla";
    Id lib2 = mgr.createLibrary(libSetup);
    CHECK(mgr.lookupLibrary(lib2) != nullptr);
    CHECK(mgr.libPool.QueryPoolInfo().NumUsedSlots == 2);
    CHECK(mgr.clipPool.Size() == 4);
    CHECK(mgr.curvePool.Size() == 12);
    CHECK(mgr.numKeys == 220);
    
    mgr.destroy(l1);
    CHECK(mgr.libPool.QueryPoolInfo().NumUsedSlots == 1);
    CHECK(mgr.clipPool.Size() == 2);
    CHECK(mgr.curvePool.Size() == 6);
    CHECK(mgr.numKeys == 110);

    mgr.discard();
    CHECK(!mgr.isValid);
    CHECK(mgr.clipPool.Size() == 0);
    CHECK(mgr.curvePool.Size() == 0);
    CHECK(mgr.numKeys == 0);
}