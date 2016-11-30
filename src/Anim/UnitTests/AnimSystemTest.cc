//------------------------------------------------------------------------------
//  AnimSystemTest.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "UnitTest++/src/UnitTest++.h"
#include "Anim/AnimSystem.h"
#include <array>

using namespace Oryol;

//------------------------------------------------------------------------------
TEST(AnimSytem) {

    std::array<float, 10> keys1; keys1.fill(1.0f);
    std::array<float, 20> keys2; keys2.fill(2.0f);
    std::array<float, 30> keys3; keys3.fill(3.0f);

    AnimSystem anim;
    anim.Setup();

    // fill key buffer
    const int keys1Offset = anim.KeyBufferSize();
    CHECK(keys1Offset == 0);
    anim.AddKeys((const uint8_t*)&keys1.front(), sizeof(keys1));
    const int keys2Offset = anim.KeyBufferSize();
    CHECK(keys2Offset == 40);
    anim.AddKeys((const uint8_t*)&keys2.front(), sizeof(keys2));
    const int keys3Offset = anim.KeyBufferSize();
    CHECK(keys3Offset == 120);
    anim.AddKeys((const uint8_t*)&keys3.front(), sizeof(keys3));
    CHECK(anim.KeyBufferSize() == 240);

    const float* fptr = (const float*) anim.KeyBuffer().Data();
    CHECK(fptr[0] == 1.0f);
    CHECK(fptr[1] == 1.0f);
    CHECK(fptr[9] == 1.0f);
    CHECK(fptr[10] == 2.0f);
    CHECK(fptr[11] == 2.0f);
    CHECK(fptr[29] == 2.0f);
    CHECK(fptr[40] == 3.0f);
    CHECK(fptr[41] == 3.0f);
    CHECK(fptr[59] == 3.0f);

    // add animation curves
    const int startCurveIndex = anim.NumCurves();
    CHECK(startCurveIndex == 0);
    AnimCurve curve;
    curve.IpolType = AnimIpolType::Linear;
    curve.Format = AnimKeyFormat::Float;
    curve.KeyOffset = keys1Offset;
    anim.AddCurve(curve);
    curve.KeyOffset = keys2Offset;
    anim.AddCurve(curve);
    curve.KeyOffset = keys3Offset;
    anim.AddCurve(curve);
    CHECK(anim.NumCurves() == 3);
    CHECK(anim.Curve(0).IpolType == AnimIpolType::Linear);
    CHECK(anim.Curve(0).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(0).KeyOffset == 0);
    CHECK(anim.Curve(1).IpolType == AnimIpolType::Linear);
    CHECK(anim.Curve(1).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(1).KeyOffset == 40);
    CHECK(anim.Curve(2).IpolType == AnimIpolType::Linear);
    CHECK(anim.Curve(2).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(2).KeyOffset == 120);

    // add animation clips
    const int startClipIndex = anim.NumClips();
    CHECK(startClipIndex == 0);
    AnimClip clip;
    clip.StartCurveIndex = startCurveIndex;
    clip.NumCurves = 1;
    clip.NumKeys = 10;
    clip.KeyStride = AnimKeyFormat::ByteSize(AnimKeyFormat::Float);
    anim.AddClip(clip);
    clip.StartCurveIndex = startCurveIndex + 1;
    clip.NumKeys = 20;
    anim.AddClip(clip);
    clip.StartCurveIndex = startCurveIndex + 2;
    clip.NumKeys = 30;
    anim.AddClip(clip);
    CHECK(anim.NumClips() == 3);
    CHECK(anim.Clip(0).StartCurveIndex == 0);
    CHECK(anim.Clip(0).NumCurves == 1);
    CHECK(anim.Clip(0).NumKeys == 10);
    CHECK(anim.Clip(0).KeyStride == 4);
    CHECK(anim.Clip(1).StartCurveIndex == 1);
    CHECK(anim.Clip(1).NumCurves == 1);
    CHECK(anim.Clip(1).NumKeys == 20);
    CHECK(anim.Clip(1).KeyStride == 4);
    CHECK(anim.Clip(2).StartCurveIndex == 2);
    CHECK(anim.Clip(2).NumCurves == 1);
    CHECK(anim.Clip(2).NumKeys == 30);
    CHECK(anim.Clip(2).KeyStride == 4);

    // add 2 animation libraries
    AnimLibrary lib;
    lib.StartClipIndex = startClipIndex;
    lib.NumClips = 2;
    lib.StartCurveIndex = startCurveIndex;
    lib.NumCurves = 2;
    lib.StartKeyOffset = 0;
    lib.KeyRangeSize = sizeof(keys1) + sizeof(keys2);
    lib.ClipNameMap.Add("clip0", 0);
    lib.ClipNameMap.Add("clip1", 1);
    Id lib0 = anim.AddLibrary("lib0", std::move(lib));
    CHECK(lib.ClipNameMap.Empty());

    lib.StartClipIndex = startClipIndex + 2;
    lib.NumClips = 1;
    lib.StartCurveIndex = startCurveIndex + 2;
    lib.NumCurves = 1;
    lib.StartKeyOffset = keys3Offset;
    lib.KeyRangeSize = sizeof(keys3);
    lib.ClipNameMap.Add("clip0", 0);
    Id lib1 = anim.AddLibrary("lib1", std::move(lib));

    CHECK(anim.LookupLibrary("lib0") == lib0);
    CHECK(anim.LookupLibrary("lib1") == lib1);
    CHECK(!anim.LookupLibrary("lib2").IsValid());

    CHECK(anim.Library(lib0) != nullptr);
    CHECK(anim.Library(lib1) != nullptr);
    CHECK(anim.Library(lib0)->id == lib0);
    CHECK(anim.Library(lib1)->id == lib1);

    CHECK(anim.Library(lib0)->StartClipIndex == 0);
    CHECK(anim.Library(lib0)->NumClips == 2);
    CHECK(anim.Library(lib0)->StartCurveIndex == 0);
    CHECK(anim.Library(lib0)->NumCurves == 2);
    CHECK(anim.Library(lib0)->StartKeyOffset == 0);
    CHECK(anim.Library(lib0)->KeyRangeSize == 120);
    CHECK(anim.Library(lib0)->ClipNameMap.Contains("clip0"));
    CHECK(anim.Library(lib0)->ClipNameMap.Contains("clip1"));
    CHECK(anim.Library(lib0)->ClipNameMap["clip0"] == 0);
    CHECK(anim.Library(lib0)->ClipNameMap["clip1"] == 1);
    CHECK(anim.Library(lib0)->id == lib0);
    CHECK(anim.Library(lib0)->name == "lib0");

    CHECK(anim.Library(lib1)->StartClipIndex == 2);
    CHECK(anim.Library(lib1)->NumClips == 1);
    CHECK(anim.Library(lib1)->StartCurveIndex == 2);
    CHECK(anim.Library(lib1)->NumCurves == 1);
    CHECK(anim.Library(lib1)->StartKeyOffset == 120);
    CHECK(anim.Library(lib1)->KeyRangeSize == 120);
    CHECK(anim.Library(lib1)->ClipNameMap.Contains("clip0"));
    CHECK(anim.Library(lib1)->ClipNameMap["clip0"] == 0);
    CHECK(anim.Library(lib1)->id == lib1);
    CHECK(anim.Library(lib1)->name == "lib1");

    // remove first lib and check if data has been compacted correctly
    anim.RemLibrary(lib0);
    CHECK(!anim.LookupLibrary("lib0").IsValid());
    CHECK(anim.LookupLibrary("lib1") == lib1);
    CHECK(anim.KeyBufferSize() == sizeof(keys3));
    CHECK(anim.NumCurves() == 1);
    CHECK(anim.NumClips() == 1);
    CHECK(anim.Curve(0).KeyOffset == 0);
    CHECK(anim.Curve(0).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(0).IpolType == AnimIpolType::Linear);
    CHECK(anim.Clip(0).StartCurveIndex == 0);
    CHECK(anim.Clip(0).NumCurves == 1);
    CHECK(anim.Clip(0).NumKeys == 30);
    CHECK(anim.Clip(0).KeyStride == 4);
    CHECK(anim.Library(lib1)->StartClipIndex == 0);
    CHECK(anim.Library(lib1)->NumClips == 1);
    CHECK(anim.Library(lib1)->StartCurveIndex == 0);
    CHECK(anim.Library(lib1)->NumCurves == 1);
    CHECK(anim.Library(lib1)->StartKeyOffset == 0);
    CHECK(anim.Library(lib1)->KeyRangeSize == 120);
    CHECK(anim.Library(lib1)->ClipNameMap.Contains("clip0"));
    CHECK(anim.Library(lib1)->ClipNameMap["clip0"] == 0);
    CHECK(anim.Library(lib1)->id == lib1);
    CHECK(anim.Library(lib1)->name == "lib1");
}

