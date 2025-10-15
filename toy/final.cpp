#include <bits/stdc++.h>
using namespace std;

/* ------------------- Tokens / Lexer ------------------- */
enum class TokenKind {
    End,
    Number,
    Identifier,
    Plus, Minus, Star, Slash,
    LParen, RParen,
    Assign,     // '='
    Semicolon,  // ';'
    KeywordPrint,
    Invalid
};

struct Token {
    TokenKind kind;
    string text;
    double number = 0.0;
};

static bool isKeywordPrint(const string &s) {
    return s == "print";
}

class Lexer {
    string in;
    size_t pos = 0;
public:
    Lexer(string s): in(move(s)), pos(0) {}
    char peek() const { return pos < in.size() ? in[pos] : '\0'; }
    char get() { return pos < in.size() ? in[pos++] : '\0'; }
    void skipWS() { while (isspace((unsigned char)peek())) get(); }

    Token next() {
        skipWS();
        char c = peek();
        if (c == '\0') return {TokenKind::End, ""};

        // Number (allow decimal)
        if (isdigit((unsigned char)c) || (c == '.' && pos + 1 < in.size() && isdigit((unsigned char)in[pos+1]))) {
            string s;
            bool seen_dot = false;
            while (isdigit((unsigned char)peek()) || (!seen_dot && peek() == '.')) {
                if (peek() == '.') seen_dot = true;
                s.push_back(get());
            }
            Token t; t.kind = TokenKind::Number; t.text = s; t.number = stod(s);
            return t;
        }

        // Identifier / keyword (letters, digits, underscore)
        if (isalpha((unsigned char)c) || c == '_') {
            string id;
            while (isalnum((unsigned char)peek()) || peek() == '_') id.push_back(get());
            if (isKeywordPrint(id)) return {TokenKind::KeywordPrint, id};
            return {TokenKind::Identifier, id};
        }

        // single-char tokens
        char ch = get();
        switch (ch) {
            case '+': return {TokenKind::Plus, "+"};
            case '-': return {TokenKind::Minus, "-"};
            case '*': return {TokenKind::Star, "*"};
            case '/': return {TokenKind::Slash, "/"};
            case '(': return {TokenKind::LParen, "("};
            case ')': return {TokenKind::RParen, ")"};
            case '=': return {TokenKind::Assign, "="};
            case ';': return {TokenKind::Semicolon, ";"};
            default: return {TokenKind::Invalid, string(1,ch)};
        }
    }
};

/* ------------------- AST Nodes ------------------- */
struct Expr { virtual ~Expr() = default; };

struct NumberExpr : Expr {
    double value;
    NumberExpr(double v): value(v) {}
};

struct VariableExpr : Expr {
    string name;
    VariableExpr(string n): name(move(n)) {}
};

struct BinaryExpr : Expr {
    char op;
    unique_ptr<Expr> lhs, rhs;
    BinaryExpr(char o, unique_ptr<Expr> l, unique_ptr<Expr> r): op(o), lhs(move(l)), rhs(move(r)) {}
};

struct AssignExpr : Expr {
    string name;
    unique_ptr<Expr> value;
    AssignExpr(string n, unique_ptr<Expr> v): name(move(n)), value(move(v)) {}
};

struct PrintExpr : Expr {
    unique_ptr<Expr> value;
    PrintExpr(unique_ptr<Expr> v): value(move(v)) {}
};

