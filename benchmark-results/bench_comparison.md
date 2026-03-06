# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.7 | 0.9 | 1.0 |
| switch | 0.6 | 0.6 | 1.1 | 0.9 |
| fn_ptr | 9.6 | 10.0 | 9.3 | 9.3 |
| POET | 8.7 | 8.7 | 9.3 | 9.6 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4142.8 | 4116.1 | 4033.5 | 4039.4 |
| saxpy_aligned | 4137.5 | 4097.0 | 4039.5 | 4042.5 |
| saxpy_restrict | 4133.2 | 4092.2 | 4044.5 | 4038.7 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.6 | 60.1 | 82.1 | 80.2 |
| tuned-acc_N=64 | 54.5 | 46.6 | 50.8 | 51.1 |
| dynamic_for_N=64 | 75.4 | 46.5 | 50.0 | 46.3 |
| 1-acc_N=512 | 629.0 | 481.9 | 664.2 | 645.8 |
| tuned-acc_N=512 | 410.6 | 348.5 | 372.9 | 375.1 |
| dynamic_for_N=512 | 578.9 | 382.0 | 373.7 | 343.1 |
| 1-acc_N=4096 | 5007.2 | 3855.6 | 5043.4 | 5041.9 |
| tuned-acc_N=4096 | 3258.0 | 2762.8 | 2940.2 | 2958.7 |
| dynamic_for_N=4096 | 4598.7 | 3048.8 | 2963.9 | 2717.6 |
| 1-acc_N=32768 | 40049.1 | 30839.8 | 41638.8 | 40887.3 |
| tuned-acc_N=32768 | 26022.2 | 22089.9 | 23499.0 | 23637.0 |
| dynamic_for_N=32768 | 36615.1 | 24378.4 | 23674.0 | 21687.8 |

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
| 1D_contiguous_miss | 0.6 | 0.6 | 0.9 | 0.9 |
| 1D_non-contiguous_hit | 4.6 | 4.6 | 4.8 | 4.8 |
| 1D_non-contiguous_miss | 2.7 | 2.8 | 3.1 | 3.1 |
| 2D_contiguous_hit | 9.9 | 9.3 | 9.6 | 9.6 |
| 2D_contiguous_miss | 0.6 | 0.6 | 1.2 | 1.2 |
| 2D_non-contiguous_hit | 9.9 | 10.3 | 8.9 | 8.9 |
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
| N=8_runtime | 3.0 | 3.4 | 3.3 | 3.4 |
| N=8_dispatched | 1.4 | 1.6 | 1.4 | 1.4 |
| N=16_runtime | 4.6 | 5.2 | 5.0 | 5.0 |
| N=16_dispatched | 3.1 | 3.1 | 3.0 | 3.0 |
| N=32_runtime | 12.3 | 12.1 | 10.0 | 10.0 |
| N=32_dispatched | 9.2 | 9.2 | 9.2 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12244.0 | 9411.8 | 12446.3 | 12465.0 |
| for_loop_optimal_accs | 11234.5 | 7488.6 | 7239.3 | 6642.0 |
| dynamic_for_1_acc | 12231.9 | 9413.0 | 12495.2 | 12484.3 |
| dynamic_for_optimal_accs | 7938.3 | 6733.3 | 7168.5 | 6645.1 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12223.3 | 9416.8 | 12453.5 | 12475.7 |
| dynamic_for_optimal | 7938.1 | 6735.3 | 7166.2 | 6647.7 |
| dynamic_for_spill | 6952.7 | 7014.0 | 5591.7 | 5596.2 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11224.8 | 7482.6 | 7205.8 | 6637.4 |
| computed-index | 11315.4 | 7487.1 | 7243.0 | 6635.8 |
| dynamic_for_lane_form | 11242.5 | 7480.1 | 7239.3 | 6612.2 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 8185.3 | 5801.7 | 3950.8 | 4065.9 |
| computed-index | 8181.7 | 5805.4 | 3943.0 | 4071.1 |
| dynamic_for_lane_form | 8179.1 | 5805.4 | 3945.0 | 4068.9 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5653.3 | 3732.5 | 3614.5 | 3306.8 |
| dynamic_for_RT_stride_2 | 5715.7 | 3731.5 | 3616.0 | 3313.3 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12243.5 | 9413.9 | 12462.3 | 12454.8 |
| dynamic_for_index_only_1_acc | 11252.0 | 11193.1 | 11864.5 | 11799.4 |
| dynamic_for_lane_form_optimal_accs | 11221.1 | 7475.1 | 7226.0 | 6616.8 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5386.6 | 5347.5 | 4500.2 | 4453.9 |
| dynamic_for_index_only | 11550.5 | 9125.0 | 6302.2 | 6209.8 |
| dynamic_for_lane_form_unused_lane | 11545.5 | 9123.1 | 6221.2 | 6213.2 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.5 | 2.9 | 2.9 |
| dynamic_for_index_only_N=3 | 3.1 | 2.8 | 3.3 | 3.4 |
| dynamic_for_lane_form_N=3 | 2.8 | 2.8 | 3.3 | 3.4 |
| plain_for_N=7 | 7.3 | 7.3 | 8.0 | 8.0 |
| dynamic_for_index_only_N=7 | 6.9 | 6.8 | 8.6 | 8.6 |
| dynamic_for_lane_form_N=7 | 6.8 | 6.8 | 8.6 | 8.6 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 387.7 | 388.8 | 363.9 | 360.1 |
| static_for_tuned_BS | 354.9 | 353.2 | 362.2 | 360.7 |
| static_for_default_BS | 829.0 | 785.3 | 348.5 | 347.7 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 327.4 | 240.9 | 324.6 | 324.6 |
| static_for_tuned_BS | 140.3 | 140.3 | 163.4 | 162.7 |
| static_for_default_BS | 914.6 | 919.8 | 369.7 | 370.0 |

