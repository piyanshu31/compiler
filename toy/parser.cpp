#include <iostream>
#include <vector>
#include <string>
#include <cctype>

enum class TokenType { Number, Plus, Minus, Mul, Div, LParen, RParen, End };

struct Token {
    TokenType type;
    std::string value;
};

// ---------- Lexer (same as Day 1, but included here for completeness) ----------
std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    for (size_t i = 0; i < input.size();) {
        if (std::isspace(input[i])) { i++; continue; }
        if (std::isdigit(input[i])) {
            std::string num;
            while (i < input.size() && std::isdigit(input[i])) {
                num.push_back(input[i++]);
            }
            tokens.push_back({TokenType::Number, num});
        } else if (input[i] == '+') { tokens.push_back({TokenType::Plus, "+"}); i++; }
        else if (input[i] == '-') { tokens.push_back({TokenType::Minus, "-"}); i++; }
        else if (input[i] == '*') { tokens.push_back({TokenType::Mul, "*"}); i++; }
        else if (input[i] == '/') { tokens.push_back({TokenType::Div, "/"}); i++; }
        else if (input[i] == '(') { tokens.push_back({TokenType::LParen, "("}); i++; }
        else if (input[i] == ')') { tokens.push_back({TokenType::RParen, ")"}); i++; }
        else { std::cerr << "Unexpected character: " << input[i] << "\n"; i++; }
    }
    tokens.push_back({TokenType::End, ""});
    return tokens;
}

// ---------- Parser ----------
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token peek() { return tokens[pos]; }
    Token get() { return tokens[pos++]; }

public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

    // Expr → Term (('+' | '-') Term)*
    int parseExpr() {
        int value = parseTerm();
        while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
            Token op = get();
            int rhs = parseTerm();
            if (op.type == TokenType::Plus) value += rhs;
            else value -= rhs;
        }
        return value;
    }

    // Term → Factor (('*' | '/') Factor)*
    int parseTerm() {
        int value = parseFactor();
        while (peek().type == TokenType::Mul || peek().type == TokenType::Div) {
            Token op = get();
            int rhs = parseFactor();
            if (op.type == TokenType::Mul) value *= rhs;
            else value /= rhs;
        }
        return value;
    }

    // Factor → NUMBER | '(' Expr ')'
    int parseFactor() {
        Token t = get();
        if (t.type == TokenType::Number) {
            return std::stoi(t.value);
        } else if (t.type == TokenType::LParen) {
            int value = parseExpr();
            if (peek().type != TokenType::RParen) {
                throw std::runtime_error("Expected ')'");
            }
            get(); // consume ')'
            return value;
        }
        throw std::runtime_error("Unexpected token in factor");
    }
};

// ---------- Main ----------
int main() {
    std::string input;
    std::cout << "Enter expression: ";
    std::getline(std::cin, input);

    auto tokens = tokenize(input);
    Parser parser(tokens);

    try {
        int result = parser.parseExpr();
        std::cout << "Result = " << result << "\n";
    } catch (std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
    }
}
