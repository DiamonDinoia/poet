# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.7 | 1.8 | 1.0 |
| switch | 0.6 | 0.7 | 1.8 | 0.9 |
| fn_ptr | 9.4 | 10.2 | 2.0 | 9.5 |
| POET | 9.3 | 10.5 | 1.9 | 9.4 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4121.5 | 4502.8 | 4970.1 | 4040.9 |
| saxpy_aligned | 4120.3 | 4500.7 | 4965.8 | 4041.7 |
| saxpy_restrict | 4111.9 | 4506.3 | 4963.6 | 4040.6 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.9 | 64.8 | 107.4 | 81.3 |
| tuned-acc_N=64 | 54.7 | 47.7 | 47.9 | 50.8 |
| dynamic_for_N=64 | 76.3 | 50.9 | 46.5 | 46.3 |
| 1-acc_N=512 | 635.7 | 535.4 | 867.1 | 641.6 |
| tuned-acc_N=512 | 411.0 | 362.7 | 350.4 | 373.4 |
| dynamic_for_N=512 | 580.5 | 424.2 | 352.4 | 343.2 |
| 1-acc_N=4096 | 5015.1 | 4376.5 | 6880.3 | 5230.5 |
| tuned-acc_N=4096 | 3260.8 | 2878.0 | 2769.8 | 2944.0 |
| dynamic_for_N=4096 | 4607.0 | 3381.5 | 2800.9 | 2711.9 |
| 1-acc_N=32768 | 40117.4 | 34545.6 | 54937.8 | 40969.6 |
| tuned-acc_N=32768 | 26058.6 | 22970.8 | 22127.8 | 23515.7 |
| dynamic_for_N=32768 | 36743.8 | 27075.3 | 22406.8 | 21667.6 |

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
| 1D_contiguous_hit | 9.4 | 10.5 | 1.8 | 9.3 |
| 1D_contiguous_miss | 0.6 | 0.7 | 1.9 | 0.9 |
| 1D_non-contiguous_hit | 4.6 | 5.1 | 4.7 | 4.8 |
| 1D_non-contiguous_miss | 2.7 | 2.5 | 3.2 | 3.1 |
| 2D_contiguous_hit | 10.0 | 11.3 | 3.7 | 10.0 |
| 2D_contiguous_miss | 0.9 | 1.1 | 2.0 | 1.2 |
| 2D_non-contiguous_hit | 10.3 | 9.5 | 8.8 | 9.2 |
| 2D_non-contiguous_miss | 5.3 | 6.0 | 6.4 | 6.4 |
| 5D_contiguous_hit | 2.6 | 2.9 | 1.9 | 2.6 |
| 5D_contiguous_miss | 0.3 | 0.4 | 2.0 | 0.6 |
| 5D_non-contiguous_hit | 13.8 | 10.0 | 6.6 | 7.0 |
| 5D_non-contiguous_miss | 0.6 | 0.4 | 1.9 | 0.6 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 2.1 | 2.5 | 3.2 | 2.5 |
| N=4_dispatched | 0.9 | 0.9 | 0.8 | 0.8 |
| N=8_runtime | 3.0 | 3.9 | 3.3 | 3.3 |
| N=8_dispatched | 1.4 | 1.6 | 2.0 | 1.4 |
| N=16_runtime | 4.6 | 5.0 | 4.9 | 5.1 |
| N=16_dispatched | 3.1 | 3.4 | 3.1 | 3.0 |
| N=32_runtime | 12.3 | 11.9 | 11.8 | 11.5 |
| N=32_dispatched | 9.2 | 10.2 | 8.9 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12282.0 | 10537.8 | 16789.6 | 12489.4 |
| for_loop_optimal_accs | 11226.2 | 8293.6 | 6791.7 | 6638.7 |
| dynamic_for_1_acc | 12236.2 | 10540.8 | 16792.4 | 12482.6 |
| dynamic_for_optimal_accs | 7959.9 | 7081.9 | 6761.2 | 6642.8 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12241.8 | 10537.7 | 17043.4 | 12466.4 |
| dynamic_for_optimal | 7957.4 | 7076.6 | 6783.0 | 6643.7 |
| dynamic_for_spill | 6954.8 | 7503.3 | 5172.9 | 5594.3 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11246.1 | 8295.5 | 6789.9 | 6642.2 |
| computed-index | 11332.9 | 8280.8 | 6784.2 | 6647.8 |
| dynamic_for_lane_form | 11236.9 | 8300.4 | 6829.3 | 6618.2 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 8227.7 | 6772.2 | 3717.5 | 4066.0 |
| computed-index | 8382.8 | 6862.2 | 3721.0 | 4065.0 |
| dynamic_for_lane_form | 8187.2 | 6780.1 | 3723.6 | 4069.0 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5658.4 | 4126.5 | 3431.8 | 3306.6 |
| dynamic_for_RT_stride_2 | 5659.3 | 4129.4 | 3422.8 | 3311.9 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12508.6 | 10543.4 | 16781.5 | 12491.4 |
| dynamic_for_index_only_1_acc | 11273.3 | 11444.8 | 15607.0 | 11825.2 |
| dynamic_for_lane_form_optimal_accs | 11227.9 | 8299.4 | 6831.7 | 6617.7 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5365.0 | 5073.9 | 4478.2 | 4454.9 |
| dynamic_for_index_only | 11563.6 | 7827.1 | 5890.3 | 6249.2 |
| dynamic_for_lane_form_unused_lane | 11563.6 | 7828.5 | 5863.1 | 6221.5 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.8 | 2.9 | 2.9 |
| dynamic_for_index_only_N=3 | 3.1 | 2.9 | 3.7 | 3.4 |
| dynamic_for_lane_form_N=3 | 2.9 | 3.0 | 3.7 | 3.4 |
| plain_for_N=7 | 7.4 | 7.4 | 8.0 | 8.0 |
| dynamic_for_index_only_N=7 | 6.8 | 7.2 | 9.0 | 8.6 |
| dynamic_for_lane_form_N=7 | 6.8 | 7.2 | 9.5 | 8.6 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 391.5 | 422.9 | 409.6 | 360.3 |
| static_for_tuned_BS | 353.3 | 374.1 | 407.9 | 361.1 |
| static_for_default_BS | 827.3 | 666.3 | 390.3 | 345.1 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 318.5 | 265.6 | 435.2 | 324.7 |
| static_for_tuned_BS | 140.2 | 152.9 | 469.0 | 162.8 |
| static_for_default_BS | 916.8 | 816.5 | 405.9 | 372.9 |

