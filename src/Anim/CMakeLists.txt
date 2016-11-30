fips_begin_module(Anim)
    fips_vs_warning_level(3)
    fips_files(
        AnimSystem.h AnimSystem.cc
        Types.h
    )
    fips_dir(Core)
    fips_deps(Resource)
fips_end_module()

fips_begin_unittest(Anim)
    fips_vs_warning_level(3)
    fips_dir(UnitTests)
    fips_files(
        AnimSystemTest.cc
    )
    fips_deps(Anim)
fips_end_unittest()