/* ------------------- Parser (recursive descent) ------------------- */
class Parser {
    vector<Token> toks;
    size_t pos = 0;
public:
    Parser(vector<Token> t): toks(move(t)), pos(0) {}
    Token& peek() {
        if (pos >= toks.size()) throw runtime_error("Parser: unexpected end");
        return toks[pos];
    }
    Token get() {
        if (pos >= toks.size()) throw runtime_error("Parser: unexpected end");
        return toks[pos++];
    }
    bool accept(TokenKind k) {
        if (pos < toks.size() && toks[pos].kind == k) { pos++; return true; }
        return false;
    }
    vector<unique_ptr<Expr>> parseProgram() {
        vector<unique_ptr<Expr>> out;
        while (pos < toks.size() && peek().kind != TokenKind::End) {
            auto s = parseStatement();
            out.push_back(move(s));

            // Require semicolon after each statement
            if (!accept(TokenKind::Semicolon) && peek().kind != TokenKind::End) {
                throw runtime_error("Parser: expected ';' after statement");
            }

        }
        return out;
    }




private:
    unique_ptr<Expr> parseStatement() {
        if (pos < toks.size()) {
            if (peek().kind == TokenKind::KeywordPrint) {
                get(); // consume 'print'
                // allow: print ( <expr> )  or print <expr>
                if (accept(TokenKind::LParen)) {
                    auto e = parseExpression();
                    if (!accept(TokenKind::RParen)) throw runtime_error("Parser: expected ')'");
                    return make_unique<PrintExpr>(move(e));
                } else {
                    auto e = parseExpression();
                    return make_unique<PrintExpr>(move(e));
                }
            }
            // assignment: Identifier '=' expression
            if (peek().kind == TokenKind::Identifier) {
                // lookahead
                Token id = toks[pos];
                if (pos + 1 < toks.size() && toks[pos+1].kind == TokenKind::Assign) {
                    pos++; // consume id
                    pos++; // consume =
                    auto rhs = parseExpression();
                    return make_unique<AssignExpr>(id.text, move(rhs));
                }
            }
        }
        return parseExpression();
    }

    unique_ptr<Expr> parseExpression() {
        auto left = parseTerm();
        while (pos < toks.size() && (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus)) {
            char op = get().text[0];
            auto right = parseTerm();
            left = make_unique<BinaryExpr>(op, move(left), move(right));
        }
        return left;
    }

    unique_ptr<Expr> parseTerm() {
        auto left = parseFactor();
        while (pos < toks.size() && (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash)) {
            char op = get().text[0];
            auto right = parseFactor();
            left = make_unique<BinaryExpr>(op, move(left), move(right));
        }
        return left;
    }

    unique_ptr<Expr> parseFactor() {
        // Support unary minus here
        if (peek().kind == TokenKind::Minus) {
            get(); // consume '-'
            auto operand = parseFactor();
            return make_unique<BinaryExpr>('-', make_unique<NumberExpr>(0.0), move(operand));
        }

        Token t = get();
        if (t.kind == TokenKind::Number) {
            return make_unique<NumberExpr>(t.number);
        } else if (t.kind == TokenKind::Identifier) {
            return make_unique<VariableExpr>(t.text);
        } else if (t.kind == TokenKind::LParen) {
            auto e = parseExpression();
            if (!accept(TokenKind::RParen)) throw runtime_error("Parser: expected ')'");
            return e;
        }
        throw runtime_error(string("Parser: unexpected token '") + t.text + "'");
    }
};

/* ------------------- Semantic Analyzer ------------------- */
/* Very small pass: track which variables were assigned (declared) so far.
 *   Warnings are printed for use-before-assignment. Assignments "declare" variable. */
class SemanticAnalyzer {
    unordered_set<string> assigned;
    vector<string> warnings;
    vector<string> errors;
public:
    void analyzeProgram(const vector<unique_ptr<Expr>>& prog) {
        assigned.clear(); warnings.clear(); errors.clear();
        for (const auto &s : prog) analyzeStmt(s.get());
    }
    const vector<string>& getWarnings() const { return warnings; }
    const vector<string>& getErrors() const { return errors; }

private:
    void analyzeExpr(const Expr* e) {
        if (!e) return;
        if (auto n = dynamic_cast<const NumberExpr*>(e)) return;
        if (auto v = dynamic_cast<const VariableExpr*>(e)) {
            if (assigned.find(v->name) == assigned.end())
                warnings.push_back("use of variable '" + v->name + "' before assignment");
            return;
        }
        if (auto b = dynamic_cast<const BinaryExpr*>(e)) {
            analyzeExpr(b->lhs.get());
            analyzeExpr(b->rhs.get());
            return;
        }
        if (auto as = dynamic_cast<const AssignExpr*>(e)) {
            analyzeExpr(as->value.get());
            // assignment declares variable afterwards
            assigned.insert(as->name);
            return;
        }
        if (auto p = dynamic_cast<const PrintExpr*>(e)) {
            analyzeExpr(p->value.get());
            return;
        }
        errors.push_back("semantic: unknown node");
    }

