//------------------------------------------------------------------------------
//  animTrackerTest.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "UnitTest++/src/UnitTest++.h"
#include "Anim/private/animTracker.h"
#include <float.h>

using namespace Oryol;
using namespace _priv;

TEST(animTrackerTest) {

    const double delta = 0.00001;

    animTracker tracker;

    AnimJob job0;
    job0.ClipIndex = 0;
    job0.TrackIndex = 2;
    job0.StartTime = job0.Duration = 0.0f;
    job0.FadeIn = job0.FadeOut = 0.1f;
    bool res0 = tracker.add(0.0, 23, job0, 10.0f);
    CHECK(res0);
    CHECK(tracker.items.Size() == 1);
    CHECK(tracker.items[0].id == 23);
    CHECK(tracker.items[0].valid);
    CHECK(tracker.items[0].clipIndex == 0);
    CHECK(tracker.items[0].trackIndex == 2);
    CHECK_CLOSE(tracker.items[0].mixWeight, 1.0f, delta);
    CHECK_CLOSE(tracker.items[0].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[0].absFadeInTime, 0.1, delta);
    CHECK_CLOSE(tracker.items[0].absFadeOutTime, DBL_MAX, delta);
    CHECK_CLOSE(tracker.items[0].absEndTime, DBL_MAX, delta);

    // insert a job at a higher track
    AnimJob job1;
    job1.ClipIndex = 1;
    job1.TrackIndex = 5;
    job1.StartTime = 0.0f;
    job1.MixWeight = 0.5f;
    job1.Duration = 1.5f;
    job1.DurationIsLoopCount = true;
    job1.FadeIn = job1.FadeOut = 0.1f;
    bool res1 = tracker.add(0.0, 25, job1, 20.0f);
    CHECK(res1);
    CHECK(tracker.items.Size() == 2);
    CHECK(tracker.items[0].id == 23);
    CHECK(tracker.items[1].id == 25);
    CHECK(tracker.items[1].valid);
    CHECK(tracker.items[1].clipIndex == 1);
    CHECK(tracker.items[1].trackIndex == 5);
    CHECK_CLOSE(tracker.items[1].mixWeight, 0.5f, delta);
    CHECK_CLOSE(tracker.items[1].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[1].absFadeInTime, 0.1, delta);
    CHECK_CLOSE(tracker.items[1].absFadeOutTime, 29.9f, delta);
    CHECK_CLOSE(tracker.items[1].absEndTime, 30.0f, delta);

    // insert a job at lower track
    AnimJob job2;
    job2.ClipIndex = 2;
    job2.TrackIndex = 0;
    job2.StartTime = 0.0f;
    job2.MixWeight = 0.1f;
    job2.Duration = 20.0f;
    job2.FadeIn = job2.FadeOut = 0.2f;
    bool res2 = tracker.add(0.0, 31, job2, 10.0f);
    CHECK(res2);
    CHECK(tracker.items.Size() == 3);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 25);
    CHECK_CLOSE(tracker.items[0].mixWeight, 0.1f, delta);
    CHECK_CLOSE(tracker.items[0].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[0].absFadeInTime, 0.2, delta);
    CHECK_CLOSE(tracker.items[0].absFadeOutTime, 19.8, delta);
    CHECK_CLOSE(tracker.items[0].absEndTime, 20.0, delta);

    // insert at track #4
    AnimJob job3;
    job3.ClipIndex = 3;
    job3.TrackIndex = 4;
    job3.StartTime = 1.0f;
    job3.Duration = 2.0f;
    job3.FadeIn = job3.FadeOut = 0.1f;
    bool res3 = tracker.add(0.0, 44, job3, 10.0f);
    CHECK(res3);
    CHECK(tracker.items.Size() == 4);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 44);
    CHECK(tracker.items[3].id == 25);

    // insert at track 2 at time 10.0, check that existing job on track 2 is clipped
    AnimJob job4;
    job4.ClipIndex = 4;
    job4.TrackIndex = 2;
    job4.StartTime = 0.0f;
    job4.Duration = 0.0f;
    job4.FadeIn = job4.FadeOut = 0.1f;
    bool res4 = tracker.add(10.0, 55, job4, 10.0f);
    CHECK(res4);
    CHECK(tracker.items.Size() == 5);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 55);
    CHECK(tracker.items[3].id == 44);
    CHECK(tracker.items[4].id == 25);
    CHECK_CLOSE(tracker.items[1].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[1].absFadeInTime, 0.1, delta);
    CHECK_CLOSE(tracker.items[1].absFadeOutTime, 10.0, delta);
    CHECK_CLOSE(tracker.items[1].absEndTime, 10.1, delta);
    CHECK_CLOSE(tracker.items[2].absStartTime, 10.0, delta);
    CHECK_CLOSE(tracker.items[2].absFadeInTime, 10.1, delta);
    CHECK_CLOSE(tracker.items[2].absFadeOutTime, DBL_MAX, delta);
    CHECK_CLOSE(tracker.items[2].absEndTime, DBL_MAX, delta);

    // insert on track 2 with overlapping start and end
    AnimJob job5;
    job5.ClipIndex = 5;
    job5.TrackIndex = 2;
    job5.StartTime = 5.0f;
    job5.Duration = 10.0f;
    job5.FadeIn = job5.FadeOut = 0.2f;
    bool res5 = tracker.add(0.0f, 66, job5, 10.0f);
    CHECK(res5);
    CHECK(tracker.items.Size() == 6);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 66);
    CHECK(tracker.items[3].id == 55);
    CHECK(tracker.items[4].id == 44);
    CHECK(tracker.items[5].id == 25);
    CHECK(tracker.items[1].trackIndex == 2);
    CHECK(tracker.items[2].trackIndex == 2);
    CHECK(tracker.items[3].trackIndex == 2);
    CHECK_CLOSE(tracker.items[1].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[1].absFadeInTime, 0.1, delta);
    CHECK_CLOSE(tracker.items[1].absFadeOutTime, 5.0, delta);
    CHECK_CLOSE(tracker.items[1].absEndTime, 5.2, delta);
    CHECK_CLOSE(tracker.items[2].absStartTime, 5.0, delta);
    CHECK_CLOSE(tracker.items[2].absFadeInTime, 5.2, delta);
    CHECK_CLOSE(tracker.items[2].absFadeOutTime, 14.8, delta);
    CHECK_CLOSE(tracker.items[2].absEndTime, 15.0, delta);
    CHECK_CLOSE(tracker.items[3].absStartTime, 14.8, delta);
    CHECK_CLOSE(tracker.items[3].absFadeInTime, 15.0, delta);
    CHECK_CLOSE(tracker.items[3].absFadeOutTime, DBL_MAX, delta);
    CHECK_CLOSE(tracker.items[3].absEndTime, DBL_MAX, delta);

    // insert a job which completely obscures another job
    AnimJob job6;
    job6.ClipIndex = 3;
    job6.TrackIndex = 4;
    job6.StartTime = 0.0f;
    job6.Duration = 5.0f;
    job6.FadeIn = job3.FadeOut = 0.1f;
    bool res6 = tracker.add(0.0, 77, job6, 10.0f);
    CHECK(res6);
    CHECK(tracker.items.Size() == 7);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 66);
    CHECK(tracker.items[3].id == 55);
    CHECK(tracker.items[4].id == 77);
    CHECK(tracker.items[5].id == 44);
    CHECK(tracker.items[6].id == 25);
    CHECK(!tracker.items[5].valid);
    
    // FIXME: test garbageCollect
}

