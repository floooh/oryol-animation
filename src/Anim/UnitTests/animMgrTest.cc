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
    setup.MaxNumClips = 16;
    setup.MaxNumCurves = 128;
    setup.MaxNumKeys = 1024;
    setup.ResourceLabelStackCapacity = 16;
    setup.ResourceRegistryCapacity = 24;
    animMgr mgr;
    mgr.setup(setup);
    CHECK(mgr.isValid);
    CHECK(mgr.resContainer.IsValid());
    CHECK(mgr.resContainer.registry.IsValid());
    CHECK(mgr.clipPool.IsValid());
    CHECK(mgr.curvePool.Capacity() == 128);
    CHECK(mgr.maxKeys == 1024);
    CHECK(mgr.numKeys == 0);
    CHECK(mgr.keyPool != nullptr);

    // create a clip
    AnimClipSetup clipSetup;
    clipSetup.Name = "clip1";
    clipSetup.NumCurves = 3;
    clipSetup.NumKeys = 5;
    clipSetup.InitCurve = [](const AnimClip& clip, int curveIndex, AnimCurve& curve) {
        CHECK(clip.Name == "clip1");
        CHECK(clip.NumCurves == 3);
        CHECK(clip.NumKeys == 5);
        CHECK(clip.curveIndex == 0);
        CHECK(curveIndex < 3);
        switch (curveIndex) {
            case 0:
                curve.Format = AnimCurveFormat::Float3;
                curve.StaticValue[0] = 1.0f;
                curve.StaticValue[1] = 2.0f;
                curve.StaticValue[2] = 3.0f;
                curve.StaticValue[3] = 4.0f;
                break;
            case 1:
                curve.Format = AnimCurveFormat::Float2;
                curve.StaticValue[0] = 5.0f;
                curve.StaticValue[1] = 6.0f;
                curve.StaticValue[2] = 7.0f;
                curve.StaticValue[3] = 8.0f;
                break;
            case 2:
                curve.Format = AnimCurveFormat::Static;
                curve.StaticValue[0] = 9.0f;
                curve.StaticValue[1] = 10.0f;
                curve.StaticValue[2] = 11.0f;
                curve.StaticValue[3] = 12.0f;
                break;
        }
    };
    clipSetup.InitKeys = [](const AnimClip& clip, float* keyPtr, int stride, int rows) {
        // FIXME

    };
    Id clipId = mgr.createClip(clipSetup);
    CHECK(clipId.IsValid());
    AnimClip* clip = mgr.lookupClip(clipId);
    CHECK(clip);
    CHECK(clip->Name == "clip1");
    CHECK(clip->NumCurves == 3);
    CHECK(clip->NumKeys == 5);
    CHECK(clip->curveIndex == 0);
    CHECK(clip->keyStride == 5);
    CHECK(clip->keyIndex == 0);
    CHECK(clip->numPoolKeys == 25);
    CHECK(mgr.curvePool.Size() == 3);
    CHECK(mgr.curvePool[0].Format == AnimCurveFormat::Float3);
    CHECK(mgr.curvePool[1].Format == AnimCurveFormat::Float2);
    CHECK(mgr.curvePool[2].Format == AnimCurveFormat::Static);

    mgr.discard();
    CHECK(!mgr.isValid);
}