# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.7 | 0.9 | 1.0 |
| switch | 0.6 | 0.6 | 0.9 | 0.9 |
| fn_ptr | 9.6 | 9.6 | 9.3 | 9.3 |
| POET | 8.8 | 8.7 | 9.5 | 9.6 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4153.0 | 4141.7 | 4034.7 | 4038.2 |
| saxpy_aligned | 4151.8 | 4138.9 | 4041.1 | 4037.8 |
| saxpy_restrict | 4151.7 | 4148.7 | 4038.0 | 4036.0 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.7 | 60.0 | 81.9 | 80.0 |
| tuned-acc_N=64 | 54.3 | 46.9 | 50.9 | 51.1 |
| dynamic_for_N=64 | 75.3 | 46.5 | 50.1 | 46.3 |
| 1-acc_N=512 | 629.1 | 482.1 | 666.6 | 646.7 |
| tuned-acc_N=512 | 411.0 | 348.5 | 373.3 | 375.3 |
| dynamic_for_N=512 | 579.1 | 382.4 | 373.9 | 342.8 |
| 1-acc_N=4096 | 5009.5 | 3857.2 | 5110.8 | 5050.8 |
| tuned-acc_N=4096 | 3257.2 | 2762.3 | 2941.1 | 2959.4 |
| dynamic_for_N=4096 | 4607.4 | 3050.4 | 2963.5 | 2715.8 |
| 1-acc_N=32768 | 40122.5 | 30858.4 | 41640.8 | 40972.8 |
| tuned-acc_N=32768 | 25971.8 | 22084.8 | 23521.1 | 23609.7 |
| dynamic_for_N=32768 | 36897.4 | 24413.0 | 23686.3 | 21672.7 |

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
| 1D_non-contiguous_hit | 4.9 | 4.6 | 4.8 | 4.8 |
| 1D_non-contiguous_miss | 2.7 | 2.6 | 3.1 | 3.1 |
| 2D_contiguous_hit | 9.9 | 9.3 | 9.6 | 9.6 |
| 2D_contiguous_miss | 0.6 | 0.6 | 1.2 | 1.2 |
| 2D_non-contiguous_hit | 9.8 | 10.3 | 9.0 | 9.0 |
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
| N=8_runtime | 3.1 | 3.4 | 3.3 | 3.4 |
| N=8_dispatched | 1.4 | 1.6 | 1.4 | 1.4 |
| N=16_runtime | 4.6 | 5.2 | 5.0 | 5.1 |
| N=16_dispatched | 3.1 | 3.1 | 3.0 | 2.9 |
| N=32_runtime | 12.3 | 12.2 | 10.0 | 10.0 |
| N=32_dispatched | 9.2 | 9.2 | 9.1 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12280.2 | 9418.8 | 12481.2 | 12473.6 |
| for_loop_optimal_accs | 11229.1 | 7484.5 | 7230.7 | 6635.9 |
| dynamic_for_1_acc | 12257.8 | 9417.9 | 12487.0 | 12512.3 |
| dynamic_for_optimal_accs | 7946.1 | 6736.1 | 7175.4 | 6641.3 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12230.1 | 9417.5 | 12463.2 | 12472.4 |
| dynamic_for_optimal | 7944.5 | 6735.9 | 7185.8 | 6650.6 |
| dynamic_for_spill | 6946.3 | 7018.5 | 5596.7 | 5591.6 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11239.7 | 7493.7 | 7210.2 | 6637.1 |
| computed-index | 11357.0 | 7498.8 | 7219.4 | 6641.4 |
| dynamic_for_lane_form | 11222.5 | 7478.7 | 7233.4 | 6608.9 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 8176.3 | 5806.7 | 3952.5 | 4067.4 |
| computed-index | 8179.7 | 5821.5 | 3945.4 | 4068.5 |
| dynamic_for_lane_form | 8189.1 | 5807.4 | 3956.2 | 4067.7 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5645.3 | 3732.4 | 3618.2 | 3307.8 |
| dynamic_for_RT_stride_2 | 5651.3 | 3734.9 | 3618.2 | 3311.3 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12299.2 | 9421.4 | 12463.2 | 12490.3 |
| dynamic_for_index_only_1_acc | 11249.1 | 11216.4 | 11870.8 | 11803.3 |
| dynamic_for_lane_form_optimal_accs | 11231.4 | 7480.9 | 7228.8 | 6609.8 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5365.8 | 5330.5 | 4459.1 | 4451.4 |
| dynamic_for_index_only | 11588.9 | 9137.1 | 6213.2 | 6213.2 |
| dynamic_for_lane_form_unused_lane | 11620.6 | 9124.5 | 6214.5 | 6213.2 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.5 | 2.9 | 2.9 |
| dynamic_for_index_only_N=3 | 3.1 | 2.8 | 3.4 | 3.4 |
| dynamic_for_lane_form_N=3 | 2.8 | 2.8 | 3.4 | 3.4 |
| plain_for_N=7 | 7.3 | 7.3 | 8.0 | 8.0 |
| dynamic_for_index_only_N=7 | 6.9 | 6.8 | 8.6 | 8.6 |
| dynamic_for_lane_form_N=7 | 6.8 | 6.8 | 8.6 | 8.6 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 387.5 | 389.5 | 369.9 | 361.5 |
| static_for_tuned_BS | 354.7 | 354.6 | 365.3 | 363.3 |
| static_for_default_BS | 827.8 | 785.3 | 347.9 | 349.3 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 327.8 | 241.4 | 324.9 | 324.2 |
| static_for_tuned_BS | 140.5 | 140.9 | 162.7 | 163.8 |
| static_for_default_BS | 923.9 | 920.6 | 377.3 | 371.9 |

