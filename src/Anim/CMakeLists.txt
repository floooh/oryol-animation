fips_begin_module(Anim)
    fips_files(
        Anim.h Anim.cc
    )
    fips_dir(Core)
    fips_files(
        Types.h
        AnimSetup.h
    )
    fips_deps(Resource)
fips_end_module()
