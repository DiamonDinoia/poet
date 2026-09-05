/// \file exact_unroll_check.cpp
/// \brief Proves `dynamic_for<U>` emits its body exactly `U` times per back-edge.
///
/// Compiles `exact_unroll_fixture.cpp` to assembly with this build's own
/// compiler, over several optimization and unrolling flag sets, then reads the
/// listing back: for every fixture function it locates each loop by its
/// back-edge (a branch whose target label is defined above it), counts the FMAs
/// inside, and requires `U`.
///
/// The reader knows no ISA and no assembler dialect beyond three shapes: a
/// label definition, a branch, and an FMA. Any flag the compiler rejects is
/// probed first and the cell is reported as skipped, never silently dropped.
///
/// Usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Case {
    std::string name;
    int expect;// FMAs per back-edge, or, at `backedges == 0`, FMAs in the whole
               // function; -1 for report-only
    int backedges;// required number of back-edges, or -1 for report-only
    int branches;// required branch count, or -1 to leave it unclaimed
};

/// What one function's listing says: its loops, one FMA count per back-edge,
/// plus the totals a straight-line claim needs.
struct Profile {
    std::vector<int> loops;
    int fmas = 0;
    int branches = 0;
    bool found = false;
};

auto quoted(const std::string &text) -> std::string { return "\"" + text + "\""; }

/// Everything before the first assembler comment marker. clang appends the
/// evaluated expression to arithmetic lines and a loop-depth note to loop
/// headers, either of which would otherwise be scanned for label names.
auto strip_comment(const std::string &line) -> std::string {
    const std::size_t hash = line.find('#');
    const std::size_t slash = line.find("//");
    const std::size_t semi = line.find(';');
    const std::size_t at = line.find('@');
    std::size_t cut = std::min(std::min(hash, slash), std::min(semi, at));
    return cut == std::string::npos ? line : line.substr(0, cut);
}

auto first_token(const std::string &line) -> std::string {
    std::size_t begin = line.find_first_not_of(" \t");
    if (begin == std::string::npos) { return {}; }
    const std::size_t end = line.find_first_of(" \t", begin);
    return line.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

auto is_call(const std::string &mnemonic) -> bool {
    static const std::regex call_op(R"(^(call|callq|bl|blx|jal|bctrl)$)", std::regex::icase);
    return std::regex_match(mnemonic, call_op);
}

/// Takes its target unconditionally, so control never falls through.
auto is_unconditional(const std::string &mnemonic) -> bool {
    static const std::regex uncond_op(R"(^(jmp[a-z]*|b|j|br|bra)$)", std::regex::icase);
    return std::regex_match(mnemonic, uncond_op);
}

auto is_fma(const std::string &mnemonic, const std::string &operands) -> bool {
    // x86 `vfmadd132sd` / `vfnmsub213pd`, AArch64 `fmadd` `fmla`, RISC-V
    // `fmadd.d`, Power `xsmaddadp`. A target with no FMA instruction reaches
    // libm instead, and that call is the body's one FMA.
    static const std::regex fma_op(R"(^(v?fn?m(add|sub)|fml[as]|x[sv]n?m(add|sub)))", std::regex::icase);
    static const std::regex fma_callee(R"((^|[^A-Za-z0-9_])_?fma([^A-Za-z0-9_]|$))");
    if (std::regex_search(mnemonic, fma_op)) { return true; }
    // A tail call reaches libm by jumping, not calling. The last body of a
    // straight-line block is exactly where that happens.
    return (is_call(mnemonic) || is_unconditional(mnemonic)) && std::regex_search(operands, fma_callee);
}

auto is_branch(const std::string &mnemonic) -> bool {
    // x86 `jne`, AArch64 `b.ne` `cbnz` `tbz`, Power `bdnz`, RISC-V `beq` `j`.
    // A non-branch that happens to start with `b` (`bic`, `bfi`) is filtered
    // out by the caller: only an operand naming a label of this function counts.
    static const std::regex branch_op(R"(^([jb][A-Za-z0-9_.]*|cbn?z|tbn?z)$)", std::regex::icase);
    return std::regex_match(mnemonic, branch_op);
}

/// Ends the function, so nothing after it is reachable by fall-through.
/// Without this a compiler's backward jump into a shared epilogue reads as a
/// loop: every instruction below it falls through to the jump on paper.
auto is_return(const std::string &mnemonic) -> bool {
    static const std::regex ret_op(R"(^(ret[a-z]*|blr|bctr|jr|ud2|hlt|brk|trap)$)", std::regex::icase);
    return std::regex_match(mnemonic, ret_op);
}

/// The FMA count of every loop in `name`, one entry per back-edge.
///
/// A back-edge is a branch to a label defined above it that the label can reach
/// again. The reachability walk is what separates a loop from a compiler's
/// backward jump into a shared epilogue, which is not a cycle.
/// The half-open line range of `name`'s body, empty when the listing has no
/// such symbol. The name is matched literally: a mangled callee carries `$` and
/// `.`, which a regex would read as anchors and wildcards.
auto function_span(const std::vector<std::string> &lines, const std::string &name)
  -> std::pair<std::size_t, std::size_t> {
    std::size_t begin = lines.size();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string token = first_token(lines[i]);
        if (token == name + ":" || token == "_" + name + ":") {
            begin = i;
            break;
        }
    }
    if (begin == lines.size()) { return { begin, begin }; }

    std::size_t end = lines.size();
    for (std::size_t i = begin + 1; i < lines.size(); ++i) {
        const std::string token = first_token(lines[i]);
        if (token == ".size" || token == ".cfi_endproc") {
            end = i;
            break;
        }
    }
    return { begin, end };
}