    void analyzeStmt(const Expr* s) { analyzeExpr(s); }
};

/* ------------------- CodeGen: tiny stack instructions ------------------- */
enum class OpCode { PUSH_CONST, LOAD_VAR, STORE_VAR, ADD, SUB, MUL, DIV, PRINT };

struct Instr {
    OpCode op;
    double val;     // for PUSH_CONST
    string name;    // for LOAD_VAR / STORE_VAR
    Instr(OpCode o=OpCode::PUSH_CONST, double v=0.0, string n=""): op(o), val(v), name(move(n)) {}
};

class CodeGen {
    vector<Instr> code;
public:
    vector<Instr> generateProgram(const vector<unique_ptr<Expr>>& prog) {
        code.clear();
        for (const auto &s : prog) {
            generateExpr(s.get());
            // After a statement, we may leave value on stack; PRINT handles printing for print statements.
            // For expressions/assignments we leave result on stack as well (REPL may print it).
        }
        return code;
    }

private:
    void generateExpr(const Expr* e) {
        if (!e) return;
        if (auto n = dynamic_cast<const NumberExpr*>(e)) {
            code.emplace_back(OpCode::PUSH_CONST, n->value);
            return;
        }
        if (auto v = dynamic_cast<const VariableExpr*>(e)) {
            code.emplace_back(OpCode::LOAD_VAR, 0.0, v->name);
            return;
        }
        if (auto b = dynamic_cast<const BinaryExpr*>(e)) {
            generateExpr(b->lhs.get());
            generateExpr(b->rhs.get());
            switch (b->op) {
                case '+': code.emplace_back(OpCode::ADD); break;
                case '-': code.emplace_back(OpCode::SUB); break;
                case '*': code.emplace_back(OpCode::MUL); break;
                case '/': code.emplace_back(OpCode::DIV); break;
                default: throw runtime_error("CodeGen: unknown op");
            }
            return;
        }
        if (auto a = dynamic_cast<const AssignExpr*>(e)) {
            generateExpr(a->value.get());
            code.emplace_back(OpCode::STORE_VAR, 0.0, a->name);
            return;
        }
        if (auto p = dynamic_cast<const PrintExpr*>(e)) {
            generateExpr(p->value.get());
            code.emplace_back(OpCode::PRINT);
            return;
        }
        throw runtime_error("CodeGen: unknown node");
    }
};

/* ------------------- VM: executes instruction vector ------------------- */
class VM {
    vector<double> stack;
    unordered_map<string,double> vars;
public:
    VM() = default;

    double execSingle(const vector<Instr>& code) {
        stack.clear();
        for (size_t ip=0; ip<code.size(); ++ip) {
            const Instr &ins = code[ip];
            switch (ins.op) {
                case OpCode::PUSH_CONST:
                    stack.push_back(ins.val);
                    break;
                case OpCode::LOAD_VAR: {
                    auto it = vars.find(ins.name);
                    if (it == vars.end()) throw runtime_error("VM: undefined variable '" + ins.name + "'");
                    stack.push_back(it->second);
                } break;
                case OpCode::STORE_VAR: {
                    if (stack.empty()) throw runtime_error("VM: store with empty stack");
                    double v = stack.back(); stack.pop_back();
                    vars[ins.name] = v;
                    // push value back so caller can view result if needed
                    stack.push_back(v);
                } break;
                case OpCode::ADD: {
                    if (stack.size() < 2) throw runtime_error("VM: stack underflow ADD");
                    double r = stack.back(); stack.pop_back();
                    double l = stack.back(); stack.pop_back();
                    stack.push_back(l + r);
                } break;
                case OpCode::SUB: {
                    if (stack.size() < 2) throw runtime_error("VM: stack underflow SUB");
                    double r = stack.back(); stack.pop_back();
                    double l = stack.back(); stack.pop_back();
                    stack.push_back(l - r);
                } break;
                case OpCode::MUL: {
                    if (stack.size() < 2) throw runtime_error("VM: stack underflow MUL");
                    double r = stack.back(); stack.pop_back();
                    double l = stack.back(); stack.pop_back();
                    stack.push_back(l * r);
                } break;
                case OpCode::DIV: {
                    if (stack.size() < 2) throw runtime_error("VM: stack underflow DIV");
                    double r = stack.back(); stack.pop_back();
                    double l = stack.back(); stack.pop_back();
                    if (r == 0.0) throw runtime_error("VM: division by zero");
                    stack.push_back(l / r);
                } break;
                case OpCode::PRINT: {
                    if (stack.empty()) throw runtime_error("VM: stack underflow PRINT");
                    double v = stack.back(); stack.pop_back();
                    // print nicely
                    cout.setf(std::ios::fmtflags(0), ios::floatfield);
                    cout << v << "\n";
                } break;
                default:
                    throw runtime_error("VM: unknown opcode");
            }
        }
        if (stack.empty()) return 0.0;
        return stack.back();
    }

