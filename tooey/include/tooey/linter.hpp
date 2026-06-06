#pragma once

#include "tooey/ast.hpp"
#include "ooey/types.hpp"
#include <string>
#include <vector>
#include <set>

namespace tooey {

struct Diagnostic {
    int line;
    int start_col;
    int end_col;
    ooey::Color color;
};

class Linter {
public:
    static std::vector<Diagnostic> run_diagnostics(const std::string& dsl);

    static void register_known_type(const std::string& type_name);
    static void reset_known_types();

private:
    static std::set<std::string> known_types_;
    static bool initialized_;
    static void init_default_known_types();
};

} // namespace tooey
