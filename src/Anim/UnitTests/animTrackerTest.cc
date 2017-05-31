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
    CHECK(tracker.numItems == 1);
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
    CHECK(tracker.numItems == 2);
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
    CHECK(tracker.numItems == 3);
    CHECK(tracker.items[0].id == 31);
    CHECK(tracker.items[1].id == 23);
    CHECK(tracker.items[2].id == 25);
    CHECK_CLOSE(tracker.items[0].mixWeight, 0.1f, delta);
    CHECK_CLOSE(tracker.items[0].absStartTime, 0.0, delta);
    CHECK_CLOSE(tracker.items[0].absFadeInTime, 0.2, delta);
    CHECK_CLOSE(tracker.items[0].absFadeOutTime, 19.8, delta);
    CHECK_CLOSE(tracker.items[0].absEndTime, 20.0, delta);

    // FIXME: insert at track #4
    // FIXME: insert at track 2 at time 2.0, check that existing job is clipped
    // FIXME: insert with overlapping end
    // FIXME: insert same track, in future, non-overlapping
    // FIXME: insert same track, before existing track, non overlapping
    // FIXME: insert new track that completely obscures existing track

}

