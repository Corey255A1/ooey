#pragma once

#include "tooey/ast.hpp"
#include <string>

namespace tooey {

struct CodegenResult {
    std::string header;
    std::string source;
};

class CodeGenerator {
public:
    static CodegenResult generate(
        const std::shared_ptr<AstNode>& ast, 
        const std::string& class_name, 
        const std::string& view_model_class
    );
};

} // namespace tooey
