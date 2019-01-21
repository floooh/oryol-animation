fips_begin_module(Anim)
    fips_vs_warning_level(3)
    fips_files(
        Anim.h Anim.cc
        AnimTypes.h
    )
    fips_dir(private)
    fips_files(
        animMgr.h animMgr.cc
        animSequencer.h animSequencer.cc
        animInstance.h
    )
    fips_deps(Core Resource)
fips_end_module()

oryol_begin_unittest(Anim)
    fips_vs_warning_level(3)
    fips_dir(UnitTests)
    fips_files(
        AnimLibraryTest.cc
        AnimSkeletonTest.cc
        animSequencerTest.cc
    )
    fips_deps(Anim)
oryol_end_unittest()

