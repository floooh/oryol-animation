//------------------------------------------------------------------------------
//  AnimSkeletonTest.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "UnitTest++/src/UnitTest++.h"
#include "Anim/AnimTypes.h"
#include "Anim/private/animMgr.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace Oryol;
using namespace _priv;

TEST(AnimSkeletonTest) {

    AnimSetup setup;
    setup.MaxNumSkeletons = 4;
    setup.MatrixPoolCapacity = 128;
    animMgr mgr;
    mgr.setup(setup);
    CHECK(mgr.isValid);
    CHECK(mgr.skelPool.IsValid());
    CHECK(mgr.matrixPool.Capacity() == 128);

    glm::mat4 m0 = glm::translate(glm::mat4(), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::mat4 m1 = glm::translate(glm::mat4(), glm::vec3(4.0f, 5.0f, 6.0f));
    AnimSkeletonSetup skelSetup;
    skelSetup.Name = "test";
    skelSetup.Bones = {
        { "root", -1, glm::mat4(), glm::mat4() },
        { "spine0", 0, m0, glm::inverse(m0) },
        { "spine1", 1, m1, glm::inverse(m1) }
    };
    ResourceLabel l1 = mgr.resContainer.PushLabel();
    Id skelId = mgr.createSkeleton(skelSetup);
    mgr.resContainer.PopLabel();
    CHECK(mgr.matrixPool.Size() == 6);
    AnimSkeleton* skel = mgr.lookupSkeleton(skelId);
    CHECK(skel);
    CHECK(skel->Name == "test");

    mgr.destroy(l1);

    mgr.discard();
    CHECK(!mgr.isValid);
    CHECK(mgr.matrixPool.Size() == 0);
}
