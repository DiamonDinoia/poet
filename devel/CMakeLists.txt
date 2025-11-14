CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG 13.2.0
)

add_executable(gemm gemm.cpp)
target_link_libraries(gemm poet::poet xsimd)
# Add explicit AVX2/FMA flags and sane optimization flags; keep other heuristics
target_compile_options(gemm PRIVATE -Ofast -march=native -funroll-loops -frename-registers -fira-hoist-pressure 
     -fira-loop-pressure -finline-functions -fno-optimize-sibling-calls 
     -fvect-cost-model=unlimited -fschedule-insns2 -fomit-frame-pointer 
     -fopt-info-vec)
set_target_properties(gemm PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)


add_executable(kernel microkernel.cpp)
target_link_libraries(kernel poet::poet xsimd)
# Add explicit AVX2/FMA flags and sane optimization flags; keep other heuristics
target_compile_options(kernel PRIVATE -Ofast -march=native -funroll-loops -frename-registers -fira-hoist-pressure 
     -fira-loop-pressure -finline-functions -fno-optimize-sibling-calls 
     -fvect-cost-model=unlimited -fschedule-insns2 -fomit-frame-pointer 
     -fopt-info-vec)
set_target_properties(kernel PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(flops flops.cpp)
target_link_libraries(flops xsimd)
# Match flags for `flops` as well
target_compile_options(flops PRIVATE -Ofast -march=native -funroll-loops -frename-registers -fira-hoist-pressure 
     -fira-loop-pressure -finline-functions -fno-optimize-sibling-calls 
     -fvect-cost-model=unlimited -fschedule-insns2 -fomit-frame-pointer 
     -fopt-info-vec)
