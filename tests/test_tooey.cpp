#include <gtest/gtest.h>
#include "tooey/lexer.hpp"
#include "tooey/parser.hpp"
#include "tooey/codegen.hpp"
#include <iostream>

using namespace tooey;

TEST(TooeyLayout, LexerBasicTokenization) {
    std::string source = 
        "HBox id=rootLayout:\n"
        "    Button text=\"Submit\" onClick=@signal.execute\n"
        "    Label text=@binding.status\n"
        "    AI: \"dynamic text placement here\"\n";

    auto tokens = Lexer::tokenize(source);

    // Filter out EOF/newlines/comments to check meaningful tokens
    std::vector<Token> filtered;
    for (const auto& tok : tokens) {
        if (tok.type != TokenType::NEWLINE && tok.type != TokenType::END_OF_FILE && tok.type != TokenType::COMMENT) {
            filtered.push_back(tok);
        }
    }

    ASSERT_GE(filtered.size(), 8);
    
    // First token should be ELEMENT HBox
    EXPECT_EQ(filtered[0].type, TokenType::ELEMENT);
    EXPECT_EQ(filtered[0].text, "HBox");
    
    // ID Assignment
    EXPECT_EQ(filtered[1].type, TokenType::ID_ASSIGN);
    EXPECT_EQ(filtered[1].text, "rootLayout");

    // Colon
    EXPECT_EQ(filtered[2].type, TokenType::COLON);

    // Indent for the second line
    EXPECT_EQ(filtered[3].type, TokenType::INDENT);
    EXPECT_EQ(filtered[3].text.length(), 4);

    // Button
    EXPECT_EQ(filtered[4].type, TokenType::ELEMENT);
    EXPECT_EQ(filtered[4].text, "Button");

    // Inline keys and values
    EXPECT_EQ(filtered[5].type, TokenType::INLINE_KEY);
    EXPECT_EQ(filtered[5].text, "text");
    EXPECT_EQ(filtered[6].type, TokenType::EQUAL);
    EXPECT_EQ(filtered[6].text, "=");
    EXPECT_EQ(filtered[7].type, TokenType::STRING);
    EXPECT_EQ(filtered[7].text, "Submit");

    EXPECT_EQ(filtered[8].type, TokenType::INLINE_KEY);
    EXPECT_EQ(filtered[8].text, "onClick");
    EXPECT_EQ(filtered[9].type, TokenType::EQUAL);
    EXPECT_EQ(filtered[9].text, "=");
    EXPECT_EQ(filtered[10].type, TokenType::SIGNAL);
    EXPECT_EQ(filtered[10].text, "execute");
}

TEST(TooeyLayout, ParserAstGeneration) {
    std::string source = 
        "HBox id=rootLayout:\n"
        "    Button text=\"Submit\" onClick=@signal.execute\n"
        "    Label text=@binding.status\n"
        "    AI: \"dynamic text placement here\"\n";

    auto tokens = Lexer::tokenize(source);
    auto ast = Parser::parse(tokens);

    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->nodeType, "Root");
    
    ASSERT_EQ(ast->children.size(), 1);
    auto hbox = ast->children[0];
    EXPECT_EQ(hbox->nodeType, "HBox");
    EXPECT_EQ(hbox->id, "rootLayout");

    ASSERT_EQ(hbox->children.size(), 2);

    auto button = hbox->children[0];
    EXPECT_EQ(button->nodeType, "Button");
    EXPECT_EQ(button->properties["text"].type, PropertyType::String);
    EXPECT_EQ(button->properties["text"].rawData, "Submit");
    EXPECT_EQ(button->properties["onClick"].type, PropertyType::Signal);
    EXPECT_EQ(button->properties["onClick"].rawData, "execute");

    auto label = hbox->children[1];
    EXPECT_EQ(label->nodeType, "Label");
    EXPECT_EQ(label->properties["text"].type, PropertyType::Binding);
    EXPECT_EQ(label->properties["text"].rawData, "status");

    // AI block is registered on parent node (HBox) according to specification
    EXPECT_EQ(hbox->aiHint, "dynamic text placement here");
}

TEST(TooeyLayout, CodeGeneratorCppOutput) {
    std::string source = 
        "HBox id=rootLayout:\n"
        "    Button text=\"Submit\" onClick=@signal.execute\n"
        "    Label text=@binding.status\n";

    auto tokens = Lexer::tokenize(source);
    auto ast = Parser::parse(tokens);

    auto result = CodeGenerator::generate(ast, "MyTestView", "MyTestViewModel");

    // Inspect header output
    EXPECT_NE(result.header.find("class MyTestView : public gooey::Row"), std::string::npos);
    EXPECT_NE(result.header.find("std::shared_ptr<gooey::Row> rootLayout;"), std::string::npos);
    EXPECT_NE(result.header.find("class MyTestViewModel;"), std::string::npos);

    // Inspect source output
    EXPECT_NE(result.source.find("MyTestView::MyTestView(std::shared_ptr<MyTestViewModel> viewModel)"), std::string::npos);
    EXPECT_NE(result.source.find("rootLayout = std::make_shared<gooey::Row>();"), std::string::npos);
    EXPECT_NE(result.source.find(" <<= viewModel->status;"), std::string::npos);
    EXPECT_NE(result.source.find("this->add_child(rootLayout);"), std::string::npos);
}