    optional<double> getVar(const string &name) const {
        auto it = vars.find(name);
        if (it == vars.end()) return nullopt;
        return it->second;
    }

    void setVar(const string &name, double v) { vars[name] = v; }
};

/* ------------------- Small helper to generate per-statement code ------------------- */
vector<Instr> genForStmt(const unique_ptr<Expr>& stmt) {
    vector<Instr> out;
    function<void(const Expr*)> gen = [&](const Expr* e) {
        if (!e) return;
        if (auto n = dynamic_cast<const NumberExpr*>(e)) {
            out.emplace_back(OpCode::PUSH_CONST, n->value);
            return;
        }
        if (auto v = dynamic_cast<const VariableExpr*>(e)) {
            out.emplace_back(OpCode::LOAD_VAR, 0.0, v->name);
            return;
        }
        if (auto b = dynamic_cast<const BinaryExpr*>(e)) {
            gen(b->lhs.get());
            gen(b->rhs.get());
            switch (b->op) {
                case '+': out.emplace_back(OpCode::ADD); break;
                case '-': out.emplace_back(OpCode::SUB); break;
                case '*': out.emplace_back(OpCode::MUL); break;
                case '/': out.emplace_back(OpCode::DIV); break;
            }
            return;
        }
        if (auto a = dynamic_cast<const AssignExpr*>(e)) {
            gen(a->value.get());
            out.emplace_back(OpCode::STORE_VAR, 0.0, a->name);
            return;
        }
        if (auto p = dynamic_cast<const PrintExpr*>(e)) {
            gen(p->value.get());
            out.emplace_back(OpCode::PRINT);
            return;
        }
    };
    gen(stmt.get());
    return out;
}



/* ------------------- Main: REPL glue ------------------- */
int main() {
    cout << "Supports print. Enter statements; use ';' to separate. Empty line quits.\n";
    SemanticAnalyzer sem;
    VM vm;

    while (true) {
        cout << "> ";
        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) break;
        if (line == "exit" || line == "quit") break;

        try {
            // 1) tokenize
            Lexer lx(line);
            vector<Token> toks;
            while (true) {
                Token t = lx.next();
                if (t.kind == TokenKind::Invalid) throw runtime_error("Lexer: invalid char '" + t.text + "'");
                if (t.kind == TokenKind::End) break;
                toks.push_back(move(t));
            }
            toks.push_back({TokenKind::End, ""});

            // 2) parse program
            Parser parser(toks);
            auto prog = parser.parseProgram();

            // 3) semantic analyze
            sem.analyzeProgram(prog);
            for (auto &w : sem.getWarnings()) cerr << "Warning: " << w << "\n";
            if (!sem.getErrors().empty()) {
                for (auto &e : sem.getErrors()) cerr << "Error: " << e << "\n";
                continue;
            }

            // 4) for each statement: codegen & run
            for (auto &stmt : prog) {
                auto code = genForStmt(stmt);
                double res = vm.execSingle(code);
                // If statement not a print, print REPL result
                if (!dynamic_cast<PrintExpr*>(stmt.get())) {
                    // print numeric result for expressions/assignments
                    cout << res << "\n";
                }
            }
        } catch (const exception &ex) {
            cerr << "Error: " << ex.what() << "\n";
        }
    }

    cout << "Goodbye.\n";
    return 0;
}
