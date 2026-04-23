# ─────────────────────────────────────────────────────────────────────────────
# CompilerFlags.cmake — hardware-level optimisation flags
# ─────────────────────────────────────────────────────────────────────────────

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -O3                    # full optimisation + auto-vectorisation
        -march=native          # AVX2, BMI2, LZCNT, TZCNT for this exact CPU
        -funroll-loops         # unroll matching while-loop
        -fomit-frame-pointer   # one extra register on x86-64
        -fno-semantic-interposition  # allows cross-TU inlining without LTO
    )
    # Link-time optimisation: inlines addOrder into the benchmark loop
    # across translation unit boundaries
    add_link_options(-flto=auto)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

elseif(MSVC)
    add_compile_options(
        /O2         # max speed
        /Oi         # enable intrinsics (maps to hardware instructions)
        /Ot         # favour fast code
        /Gy         # function-level linking
        /GL         # whole-program optimisation (= LTO)
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_compile_options(/arch:AVX2)
    endif()
    add_link_options(/LTCG)
endif()

if(MSVC)
    add_compile_definitions(NOMINMAX)
endif()
