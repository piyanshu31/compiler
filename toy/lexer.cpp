#include <iostream>
#include <string>
#include <vector>
#include <cctype>

enum class TokenType {
    Identifier,
    Number,
    Plus, Minus, Star, Slash,
    Equals,
    LParen, RParen,
    LBrace, RBrace,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
    std::string text;
    size_t pos;
public:
    Lexer(const std::string& input) : text(input), pos(0) {}

    char currentChar() {
        if (pos < text.size()) return text[pos];
        return '\0';
    }

    void advance() { pos++; }

    void skipWhitespace() {
        while (isspace(currentChar())) advance();
    }

    Token number() {
        std::string result;
        while (isdigit(currentChar())) {
            result += currentChar();
            advance();
        }
        return {TokenType::Number, result};
    }

    Token identifier() {
        std::string result;
        while (isalnum(currentChar())) {
            result += currentChar();
            advance();
        }
        return {TokenType::Identifier, result};
    }

    Token getNextToken() {
        skipWhitespace();

        char ch = currentChar();
        if (ch == '\0') return {TokenType::EndOfFile, ""};
        if (isdigit(ch)) return number();
        if (isalpha(ch)) return identifier();

        switch (ch) {
            case '+': advance(); return {TokenType::Plus, "+"};
            case '-': advance(); return {TokenType::Minus, "-"};
            case '*': advance(); return {TokenType::Star, "*"};
            case '/': advance(); return {TokenType::Slash, "/"};
            case '=': advance(); return {TokenType::Equals, "="};
            case '(': advance(); return {TokenType::LParen, "("};
            case ')': advance(); return {TokenType::RParen, ")"};
            case '{': advance(); return {TokenType::LBrace, "{"};
            case '}': advance(); return {TokenType::RBrace, "}"};
        }

        std::cerr << "Unknown character: " << ch << "\n";
        advance();
        return getNextToken();
    }
};

int main() {
    std::string input = "x = 3 + 4";
    Lexer lexer(input);

    Token token;
    do {
        token = lexer.getNextToken();
        std::cout << "Token: " << token.value << "\n";
    } while (token.type != TokenType::EndOfFile);

    return 0;
}