/// Every symbol the listing declares a function, so a call operand can be told
/// from a libm name this file does not define.
auto function_symbols(const std::vector<std::string> &lines) -> std::vector<std::string> {
    std::vector<std::string> out;
    for (const std::string &line : lines) {
        if (first_token(line) != ".type") { continue; }
        const std::size_t comma = line.find(',');
        if (comma == std::string::npos) { continue; }
        const std::size_t begin = line.find(".type") + 5;
        std::string name = line.substr(begin, comma - begin);
        const std::size_t first = name.find_first_not_of(" \t");
        const std::size_t last = name.find_last_not_of(" \t");
        if (first == std::string::npos) { continue; }
        out.push_back(name.substr(first, last - first + 1));
    }
    return out;
}

/// The functions `name` calls that this same listing defines. The outlined tail
/// is the one that matters: `tail_binary` is a compile-time recursion, so a
/// back-edge in there means the tail became a loop.
auto callees_of(const std::vector<std::string> &lines, const std::string &name, const std::vector<std::string> &symbols)
  -> std::vector<std::string> {
    static const std::regex token_re(R"([A-Za-z_.$][A-Za-z0-9_.$]*)");
    const auto bounds = function_span(lines, name);
    std::vector<std::string> out;
    for (std::size_t i = bounds.first; i < bounds.second; ++i) {
        const std::string text = strip_comment(lines[i]);
        const std::string word = first_token(text);
        if (word.empty() || !is_call(word)) { continue; }
        const std::string rest = text.substr(text.find(word) + word.size());
        for (auto it = std::sregex_iterator(rest.begin(), rest.end(), token_re); it != std::sregex_iterator(); ++it) {
            const std::string target = it->str();
            if (target == name) { continue; }
            if (std::find(symbols.begin(), symbols.end(), target) == symbols.end()) { continue; }
            if (std::find(out.begin(), out.end(), target) == out.end()) { out.push_back(target); }
        }
    }
    return out;
}

