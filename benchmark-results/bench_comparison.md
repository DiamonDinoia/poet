# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / Dispatch baselines: if-else / switch / fn-ptr / POET (8 branches)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `dispatch if-else` | 0.62 (100.0%) | 0.94 (100.0%) | 0.63 (100.0%) | 0.93 (100.0%) |
| `dispatch switch` | 0.62 (100.2%) | 0.93 (100.2%) | 0.62 (100.5%) | 0.94 (99.8%) |
| `dispatch fn-ptr` | 9.64 (6.5%) | 9.33 (10.0%) | 9.63 (6.5%) | 9.33 (10.0%) |
| `dispatch POET` | 9.33 (6.7%) | 9.64 (9.7%) | 9.33 (6.7%) | 9.35 (10.0%) |

## compiler_comparison_bench / Vectorization probe: saxpy + reduce (N=4096)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `saxpy plain` | 1.01 (100.0%) | 0.99 (100.0%) | 1.01 (100.0%) | 0.99 (100.0%) |
| `saxpy aligned` | 1.01 (99.8%) | 0.99 (100.0%) | 1.01 (100.1%) | 0.99 (100.0%) |
| `saxpy restrict` | 1.01 (99.9%) | 0.99 (99.9%) | 1.01 (100.0%) | 0.99 (100.0%) |

## compiler_comparison_bench / N sweep: 1-acc vs tuned-acc vs dynamic_for

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `sweep 1-acc (N=64)` | 1.23 (100.0%) | 1.26 (100.0%) | 1.23 (100.0%) | 1.26 (100.0%) |
| `sweep tuned-acc (N=64)` | 1.18 (104.1%) | 0.79 (158.8%) | 1.18 (104.3%) | 0.79 (158.6%) |
| `sweep dynamic_for (N=64)` | 1.17 (104.8%) | 0.74 (171.1%) | 1.17 (105.0%) | 0.8 (157.9%) |
| `sweep 1-acc (N=512)` | 1.23 (12.5%) | 1.26 (12.5%) | 1.23 (12.5%) | 1.29 (12.2%) |
| `sweep tuned-acc (N=512)` | 1.13 (13.6%) | 0.73 (21.6%) | 1.13 (13.6%) | 0.73 (21.6%) |
| `sweep dynamic_for (N=512)` | 1.14 (13.5%) | 0.67 (23.5%) | 1.14 (13.5%) | 0.73 (21.5%) |
| `sweep 1-acc (N=4096)` | 1.22 (1.6%) | 1.25 (1.6%) | 1.22 (1.6%) | 1.25 (1.6%) |
| `sweep tuned-acc (N=4096)` | 1.12 (1.7%) | 0.72 (2.7%) | 1.12 (1.7%) | 0.72 (2.7%) |
| `sweep dynamic_for (N=4096)` | 1.12 (1.7%) | 0.66 (3.0%) | 1.12 (1.7%) | 0.72 (2.7%) |
| `sweep 1-acc (N=32768)` | 1.22 (0.2%) | 1.25 (0.2%) | 1.22 (0.2%) | 1.25 (0.2%) |
| `sweep tuned-acc (N=32768)` | 1.12 (0.2%) | 0.72 (0.3%) | 1.13 (0.2%) | 0.72 (0.3%) |
| `sweep dynamic_for (N=32768)` | 1.13 (0.2%) | 0.66 (0.4%) | 1.13 (0.2%) | 0.72 (0.3%) |

## compiler_comparison_bench / Template inlining: plain loop vs static_for (trivial body)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `inline plain-loop (N=4)` | 0.04 (100.0%) | 0.08 (100.0%) | 0.04 (100.0%) | 0.08 (100.0%) |
| `inline static_for (N=4)` | 0.04 (100.0%) | 0.08 (100.0%) | 0.04 (100.1%) | 0.08 (100.0%) |
| `inline plain-loop (N=8)` | 0.02 (100.2%) | 0.04 (99.9%) | 0.02 (100.1%) | 0.04 (100.0%) |
| `inline static_for (N=8)` | 0.02 (100.1%) | 0.04 (100.0%) | 0.02 (100.1%) | 0.04 (100.0%) |
| `inline plain-loop (N=16)` | 0.01 (100.1%) | 0.02 (99.8%) | 0.01 (100.1%) | 0.02 (100.0%) |
| `inline static_for (N=16)` | 0.01 (98.4%) | 0.02 (99.8%) | 0.01 (100.1%) | 0.02 (100.0%) |