#include <fstream>
#include <filesystem>
#include "tooey/binding.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/scrollbar.hpp"

TEST(TooeyLayout, CustomComponentChecking) {
    // Create a temporary FileMenu.ooey file in the current directory
    std::string filename = "FileMenu.ooey";
    std::ofstream ofs(filename);
    ofs << "VBox id=menuContainer:\n    Button text=\"Open\"\n";
    ofs.close();

    std::string source = 
        "HBox id=rootLayout:\n"
        "    FileMenu id=leftSidebarMenu\n";

    auto tokens = Lexer::tokenize(source);
    // Parse passing the current directory "."
    auto ast = Parser::parse(tokens, ".");

    // Clean up file first to ensure it's deleted even if assertion fails
    std::filesystem::remove(filename);

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->children.size(), 1);
    auto hbox = ast->children[0];
    ASSERT_EQ(hbox->children.size(), 1);
    
    auto file_menu = hbox->children[0];
    EXPECT_EQ(file_menu->nodeType, "FileMenu");
    EXPECT_TRUE(file_menu->isCustomComponent);
    
    ASSERT_EQ(ast->customIncludes.size(), 1);
    EXPECT_EQ(ast->customIncludes[0], "FileMenu");

    // Let's also check codegen output for this AST
    auto result = CodeGenerator::generate(ast, "MainView", "MainViewModel");
    EXPECT_NE(result.header.find("#include \"FileMenu.hpp\""), std::string::npos);
    EXPECT_NE(result.source.find("leftSidebarMenu = std::make_shared<FileMenu>(viewModel->get_file_menu_view_model());"), std::string::npos);
}

TEST(TooeyLayout, BindingOperatorWithLabel) {
    auto label = std::make_shared<gooey::controls::Label>("Initial", ooey::Font{}, ooey::Point{0,0}, ooey::Color{});
    gooey::mvvmc::Property<std::string> string_prop{"Hello"};
    
    label <<= string_prop;
    EXPECT_EQ(label->get_text(), "Hello");
    
    string_prop.set("World");
    EXPECT_EQ(label->get_text(), "World");
}

TEST(TooeyLayout, BindingOperatorWithScrollBar) {
    auto scrollbar = std::make_shared<gooey::controls::ScrollBar>(ooey::Rect{0, 0, 10, 100}, gooey::controls::ScrollBarOrientation::Vertical);
    gooey::mvvmc::Property<int> value_prop{10};
    
    scrollbar <<= value_prop;
    EXPECT_EQ(scrollbar->get_value(), 10);
    
    value_prop.set(50);
    EXPECT_EQ(scrollbar->get_value(), 50);
}

TEST(TooeyLayout, ListControlTemplateCodegen) {
    std::string source = 
        "VBox id=rootLayout:\n"
        "    ListControl id=myList items=@binding.tasks:\n"
        "        Row id=itemRow height=45:\n"
        "            CheckBox id=taskCheck checked=@binding.tasks.completed text=@binding.tasks.text\n";

    auto tokens = Lexer::tokenize(source);
    auto ast = Parser::parse(tokens);

    auto result = CodeGenerator::generate(ast, "MyListView", "MyListViewModel");

    // Inspect header output
    EXPECT_NE(result.header.find("std::shared_ptr<gooey::controls::ListControl> myList;"), std::string::npos);
    EXPECT_NE(result.header.find("#include \"gooey/controls/checkbox.hpp\""), std::string::npos);

    // Inspect source output
    EXPECT_NE(result.source.find("myList->set_item_height(45);"), std::string::npos);
    EXPECT_NE(result.source.find("using ItemListType = decltype(static_cast<MyListViewModel*>(nullptr)->tasks.get());"), std::string::npos);
    EXPECT_NE(result.source.find("using ItemType = typename std::decay_t<ItemListType>::value_type;"), std::string::npos);
    EXPECT_NE(result.source.find("myList->bind(viewModel->tasks,"), std::string::npos);
    EXPECT_NE(result.source.find("tooey::set_control_value(taskCheck, item.completed);"), std::string::npos);
    EXPECT_NE(result.source.find("tooey::set_control_value(taskCheck, item.text);"), std::string::npos);
    EXPECT_NE(result.source.find("list_val[i].completed = newValue;"), std::string::npos);
}
