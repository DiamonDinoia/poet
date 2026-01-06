void run_static_for_benchmarks();
void run_dynamic_for_benchmarks();
void run_dispatch_benchmarks();

auto main() -> int {
    run_static_for_benchmarks();
    run_dynamic_for_benchmarks();
    run_dispatch_benchmarks();
    return 0;
}