auto profile_function(const std::vector<std::string> &lines, const std::string &name) -> Profile {
    Profile out;
    const auto bounds = function_span(lines, name);
    const std::size_t begin = bounds.first;
    const std::size_t end = bounds.second;
    out.found = begin < lines.size();
    if (!out.found) { return out; }

    static const std::regex label_def(R"(^([A-Za-z_.$][A-Za-z0-9_.$]*):)");
    static const std::regex token_re(R"([A-Za-z_.$][A-Za-z0-9_.$]*)");

    const std::size_t span = end - begin;
    std::vector<std::string> mnemonic(span);
    std::vector<std::string> operands(span);
    std::vector<char> fma(span, 0);
    std::map<std::string, std::size_t> labels;

    for (std::size_t i = begin; i < end; ++i) {
        const std::string text = strip_comment(lines[i]);
        std::smatch match;
        if (std::regex_search(text, match, label_def)) { labels.emplace(match[1].str(), i - begin); }
        const std::string word = first_token(text);
        if (word.empty() || word.front() == '.' || word.back() == ':') { continue; }
        mnemonic[i - begin] = word;
        operands[i - begin] = text.substr(text.find(word) + word.size());
        fma[i - begin] = is_fma(word, operands[i - begin]) ? 1 : 0;
    }

    // Fall-through successor of every position: the next line carrying an
    // instruction, so labels and directives are transparent.
    std::vector<std::size_t> next(span, span);
    std::size_t following = span;
    for (std::size_t back = span; back > 0; --back) {
        const std::size_t i = back - 1;
        next[i] = following;
        if (!mnemonic[i].empty()) { following = i; }
    }

    const auto targets_of = [&](std::size_t i) {
        std::vector<std::size_t> targets;
        const std::string &text = operands[i];
        for (auto it = std::sregex_iterator(text.begin(), text.end(), token_re); it != std::sregex_iterator(); ++it) {
            const auto label = labels.find(it->str());
            if (label != labels.end()) { targets.push_back(next[label->second]); }
        }
        return targets;
    };

    const auto reaches = [&](std::size_t from, std::size_t goal) {
        std::vector<char> seen(span, 0);
        std::vector<std::size_t> stack{ from };
        while (!stack.empty()) {
            const std::size_t at = stack.back();
            stack.pop_back();
            if (at >= span || seen[at] != 0) { continue; }
            seen[at] = 1;
            if (at == goal) { return true; }
            if (is_return(mnemonic[at])) { continue; }
            const std::vector<std::size_t> jumps = targets_of(at);
            for (const std::size_t to : jumps) { stack.push_back(to); }
            if (jumps.empty() || !is_unconditional(mnemonic[at])) { stack.push_back(next[at]); }
        }
        return false;
    };

    for (std::size_t i = 0; i < span; ++i) { out.fmas += fma[i]; }

    for (std::size_t i = 0; i < span; ++i) {
        if (mnemonic[i].empty() || !is_branch(mnemonic[i])) { continue; }
        const std::string &text = operands[i];
        bool names_label = false;
        for (auto it = std::sregex_iterator(text.begin(), text.end(), token_re); it != std::sregex_iterator(); ++it) {
            const auto label = labels.find(it->str());
            if (label == labels.end()) { continue; }
            names_label = true;
            if (label->second > i) { continue; }
            const std::size_t head = next[label->second];
            if (head > i || !reaches(head, i)) { continue; }
            int count = 0;
            for (std::size_t k = head; k <= i; ++k) { count += fma[k]; }
            out.loops.push_back(count);
            break;
        }
        // A mnemonic that merely starts with `b` (`bic`, `bfi`) is not a branch;
        // an operand naming a label of this function is what makes it one.
        if (names_label) { ++out.branches; }
    }
    return out;
}

auto run(const std::string &command) -> int { return std::system(command.c_str()); }

}// namespace

