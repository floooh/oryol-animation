fips_begin_module(Anim)
    fips_vs_warning_level(3)
    fips_files(
        Anim.h Anim.cc
        AnimTypes.h
    )
    fips_deps(Resource)
fips_end_module()

fips_begin_unittest(Anim)
    fips_vs_warning_level(3)
    fips_dir(UnitTests)
    fips_deps(Anim)
fips_end_unittest()

