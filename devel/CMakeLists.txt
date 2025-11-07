CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG 13.2.0
)

add_executable(gemm gemm.cpp)
target_link_libraries(gemm poet::poet xsimd)
# Add explicit AVX2/FMA flags and sane optimization flags; keep other heuristics
target_compile_options(gemm PRIVATE -O3 -march=native -mavx2 -mfma -funroll-loops -frename-registers -fira-algorithm=CB -fira-region=one -fira-verbose=5)


add_executable(flops flops.cpp)
target_link_libraries(flops xsimd)
# Match flags for `flops` as well
target_compile_options(flops PRIVATE -O3 -march=native -mavx2 -mfma -funroll-loops -frename-registers -fira-algorithm=CB -fira-region=one -fira-verbose=5)
