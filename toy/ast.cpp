#include <iostream>
#include <string>
#include <cctype>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <vector>

enum class TokenKind {
    End,
    Number,
    Identifier,
    Plus, Minus, Star, Slash,
    LParen, RParen,
    Assign,
    Semicolon,
    Invalid
};

struct Token {
    TokenKind kind;
    std::string text;
    double number;
};

class Lexer {
    std::string input;
    size_t pos = 0;

    char peekChar() const { return pos < input.size() ? input[pos] : '\0'; }
    char getChar() { return pos < input.size() ? input[pos++] : '\0'; }

public:
    Lexer(std::string s) : input(std::move(s)) {}

    void skipWS() {
        while (std::isspace((unsigned char)peekChar())) getChar();
    }

    Token next() {
        skipWS();
        char c = peekChar();
        if (c == '\0') return {TokenKind::End, "", 0.0};

        if (std::isdigit((unsigned char)c)
            || (c == '.' && pos + 1 < input.size() && std::isdigit((unsigned char)input[pos+1]))) {
            std::string t;
        bool seen_dot = false;
        while (std::isdigit((unsigned char)peekChar()) || (!seen_dot && peekChar() == '.')) {
            if (peekChar() == '.') seen_dot = true;
            t.push_back(getChar());
        }
        double val = std::stod(t);
        return {TokenKind::Number, t, val};
            }

            if (std::isalpha((unsigned char)c) || c == '_') {
                std::string id;
                while (std::isalnum((unsigned char)peekChar()) || peekChar() == '_') id.push_back(getChar());
                return {TokenKind::Identifier, id, 0.0};
            }

            char ch = getChar();
            switch (ch) {
                case '+': return {TokenKind::Plus, "+", 0};
                case '-': return {TokenKind::Minus, "-", 0};
                case '*': return {TokenKind::Star, "*", 0};
                case '/': return {TokenKind::Slash, "/", 0};
                case '(': return {TokenKind::LParen, "(", 0};
                case ')': return {TokenKind::RParen, ")", 0};
                case '=': return {TokenKind::Assign, "=", 0};
                case ';': return {TokenKind::Semicolon, ";", 0};
                default:
                    return {TokenKind::Invalid, std::string(1, ch), 0};
            }
    }
};

struct Expr { virtual ~Expr() = default; };

struct NumberExpr : Expr {
    double value;
    NumberExpr(double v) : value(v) {}
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : Expr {
    char op;
    std::unique_ptr<Expr> lhs, rhs;
    BinaryExpr(char o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
    : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct AssignExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(std::string n, std::unique_ptr<Expr> v) : name(std::move(n)), value(std::move(v)) {}
};

class Parser {
    Lexer lex;
    Token cur;

    void consume() { cur = lex.next(); }

public:
    Parser(const std::string &s) : lex(s) { consume(); }

    std::unique_ptr<Expr> parsePrimary() {
        if (cur.kind == TokenKind::Number) {
            double v = cur.number;
            consume();
            return std::make_unique<NumberExpr>(v);
        }
        if (cur.kind == TokenKind::Identifier) {
            std::string id = cur.text;
            consume();
            if (cur.kind == TokenKind::Assign) {
                consume();
                auto rhs = parseExpression();
                return std::make_unique<AssignExpr>(id, std::move(rhs));
            }
            return std::make_unique<VariableExpr>(id);
        }
        if (cur.kind == TokenKind::LParen) {
            consume();
            auto e = parseExpression();
            if (cur.kind != TokenKind::RParen) throw std::runtime_error("Expected ')'");
            consume();
            return e;
        }
        throw std::runtime_error("Unexpected token in primary");
    }

    std::unique_ptr<Expr> parseTerm() {
        auto left = parsePrimary();
        while (cur.kind == TokenKind::Star || cur.kind == TokenKind::Slash) {
            char op = (cur.kind == TokenKind::Star) ? '*' : '/';
            consume();
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<Expr> parseExpression() {
        auto left = parseTerm();
        while (cur.kind == TokenKind::Plus || cur.kind == TokenKind::Minus) {
            char op = (cur.kind == TokenKind::Plus) ? '+' : '-';
            consume();
            auto right = parseTerm();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }
};

double evalAST(const Expr* node, std::unordered_map<std::string,double> &vars) {
    if (const NumberExpr* n = dynamic_cast<const NumberExpr*>(node)) {
        return n->value;
    }
    if (const VariableExpr* v = dynamic_cast<const VariableExpr*>(node)) {
        auto it = vars.find(v->name);
        if (it == vars.end()) throw std::runtime_error("Undefined variable: " + v->name);
        return it->second;
    }
    if (const BinaryExpr* b = dynamic_cast<const BinaryExpr*>(node)) {
        double a = evalAST(b->lhs.get(), vars);
        double c = evalAST(b->rhs.get(), vars);
        switch (b->op) {
            case '+': return a + c;
            case '-': return a - c;
            case '*': return a * c;
            case '/': if (c == 0) throw std::runtime_error("Division by zero"); return a / c;
            default: throw std::runtime_error("Unknown binary op");
        }
    }
    if (const AssignExpr* asg = dynamic_cast<const AssignExpr*>(node)) {
        double val = evalAST(asg->value.get(), vars);
        vars[asg->name] = val;
        return val;
    }
    throw std::runtime_error("Unknown AST node in eval");
}

void printAST(const Expr* node, int indent=0) {
    auto pad = [&](int n){ for(int i=0;i<n;i++) std::cout << ' '; };
    if (const NumberExpr* n = dynamic_cast<const NumberExpr*>(node)) {
        pad(indent); std::cout << "Number(" << n->value << ")\n"; return;
    }
    if (const VariableExpr* v = dynamic_cast<const VariableExpr*>(node)) {
        pad(indent); std::cout << "Var(" << v->name << ")\n"; return;
    }
    if (const BinaryExpr* b = dynamic_cast<const BinaryExpr*>(node)) {
        pad(indent); std::cout << "BinaryOp(" << b->op << ")\n";
        printAST(b->lhs.get(), indent + 2);
        printAST(b->rhs.get(), indent + 2);
        return;
    }
    if (const AssignExpr* a = dynamic_cast<const AssignExpr*>(node)) {
        pad(indent); std::cout << "Assign(" << a->name << ")\n";
        printAST(a->value.get(), indent + 2);
        return;
    }
    pad(indent); std::cout << "UnknownNode\n";
}

int main() {
    std::unordered_map<std::string,double> vars;
    std::string line;

    std::cout << "AST Compiler. Type expressions or assignments. Empty line to quit.\n";
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) break;

        try {
            Parser p(line);
            auto tree = p.parseExpression();
            std::cout << "[AST]\n";
            printAST(tree.get(), 2);

            double result = evalAST(tree.get(), vars);
            std::cout << "=> " << result << "\n";
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    return 0;
}
