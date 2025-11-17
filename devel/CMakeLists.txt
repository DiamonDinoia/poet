CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG master
)

add_executable(gemm gemm.cpp)
target_link_libraries(gemm poet::poet xsimd)
target_compile_options(gemm PRIVATE -Ofast -march=native -funroll-loops -finline-functions -fomit-frame-pointer)
set_target_properties(gemm PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)


add_executable(kernel microkernel.cpp)
target_link_libraries(kernel poet::poet xsimd)
target_compile_options(kernel PRIVATE
    -Ofast
    -march=native
    -funroll-loops
    -finline-functions
    -fomit-frame-pointer
    -frename-registers
    -fmodulo-sched
    -fmodulo-sched-allow-regmoves
)
set_target_properties(kernel PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(flops flops.cpp)
target_link_libraries(flops xsimd)
# Match flags for `flops` as well
target_compile_options(flops PRIVATE -Ofast -march=native -funroll-loops -finline-functions -fomit-frame-pointer)
