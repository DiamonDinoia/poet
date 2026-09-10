/// \file exact_unroll_check.cpp
/// \brief Checks that `dynamic_for<U>` emits its body exactly `U` times.
///
/// Compiles `exact_unroll_fixture.cpp` to assembly with the build's compiler under
/// three flag sets and counts the FMAs inside each fixture function, instruction or libm call.
/// A one-block range must also carry no branch. The unguarded loop `naked` is the
/// positive control: under `-funroll-loops` it must read more than one FMA.
///
/// Usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir> [compiler-arg...]

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {

struct Case {
    const char *name;
    int bodies;
    bool straight;// no branch allowed
};

constexpr Case cases[] = { { "u1_blocks", 1, false },
    { "u2_blocks", 2, false },
    { "u4_blocks", 4, false },
    { "u8_blocks", 8, false },
    { "u2_tail", 2, false },
    { "u4_tail", 4, false },
    { "u2_one", 2, true },
    { "u4_one", 4, true },
    { "u8_one", 8, true } };

struct Count {
    bool found = false;
    int bodies = 0;
    int branches = 0;
};

/// FMAs and branches between `name:` and its `.cfi_endproc`.
/// Instructions are indented; labels and directives are not counted.
auto count(const std::vector<std::string> &lines, const std::string &name) -> Count {
    static const std::regex fma(R"(fn?m(add|sub)|fml[as]|[\s,]_?fma\b)");
    static const std::regex branch(R"(^\s+(j[a-z]*|b|b\.[a-z]+|cbn?z|tbn?z)\s)");
    Count out;
    auto line = lines.begin();
    for (; line != lines.end(); ++line) {
        if (line->rfind(name + ":", 0) == 0 || line->rfind("_" + name + ":", 0) == 0) { break; }
    }
    if (line == lines.end()) { return out; }
    out.found = true;
    for (++line; line != lines.end() && line->find(".cfi_endproc") == std::string::npos; ++line) {
        const std::size_t first = line->find_first_not_of(" \t");
        if (first == 0 || first == std::string::npos || (*line)[first] == '.') { continue; }
        if (std::regex_search(*line, fma)) {
            ++out.bodies;
        } else if (std::regex_search(*line, branch)) {
            ++out.branches;
        }
    }
    return out;
}

auto quoted(const std::string &text) -> std::string { return "\"" + text + "\""; }

}// namespace

auto main(int argc, char **argv) -> int {
    if (argc < 5) {
        std::cerr << "usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir> [compiler-arg...]\n";
        return 2;
    }
    const std::string work_dir = argv[4];
    const std::string asm_path = work_dir + "/fixture.s";
    const std::string log = work_dir + "/compile.log";
    std::string command = quoted(argv[1]) + " -std=c++17 -S -o " + quoted(asm_path) + " -I" + quoted(argv[3]);
    for (int i = 5; i < argc; ++i) { command += " " + quoted(argv[i]); }
    command += " " + quoted(argv[2]) + " > " + quoted(log) + " 2>&1 ";
    std::filesystem::create_directories(work_dir);

    int failures = 0;
    for (const char *flags : { "-O2", "-O3", "-O3 -funroll-loops" }) {
        std::cout << "exact-unroll [" << flags << "] " << command << flags << "\n";
        if (std::system((command + flags).c_str()) != 0) {
            std::cout << "exact-unroll FAIL [" << flags << "] fixture did not compile:\n" << std::ifstream(log).rdbuf();
            ++failures;
            continue;
        }
        std::vector<std::string> lines;
        std::ifstream in(asm_path);
        for (std::string line; std::getline(in, line);) { lines.push_back(line); }

        for (const Case &item : cases) {
            const Count got = count(lines, item.name);
            const bool ok = got.found && got.bodies == item.bodies && (!item.straight || got.branches == 0);
            std::cout << "exact-unroll " << (ok ? "PASS" : "FAIL") << " [" << flags << "] " << item.name
                      << " bodies=" << got.bodies << " branches=" << got.branches << " expect bodies=" << item.bodies
                      << (item.straight ? " branches=0" : "") << (got.found ? "" : " (missing)") << "\n";
            failures += ok ? 0 : 1;
        }
        // The control proves the counter sees an unrolled loop where one exists.
        const Count naked = count(lines, "naked");
        const bool unrolling = std::string(flags).find("unroll") != std::string::npos;
        const bool ok = naked.found && (!unrolling || naked.bodies > 1);
        std::cout << "exact-unroll " << (ok ? "PASS" : "FAIL") << " [" << flags << "] naked bodies=" << naked.bodies
                  << (unrolling ? " expect bodies>1 (control)" : " (control, informative)") << "\n";
        failures += ok ? 0 : 1;
    }
    std::cout << "exact-unroll: " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}
