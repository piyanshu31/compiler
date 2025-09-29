#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <unordered_map>

// =====================
//  TOKENS
// =====================
enum class TokenType {
    Number,
    Identifier,
    Plus, Minus, Star, Slash,
    LParen, RParen,
    Assign,     // '='
    End
};

struct Token {
    TokenType type;
    std::string text;
};

// =====================
//  LEXER
// =====================
class Lexer {
    std::string input;
    size_t pos = 0;

public:
    Lexer(const std::string& s) : input(s) {}

    Token next() {
        while (pos < input.size() && isspace(input[pos])) pos++;

        if (pos >= input.size()) return {TokenType::End, ""};

        char c = input[pos];

        if (isdigit(c)) {
            std::string num;
            while (pos < input.size() && isdigit(input[pos])) {
                num.push_back(input[pos++]);
            }
            return {TokenType::Number, num};
        }

        if (isalpha(c)) {
            std::string id;
            while (pos < input.size() && isalnum(input[pos])) {
                id.push_back(input[pos++]);
            }
            return {TokenType::Identifier, id};
        }

        pos++;
        switch (c) {
            case '+': return {TokenType::Plus, "+"};
            case '-': return {TokenType::Minus, "-"};
            case '*': return {TokenType::Star, "*"};
            case '/': return {TokenType::Slash, "/"};
            case '(': return {TokenType::LParen, "("};
            case ')': return {TokenType::RParen, ")"};
            case '=': return {TokenType::Assign, "="};
        }

        throw std::runtime_error(std::string("Unexpected char: ") + c);
    }
};

// =====================
//  AST NODES
// =====================
struct Expr {
    virtual ~Expr() = default;
};

struct NumberExpr : Expr {
    int value;
    NumberExpr(int v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(const std::string& n) : name(n) {}
};

struct AssignExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(const std::string& n, std::unique_ptr<Expr> v)
    : name(n), value(std::move(v)) {}
};

struct BinaryExpr : Expr {
    char op;
    std::unique_ptr<Expr> left, right;
    BinaryExpr(char o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
    : op(o), left(std::move(l)), right(std::move(r)) {}
};

// =====================
//  PARSER
// =====================
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token& peek() {
        if (pos >= tokens.size()) throw std::runtime_error("Unexpected end");
        return tokens[pos];
    }

    Token get() { return tokens[pos++]; }

    bool match(TokenType t) {
        if (pos < tokens.size() && tokens[pos].type == t) {
            pos++;
            return true;
        }
        return false;
    }

public:
    Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

    std::unique_ptr<Expr> parse() {
        return parseStatement();
    }

private:
    std::unique_ptr<Expr> parseStatement() {
        if (peek().type == TokenType::Identifier) {
            Token id = get();
            if (match(TokenType::Assign)) {
                auto val = parseExpression();
                return std::make_unique<AssignExpr>(id.text, std::move(val));
            } else {
                pos--; // rollback if not assignment
            }
        }
        return parseExpression();
    }

    std::unique_ptr<Expr> parseExpression() {
        auto node = parseTerm();
        while (pos < tokens.size() &&
            (peek().type == TokenType::Plus || peek().type == TokenType::Minus)) {
            char op = get().text[0];
        auto rhs = parseTerm();
        node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(rhs));
            }
            return node;
    }

    std::unique_ptr<Expr> parseTerm() {
        auto node = parseFactor();
        while (pos < tokens.size() &&
            (peek().type == TokenType::Star || peek().type == TokenType::Slash)) {
            char op = get().text[0];
        auto rhs = parseFactor();
        node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(rhs));
            }
            return node;
    }

    std::unique_ptr<Expr> parseFactor() {
        Token tok = get();
        if (tok.type == TokenType::Number) {
            return std::make_unique<NumberExpr>(std::stoi(tok.text));
        } else if (tok.type == TokenType::Identifier) {
            return std::make_unique<VarExpr>(tok.text);
        } else if (tok.type == TokenType::LParen) {
            auto expr = parseExpression();
            if (!match(TokenType::RParen))
                throw std::runtime_error("Missing ')'");
            return expr;
        }
        throw std::runtime_error("Unexpected token: " + tok.text);
    }
};

// =====================
//  EVALUATOR
// =====================
std::unordered_map<std::string, int> variables;

int eval(Expr* expr) {
    if (auto num = dynamic_cast<NumberExpr*>(expr)) {
        return num->value;
    } else if (auto var = dynamic_cast<VarExpr*>(expr)) {
        if (variables.find(var->name) == variables.end())
            throw std::runtime_error("Undefined variable: " + var->name);
        return variables[var->name];
    } else if (auto assign = dynamic_cast<AssignExpr*>(expr)) {
        int val = eval(assign->value.get());
        variables[assign->name] = val;
        return val;
    } else if (auto bin = dynamic_cast<BinaryExpr*>(expr)) {
        int left = eval(bin->left.get());
        int right = eval(bin->right.get());
        switch (bin->op) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return right != 0 ? left / right : throw std::runtime_error("Divide by zero");
        }
    }
    throw std::runtime_error("Unknown expression type");
}

// =====================
//  MAIN
// =====================
int main() {
    std::cout << "Mini Compiler with Variables. Type 'exit' to quit.\n";

    while (true) {
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line) || line == "exit") break;

        try {
            Lexer lex(line);
            std::vector<Token> tokens;
            while (true) {
                Token t = lex.next();
                if (t.type == TokenType::End) break;
                tokens.push_back(t);
            }

            Parser parser(tokens);
            auto tree = parser.parse();

            int result = eval(tree.get());
            std::cout << result << "\n";
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}
