void run_static_for_benchmarks();
void run_dynamic_for_benchmarks();
void run_dynamic_for_dispatch_benchmarks();
void run_dispatch_benchmarks();
void run_dispatch_optimization_benchmarks();
void run_switch_vs_fptr_benchmarks();

auto main() -> int {
    run_static_for_benchmarks();
    run_dynamic_for_benchmarks();
    run_dynamic_for_dispatch_benchmarks();
    run_dispatch_benchmarks();
    run_dispatch_optimization_benchmarks();
    run_switch_vs_fptr_benchmarks();
    return 0;
}