## dispatch_bench / dispatch: dimensionality, hit/miss, contiguous/sparse

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `1D contiguous hit` | 9.38 | 9.33 | 9.41 | 9.32 |
| `1D contiguous miss` | 0.94 | 0.93 | 0.94 | 0.93 |
| `1D non-contiguous hit` | 4.6 | 4.9 | 4.59 | 4.9 |
| `1D non-contiguous miss` | 2.65 | 3.14 | 2.65 | 3.13 |
| `2D contiguous hit` | 9.96 | 9.95 | 9.95 | 9.94 |
| `2D contiguous miss` | 1.24 | 0.93 | 1.24 | 0.93 |
| `2D non-contiguous hit` | 9.51 | 8.97 | 9.53 | 8.97 |
| `2D non-contiguous miss` | 7.74 | 7.64 | 7.75 | 7.63 |
| `5D contiguous hit` | 2.57 | 2.57 | 2.57 | 2.56 |
| `5D contiguous miss` | 0.31 | 0.31 | 0.31 | 0.31 |
| `5D non-contiguous hit` | 13.59 | 7.25 | 13.59 | 7.25 |
| `5D non-contiguous miss` | 0.62 | 0.31 | 0.62 | 0.31 |

## dispatch_optimization_bench / Compile-time specialization: runtime N vs dispatched N

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `N=4  runtime` | 1.46 (100.0%) | 2.1 (100.0%) | 1.46 (100.0%) | 2.1 (100.0%) |
| `N=4  dispatched` | 0.95 (153.0%) | 0.96 (218.8%) | 0.95 (153.8%) | 0.93 (224.5%) |
| `N=8  runtime` | 2.26 (64.5%) | 3.3 (63.5%) | 2.26 (64.8%) | 3.3 (63.5%) |
| `N=8  dispatched` | 1.43 (101.7%) | 1.56 (134.9%) | 1.43 (102.5%) | 1.56 (134.8%) |
| `N=16 runtime` | 4.37 (33.4%) | 4.88 (43.0%) | 4.37 (33.5%) | 4.88 (43.0%) |
| `N=16 dispatched` | 3.09 (47.2%) | 3.08 (68.2%) | 3.09 (47.4%) | 3.09 (67.8%) |
| `N=32 runtime` | 12.47 (11.7%) | 10.25 (20.5%) | 12.49 (11.7%) | 10.25 (20.5%) |
| `N=32 dispatched` | 9.16 (15.9%) | 9.15 (22.9%) | 9.15 (16.0%) | 9.15 (22.9%) |

## dynamic_for_bench / Multi-acc: dynamic_for lane callbacks (N=10000)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `for loop (1 acc)` | 1.22 (100.0%) | 1.28 (100.0%) | 1.22 (100.0%) | 1.28 (100.0%) |
| `for loop (optimal accs)` | 1.12 (109.0%) | 0.66 (192.5%) | 1.12 (109.0%) | 0.72 (176.9%) |
| `dynamic_for (optimal accs)` | 0.8 (152.8%) | 0.66 (192.2%) | 0.8 (152.9%) | 0.72 (177.9%) |

## dynamic_for_bench / Unroll: plain for vs optimal vs spill (N=10000)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `plain for (1 acc)` | 1.22 (100.0%) | 1.25 (100.0%) | 1.22 (100.0%) | 1.25 (100.0%) |
| `dynamic_for<optimal>` | 0.8 (152.9%) | 0.66 (188.1%) | 0.8 (152.9%) | 0.72 (174.1%) |
| `dynamic_for<spill>` | 0.7 (175.0%) | 0.54 (233.2%) | 0.7 (175.5%) | 0.54 (233.0%) |

## static_for_bench / Map: static_for tuned vs default (N=256, heavy body)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `for loop` | 1.53 (100.0%) | 1.41 (100.0%) | 1.53 (100.0%) | 1.41 (100.0%) |
| `static_for (tuned BS)` | 1.38 (110.8%) | 1.41 (100.0%) | 1.38 (110.4%) | 1.41 (99.9%) |
| `static_for (default BS)` | 3.06 (50.0%) | 1.36 (103.5%) | 3.05 (50.0%) | 1.36 (103.7%) |

## static_for_bench / Multi-acc: static_for tuned vs default (N=256, heavy body)

| Benchmark | gcc-14 default | clang-21 default | gcc-latest default | clang-latest default |
|:----------|--------:|--------:|--------:|--------:|
| `for loop` | 1.24 (100.0%) | 1.28 (100.0%) | 1.24 (100.0%) | 1.28 (100.0%) |
| `static_for (tuned BS)` | 0.55 (226.4%) | 0.63 (201.9%) | 0.55 (226.7%) | 0.63 (201.9%) |
| `static_for (default BS)` | 3.7 (33.6%) | 1.44 (88.7%) | 3.7 (33.6%) | 1.44 (88.4%) |