auto main(int argc, char **argv) -> int {
    if (argc != 5) {
        std::cerr << "usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir>\n";
        return 2;
    }
    const std::string compiler = argv[1];
    const std::string fixture = argv[2];
    const std::string include_dir = argv[3];
    const std::string work_dir = argv[4];

    const std::string probe = work_dir + "/probe.cpp";
    const std::string log = work_dir + "/compile.log";
    std::filesystem::create_directories(work_dir);
    {
        std::ofstream out(probe);
        out << "int main() { return 0; }\n";
    }
    if (!std::ifstream(probe)) {
        std::cerr << "exact-unroll: cannot write into " << work_dir << "\n";
        return 2;
    }

    // Every flag set that changes whether an unroller runs. `-march` matters
    // because a target without FMA hides the body behind a libm call.
    const std::vector<std::string> flag_sets = { "-O2",
        "-O3",
        "-O3 -funroll-loops",
        "-O3 -funroll-all-loops",
        "-O3 -march=native",
        "-O3 -funroll-loops -march=native",
        "-O3 -funroll-loops -march=x86-64-v3" };
    const std::vector<std::string> standards = { "c++17", "c++20" };

    // `branches` is claimed only where straight-line code is the contract: a
    // loop's own back-edge is a branch, so a case that keeps a loop cannot
    // budget them.
    const std::vector<Case> cases = { { "sf1_runtime", 1, 1, -1 },
        { "sf2_runtime", 2, 1, -1 },
        { "sf4_runtime", 4, 1, -1 },
        { "sf8_runtime", 8, 1, -1 },
        { "sf1_const", 1, 1, -1 },
        { "sf2_const", 2, 1, -1 },
        { "sf4_const", 4, 1, -1 },
        { "sf8_const", 8, 1, -1 },
        { "sf2_small", 2, 1, -1 },
        { "sf4_small", 4, 1, -1 },
        { "sf2_one", 2, 0, 0 },
        { "sf4_one", 4, 0, 0 },
        { "sf8_one", 8, 0, 0 },
        { "control_barrier4", 4, 1, -1 },
        { "control_naked1", -1, -1, -1 } };

    int failures = 0;
    int checked = 0;
    int skipped = 0;

    for (const std::string &standard : standards) {
        for (const std::string &flags : flag_sets) {
            const std::string cell = standard + " " + flags;
            const std::string base = "-std=" + standard + " " + flags;
            const std::string probe_cmd = quoted(compiler) + " " + base + " -Werror -c -o "
                                          + quoted(work_dir + "/probe.o") + " " + quoted(probe) + " > " + quoted(log)
                                          + " 2>&1";
            if (run(probe_cmd) != 0) {
                std::cout << "exact-unroll SKIP  [" << cell << "] compiler rejects these flags\n";
                ++skipped;
                continue;
            }

            const std::string asm_path = work_dir + "/fixture.s";
            const std::string build = quoted(compiler) + " " + base + " -DPOET_EXACT_UNROLL_CONTROL -S -o "
                                      + quoted(asm_path) + " -I" + quoted(include_dir) + " " + quoted(fixture) + " > "
                                      + quoted(log) + " 2>&1";
            std::cout << "exact-unroll CELL  [" << cell << "] " << build << "\n";
            if (run(build) != 0) {
                // The log lives in a build tree CI does not keep, so it goes to stdout.
                std::cout << "exact-unroll FAIL  [" << cell << "] fixture did not compile:\n" << std::ifstream(log).rdbuf()
                          << "\n";
                ++failures;
                continue;
            }

            std::vector<std::string> lines;
            {
                std::ifstream in(asm_path);
                std::string line;
                while (std::getline(in, line)) { lines.push_back(line); }
            }

            const std::vector<std::string> symbols = function_symbols(lines);

            for (const Case &item : cases) {
                const Profile body = profile_function(lines, item.name);
                if (!body.found) {
                    std::cout << "exact-unroll FAIL  [" << cell << "] " << item.name << " missing from the listing\n";
                    ++failures;
                    continue;
                }
                int widest = 0;
                for (const int fmas : body.loops) { widest = std::max(widest, fmas); }
                // A case claiming no back-edge has no per-loop count to read, so
                // its `expect` is the whole function's body instead.
                const int reading = item.backedges == 0 ? body.fmas : widest;
                // The outlined tail must stay a branch tree. A loop there means
                // the tail stopped folding, which is the other half of the
                // contract: the main loop is exact, the tail is straight-line.
                int tail_loops = 0;
                for (const std::string &callee : callees_of(lines, item.name, symbols)) {
                    tail_loops += static_cast<int>(profile_function(lines, callee).loops.size());
                }
                const bool report_only = item.expect < 0;
                const bool ok = report_only
                                || (static_cast<int>(body.loops.size()) == item.backedges && reading == item.expect
                                    && tail_loops == 0 && (item.branches < 0 || body.branches == item.branches));
                std::cout << "exact-unroll " << (report_only ? "NOTE " : (ok ? "PASS " : "FAIL ")) << " [" << cell
                          << "] " << item.name << " back-edges=" << body.loops.size() << " fma=" << reading
                          << " branches=" << body.branches << " tail-loops=" << tail_loops;
                if (report_only) {
                    std::cout << " (no barrier: the flag set unrolls a one-FMA loop by " << widest << ")\n";
                } else {
                    std::cout << " expect back-edges=" << item.backedges << " fma=" << item.expect << " branches=";
                    if (item.branches < 0) {
                        std::cout << "any";
                    } else {
                        std::cout << item.branches;
                    }
                    std::cout << " tail-loops=0\n";
                    ++checked;
                    if (!ok) { ++failures; }
                }
            }
        }
    }

    std::cout << "exact-unroll: " << checked << " claims checked, " << failures << " failed, " << skipped
              << " flag sets skipped\n";
    if (checked == 0) {
        std::cout << "exact-unroll: nothing was checked, which is a broken check, not a clean one\n";
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
