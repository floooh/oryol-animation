//------------------------------------------------------------------------------
//  AnimSystemTest.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "UnitTest++/src/UnitTest++.h"
#include "Anim/AnimSystem.h"
#include <array>
#include "glm/glm.hpp"
#include "glm/gtc/packing.hpp"

using namespace Oryol;

//------------------------------------------------------------------------------
TEST(AnimSystem_AddRemove) {

    std::array<float, 10> keys1; keys1.fill(1.0f);
    std::array<float, 20> keys2; keys2.fill(2.0f);
    std::array<float, 30> keys3; keys3.fill(3.0f);

    AnimSystem anim;
    anim.Setup();

    // fill key buffer
    const int keys1Offset = anim.AddKeys(keys1.begin(), sizeof(keys1));
    CHECK(keys1Offset == 0);
    const int keys2Offset = anim.AddKeys(keys2.begin(), sizeof(keys2));
    CHECK(keys2Offset == 40);
    const int keys3Offset = anim.AddKeys(keys3.begin(), sizeof(keys3));
    CHECK(keys3Offset == 120);
    CHECK(anim.KeyBuffer().Size() == 240);

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
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
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
    CHECK(anim.Curve(0).KeyStride == 4);
    CHECK(anim.Curve(1).IpolType == AnimIpolType::Linear);
    CHECK(anim.Curve(1).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(1).KeyOffset == 40);
    CHECK(anim.Curve(1).KeyStride == 4);
    CHECK(anim.Curve(2).IpolType == AnimIpolType::Linear);
    CHECK(anim.Curve(2).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(2).KeyOffset == 120);
    CHECK(anim.Curve(2).KeyStride == 4);

    // add animation clips
    const int startClipIndex = anim.NumClips();
    CHECK(startClipIndex == 0);
    AnimClip clip;
    clip.StartCurveIndex = startCurveIndex;
    clip.NumCurves = 1;
    clip.NumKeys = 10;
    clip.KeyDuration = 1000000 / 25;
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
    CHECK(anim.Clip(1).StartCurveIndex == 1);
    CHECK(anim.Clip(1).NumCurves == 1);
    CHECK(anim.Clip(1).NumKeys == 20);
    CHECK(anim.Clip(2).StartCurveIndex == 2);
    CHECK(anim.Clip(2).NumCurves == 1);
    CHECK(anim.Clip(2).NumKeys == 30);

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
    CHECK(anim.KeyBuffer().Size() == sizeof(keys3));
    CHECK(anim.NumCurves() == 1);
    CHECK(anim.NumClips() == 1);
    CHECK(anim.Curve(0).KeyOffset == 0);
    CHECK(anim.Curve(0).Format == AnimKeyFormat::Float);
    CHECK(anim.Curve(0).KeyStride == 4);
    CHECK(anim.Curve(0).IpolType == AnimIpolType::Linear);
    CHECK(anim.Clip(0).StartCurveIndex == 0);
    CHECK(anim.Clip(0).NumCurves == 1);
    CHECK(anim.Clip(0).NumKeys == 30);
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

//------------------------------------------------------------------------------
TEST(AnimSystem_Sample) {
    AnimSystem anim;
    anim.Setup();

    static const int numKeys = 10;
    std::array<float, numKeys> keysFloat;
    std::array<glm::vec2, numKeys> keysFloat2;
    std::array<glm::vec3, numKeys> keysFloat3;
    std::array<glm::vec4, numKeys> keysFloat4;
    std::array<uint32_t, numKeys> keysByte4N;
    std::array<uint32_t, numKeys> keysShort2N;
    std::array<uint64_t, numKeys> keysShort4N;

    for (int i = 0; i < numKeys; i++) {
        keysFloat[i] = float(i);
        keysFloat2[i] = glm::vec2(i*2.0f, i*3.0f);
        keysFloat3[i] = glm::vec3(i*4.0f, i*5.0f, i*6.0f);
        keysFloat4[i] = glm::vec4(i*7.0f, i*8.0f, i*9.0f, i*10.0f);
        keysByte4N[i] = glm::packSnorm4x8(glm::vec4(i*11.0f, i*12.0f, i*13.0f, i*14.0f));
        keysShort2N[i] = glm::packSnorm2x16(glm::vec2(i*15.0f, i*16.0f));
        keysShort4N[i] = glm::packSnorm4x16(glm::vec4(i*17.0f, i*18.0f, i*19.0f, i*20.0f));
    }

    AnimCurve curve;
    curve.IpolType = AnimIpolType::Linear;
    curve.Format = AnimKeyFormat::Float;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysFloat.begin(), sizeof(keysFloat));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Float2;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysFloat2.begin(), sizeof(keysFloat2));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Float3;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysFloat3.begin(), sizeof(keysFloat3));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Float4;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysFloat4.begin(), sizeof(keysFloat4));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Byte4N;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysByte4N.begin(), sizeof(keysByte4N));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Short2N;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysShort2N.begin(), sizeof(keysShort2N));
    anim.AddCurve(curve);
    curve.Format = AnimKeyFormat::Short4N;
    curve.KeyStride = AnimKeyFormat::ByteSize(curve.Format);
    curve.KeyOffset = anim.AddKeys(keysShort4N.begin(), sizeof(keysShort4N));
    anim.AddCurve(curve);

    const int keyDuration = 1000000 / 25;
    AnimClip clip;
    clip.StartCurveIndex = 0;
    clip.NumCurves = anim.NumCurves();
    clip.NumKeys = numKeys;
    clip.KeyDuration = keyDuration;
    anim.AddClip(clip);

    AnimLibrary lib;
    lib.StartClipIndex = 0;
    lib.NumClips = anim.NumClips();
    lib.StartCurveIndex = 0;
    lib.NumCurves = anim.NumCurves();
    lib.StartKeyOffset = 0;
    lib.KeyRangeSize = anim.KeyBuffer().Size();
    lib.ClipNameMap.Add("clip0", 0);
    anim.AddLibrary("lib0", std::move(lib));

    anim.Sample(0, 0, AnimSampleMode::Step, AnimWrapMode::Clamp, 0);
    anim.Sample(0, keyDuration * 1, AnimSampleMode::Step, AnimWrapMode::Clamp, 1);


}
