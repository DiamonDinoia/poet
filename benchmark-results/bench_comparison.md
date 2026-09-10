# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.7 | 1.0 | 0.9 |
| switch | 0.6 | 0.6 | 1.1 | 0.9 |
| fn_ptr | 9.3 | 9.3 | 9.5 | 9.4 |
| POET | 9.3 | 9.3 | 9.3 | 9.4 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4096.5 | 4112.6 | 4059.5 | 4042.7 |
| saxpy_aligned | 4103.1 | 4105.0 | 4046.1 | 4040.9 |
| saxpy_restrict | 4115.3 | 4123.4 | 4040.1 | 4039.0 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.7 | 60.1 | 80.1 | 80.1 |
| tuned-acc_N=64 | 54.6 | 46.7 | 51.1 | 50.8 |
| dynamic_for_N=64 | 76.0 | 50.9 | 44.4 | 42.2 |
| 1-acc_N=512 | 629.0 | 482.2 | 646.2 | 642.9 |
| tuned-acc_N=512 | 410.6 | 348.7 | 375.4 | 373.6 |
| dynamic_for_N=512 | 580.7 | 385.4 | 314.8 | 296.5 |
| 1-acc_N=4096 | 5013.5 | 3859.0 | 5046.3 | 5235.2 |
| tuned-acc_N=4096 | 3259.2 | 2766.0 | 2962.6 | 2942.7 |
| dynamic_for_N=4096 | 4624.9 | 3057.0 | 2476.6 | 2335.5 |
| 1-acc_N=32768 | 40087.7 | 30877.3 | 40923.4 | 40824.1 |
| tuned-acc_N=32768 | 25982.5 | 22097.6 | 23635.5 | 23701.3 |
| dynamic_for_N=32768 | 36899.3 | 24428.1 | 19757.3 | 18622.0 |

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
| 1D_contiguous_hit | 9.5 | 9.5 | 9.3 | 9.3 |
| 1D_contiguous_miss | 0.6 | 0.6 | 0.9 | 0.9 |
| 1D_non-contiguous_hit | 5.1 | 4.8 | 4.7 | 4.7 |
| 1D_non-contiguous_miss | 2.7 | 2.7 | 3.4 | 3.4 |
| 2D_contiguous_hit | 10.0 | 10.0 | 9.6 | 9.6 |
| 2D_contiguous_miss | 0.9 | 0.9 | 1.3 | 1.2 |
| 2D_non-contiguous_hit | 7.8 | 7.5 | 8.9 | 8.7 |
| 2D_non-contiguous_miss | 5.4 | 5.7 | 6.5 | 6.6 |
| 5D_contiguous_hit | 2.6 | 2.6 | 2.6 | 2.6 |
| 5D_contiguous_miss | 0.6 | 0.3 | 0.6 | 0.6 |
| 5D_non-contiguous_hit | 8.9 | 10.1 | 6.8 | 6.8 |
| 5D_non-contiguous_miss | 0.3 | 0.7 | 0.6 | 0.6 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 2.1 | 3.1 | 2.5 | 2.5 |
| N=4_dispatched | 0.9 | 0.8 | 0.8 | 0.8 |
| N=8_runtime | 3.1 | 3.4 | 3.3 | 3.3 |
| N=8_dispatched | 1.4 | 1.6 | 1.4 | 1.4 |
| N=16_runtime | 4.6 | 5.2 | 5.1 | 5.1 |
| N=16_dispatched | 3.1 | 3.1 | 3.0 | 3.0 |
| N=32_runtime | 12.3 | 12.2 | 10.0 | 10.0 |
| N=32_dispatched | 9.2 | 9.2 | 9.2 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12222.2 | 9433.5 | 12491.5 | 12470.8 |
| for_loop_optimal_accs | 11215.0 | 7486.0 | 7263.8 | 6652.3 |
| dynamic_for_1_acc | 12290.8 | 9421.0 | 12481.2 | 12488.7 |
| dynamic_for_optimal_accs | 7949.3 | 6913.6 | 6233.3 | 5692.6 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12245.2 | 9498.5 | 12474.8 | 12462.5 |
| dynamic_for_optimal | 7951.6 | 6932.0 | 6230.3 | 5690.9 |
| dynamic_for_spill | 6919.0 | 7156.1 | 5440.7 | 5315.4 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11212.3 | 7493.3 | 7222.4 | 6644.5 |
| computed-index | 11241.0 | 7489.5 | 7215.2 | 6641.8 |
| dynamic_for_lane_form | 11272.3 | 7471.4 | 6038.1 | 5689.8 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 7858.6 | 5805.3 | 3953.1 | 4070.2 |
| computed-index | 7798.5 | 5811.7 | 3948.2 | 4073.0 |
| dynamic_for_lane_form | 8183.8 | 5783.3 | 4410.4 | 4564.6 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5635.2 | 3737.9 | 3020.9 | 2848.3 |
| dynamic_for_RT_stride_2 | 5622.0 | 3735.6 | 3020.8 | 2845.4 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12244.2 | 9424.6 | 12484.8 | 12474.1 |
| dynamic_for_index_only_1_acc | 11217.0 | 11334.5 | 11633.1 | 11647.4 |
| dynamic_for_lane_form_optimal_accs | 11277.9 | 7470.4 | 6038.1 | 5687.4 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5366.1 | 5324.8 | 4475.1 | 4456.3 |
| dynamic_for_index_only | 11685.2 | 9195.2 | 6185.4 | 6223.6 |
| dynamic_for_lane_form_unused_lane | 11693.5 | 9187.3 | 6180.6 | 6229.1 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.4 | 2.9 | 2.9 |
| dynamic_for_index_only_N=3 | 3.1 | 2.6 | 3.2 | 3.2 |
| dynamic_for_lane_form_N=3 | 2.8 | 2.6 | 3.2 | 3.2 |
| plain_for_N=7 | 7.4 | 7.3 | 8.0 | 8.0 |
| dynamic_for_index_only_N=7 | 6.8 | 6.8 | 8.6 | 8.5 |
| dynamic_for_lane_form_N=7 | 6.8 | 6.9 | 8.5 | 8.6 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 391.0 | 388.3 | 359.7 | 359.9 |
| static_for_tuned_BS | 353.2 | 352.4 | 361.0 | 360.1 |
| static_for_default_BS | 826.9 | 790.5 | 347.9 | 348.1 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 318.3 | 241.4 | 324.8 | 325.9 |
| static_for_tuned_BS | 140.2 | 140.9 | 162.9 | 162.8 |
| static_for_default_BS | 915.5 | 913.5 | 370.4 | 370.4 |

