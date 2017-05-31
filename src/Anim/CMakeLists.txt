fips_begin_module(Anim)
    fips_vs_warning_level(3)
    fips_files(
        Anim.h Anim.cc
        AnimTypes.h
    )
    fips_dir(private)
    fips_files(
        animMgr.h animMgr.cc
        animTracker.h animTracker.cc
    )
    fips_deps(Core Resource)
fips_end_module()

fips_begin_unittest(Anim)
    fips_vs_warning_level(3)
    fips_dir(UnitTests)
    fips_files(
        animMgrTest.cc
        animTrackerTest.cc
    )
    fips_deps(Anim)
fips_end_unittest()

