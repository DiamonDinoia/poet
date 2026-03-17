# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.7 | 0.9 | 0.9 |
| switch | 0.6 | 0.6 | 0.9 | 0.9 |
| fn_ptr | 9.7 | 9.6 | 9.5 | 9.4 |
| POET | 8.7 | 8.7 | 9.4 | 9.6 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4196.3 | 4110.5 | 4031.7 | 4038.1 |
| saxpy_aligned | 4164.2 | 4106.1 | 4038.0 | 4037.7 |
| saxpy_restrict | 4138.1 | 4107.4 | 4037.8 | 4040.7 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.7 | 60.1 | 82.1 | 80.1 |
| tuned-acc_N=64 | 54.4 | 46.6 | 50.9 | 51.1 |
| dynamic_for_N=64 | 75.3 | 46.4 | 50.1 | 46.3 |
| 1-acc_N=512 | 629.6 | 481.8 | 665.0 | 647.0 |
| tuned-acc_N=512 | 410.7 | 348.5 | 372.8 | 375.2 |
| dynamic_for_N=512 | 583.6 | 381.8 | 374.2 | 343.1 |
| 1-acc_N=4096 | 5019.6 | 3857.1 | 5047.2 | 5039.8 |
| tuned-acc_N=4096 | 3258.3 | 2762.4 | 2950.4 | 2960.7 |
| dynamic_for_N=4096 | 4636.4 | 3049.9 | 2965.1 | 2715.1 |
| 1-acc_N=32768 | 40066.6 | 30852.2 | 41579.5 | 40961.7 |
| tuned-acc_N=32768 | 25977.4 | 22094.1 | 23488.7 | 23615.6 |
| dynamic_for_N=32768 | 36853.9 | 24375.2 | 23701.0 | 21697.1 |

## compiler_comparison_bench / Inline

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_loop_N=4 | 0.2 | 0.2 | 0.3 | 0.3 |
| static_for_N=4 | 0.2 | 0.2 | 0.3 | 0.3 |
| plain_loop_N=8 | 0.2 | 0.2 | 0.3 | 0.3 |
| static_for_N=8 | 0.2 | 0.2 | 0.3 | 0.3 |
| plain_loop_N=16 | 0.2 | 0.2 | 0.3 | 0.3 |
| static_for_N=16 | 0.2 | 0.2 | 0.3 | 0.3 |

## dispatch_bench / Dispatch

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1D_contiguous_hit | 8.7 | 9.4 | 9.4 | 9.4 |
| 1D_contiguous_miss | 0.6 | 0.6 | 1.0 | 0.9 |
| 1D_non-contiguous_hit | 4.9 | 4.6 | 4.8 | 4.8 |
| 1D_non-contiguous_miss | 2.7 | 2.6 | 3.1 | 3.1 |
| 2D_contiguous_hit | 9.9 | 9.3 | 9.6 | 9.6 |
| 2D_contiguous_miss | 0.6 | 0.6 | 1.3 | 1.2 |
| 2D_non-contiguous_hit | 10.3 | 9.9 | 9.0 | 8.9 |
| 2D_non-contiguous_miss | 5.6 | 5.3 | 6.3 | 6.3 |
| 5D_contiguous_hit | 2.6 | 2.6 | 2.6 | 2.6 |
| 5D_contiguous_miss | 0.3 | 0.3 | 0.6 | 0.6 |
| 5D_non-contiguous_hit | 13.7 | 12.9 | 7.0 | 7.0 |
| 5D_non-contiguous_miss | 0.3 | 0.3 | 0.6 | 0.6 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 2.1 | 3.1 | 2.5 | 2.5 |
| N=4_dispatched | 0.9 | 0.8 | 0.8 | 0.8 |
| N=8_runtime | 3.0 | 3.4 | 3.3 | 3.3 |
| N=8_dispatched | 1.4 | 1.6 | 1.4 | 1.4 |
| N=16_runtime | 4.6 | 5.2 | 5.0 | 5.0 |
| N=16_dispatched | 3.1 | 3.1 | 3.0 | 3.0 |
| N=32_runtime | 12.3 | 12.1 | 10.0 | 10.0 |
| N=32_dispatched | 9.2 | 9.2 | 9.2 | 9.1 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12311.3 | 9419.5 | 12456.8 | 12488.5 |
| for_loop_optimal_accs | 11226.8 | 7493.9 | 7246.1 | 6634.9 |
| dynamic_for_1_acc | 12257.2 | 9419.1 | 12495.7 | 12505.2 |
| dynamic_for_optimal_accs | 7990.2 | 6736.6 | 7176.4 | 6637.7 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 13551.8 | 9415.3 | 12472.4 | 12469.2 |
| dynamic_for_optimal | 8104.4 | 6734.7 | 7176.2 | 6638.9 |
| dynamic_for_spill | 6956.6 | 7012.2 | 5596.1 | 5592.7 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11243.2 | 7484.3 | 7214.3 | 6639.5 |
| computed-index | 11399.4 | 7487.3 | 7212.6 | 6637.9 |
| dynamic_for_lane_form | 11222.3 | 7482.0 | 7244.6 | 6612.1 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 8176.8 | 5806.1 | 3951.3 | 4065.8 |
| computed-index | 8182.1 | 5809.2 | 3942.6 | 4068.6 |
| dynamic_for_lane_form | 8184.9 | 5834.7 | 3948.4 | 4072.7 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5639.0 | 3732.3 | 3621.4 | 3310.5 |
| dynamic_for_RT_stride_2 | 5644.6 | 3736.7 | 3617.7 | 3311.4 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12248.5 | 9418.2 | 12463.0 | 12462.1 |
| dynamic_for_index_only_1_acc | 11322.3 | 11213.7 | 11862.0 | 11851.9 |
| dynamic_for_lane_form_optimal_accs | 11375.5 | 7483.1 | 7239.0 | 6611.9 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5360.6 | 5319.6 | 4470.8 | 4455.7 |
| dynamic_for_index_only | 11561.8 | 9126.1 | 6219.7 | 6205.3 |
| dynamic_for_lane_form_unused_lane | 11547.8 | 9135.2 | 6222.6 | 6214.5 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.5 | 2.9 | 2.9 |
| dynamic_for_index_only_N=3 | 3.1 | 2.8 | 3.4 | 3.4 |
| dynamic_for_lane_form_N=3 | 2.8 | 2.8 | 3.4 | 3.4 |
| plain_for_N=7 | 8.0 | 7.3 | 8.0 | 8.0 |
| dynamic_for_index_only_N=7 | 8.4 | 6.8 | 8.6 | 8.6 |
| dynamic_for_lane_form_N=7 | 6.8 | 6.8 | 8.6 | 8.6 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 387.3 | 388.8 | 363.6 | 360.3 |
| static_for_tuned_BS | 355.4 | 353.1 | 363.0 | 360.8 |
| static_for_default_BS | 828.1 | 784.7 | 347.6 | 346.1 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 327.9 | 241.2 | 324.4 | 324.6 |
| static_for_tuned_BS | 140.6 | 140.8 | 162.6 | 162.9 |
| static_for_default_BS | 917.6 | 919.8 | 371.2 | 370.3 |

