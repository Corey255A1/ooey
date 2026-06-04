#pragma once

#include "tooey/ast.hpp"
#include "tooey/lexer.hpp"
#include <memory>
#include <vector>

namespace tooey {

class Parser {
public:
    static std::shared_ptr<AstNode> parse(const std::vector<Token>& tokens, const std::string& current_directory = "");
};

} // namespace tooey
