// ============================================================
// token_tables.cpp — Vitte Compiler
// Centralized keyword/operator tables for lexer+parser+driver
// ============================================================

#include "token_tables.hpp"

#include <algorithm>
#include <unordered_map>

namespace vitte::frontend::tokens {

static constexpr KeywordSpec kKeywords[] = {
    {"space", TokenKind::KwSpace, true},
    {"pull", TokenKind::KwPull, true},
    {"use", TokenKind::KwUse, true},
    {"share", TokenKind::KwShare, true},
    {"form", TokenKind::KwForm, true},
    {"field", TokenKind::KwField, false},
    {"pick", TokenKind::KwPick, true},
    {"case", TokenKind::KwCase, true},
    {"trait", TokenKind::KwTrait, false},
    {"type", TokenKind::KwType, true},
    {"const", TokenKind::KwConst, true},
    {"macro", TokenKind::KwMacro, false},
    {"proc", TokenKind::KwProc, true},
    {"entry", TokenKind::KwEntry, true},
    {"at", TokenKind::KwAt, true},
    {"asm", TokenKind::KwAsm, false},
    {"unsafe", TokenKind::KwUnsafe, false},
    {"match", TokenKind::KwMatch, true},
    {"let", TokenKind::KwLet, true},
    {"make", TokenKind::KwMake, true},
    {"set", TokenKind::KwSet, true},
    {"give", TokenKind::KwGive, true},
    {"emit", TokenKind::KwEmit, true},
    {"if", TokenKind::KwIf, true},
    {"else", TokenKind::KwElse, true},
    {"otherwise", TokenKind::KwOtherwise, true},
    {"select", TokenKind::KwSelect, false},
    {"when", TokenKind::KwWhen, true},
    {"is", TokenKind::KwIs, true},
    {"loop", TokenKind::KwLoop, true},
    {"for", TokenKind::KwFor, true},
    {"in", TokenKind::KwIn, true},
    {"break", TokenKind::KwBreak, true},
    {"continue", TokenKind::KwContinue, true},
    {"return", TokenKind::KwReturn, true},
    {"not", TokenKind::KwNot, true},
    {"and", TokenKind::KwAnd, true},
    {"or", TokenKind::KwOr, true},
    {"as", TokenKind::KwAs, true},
    {"all", TokenKind::KwAll, true},
    {"true", TokenKind::KwTrue, true},
    {"false", TokenKind::KwFalse, true},
    {"bool", TokenKind::KwBool, true},
    {"string", TokenKind::KwString, true},
    {"int", TokenKind::KwInt, true},
};

static constexpr BinaryOpSpec kBinaryOps[] = {
#define VITTE_PRECEDENCE_OP(token, lexeme, op, prec, core) {token, lexeme, op, prec, core},
#include "precedence_table.def"
#undef VITTE_PRECEDENCE_OP
};

TokenKind keyword_kind(std::string_view ident) {
    static const std::unordered_map<std::string, TokenKind> table = []() {
        std::unordered_map<std::string, TokenKind> out;
        for (const auto& kw : kKeywords) {
            out.emplace(kw.text, kw.kind);
        }
        return out;
    }();
    const auto it = table.find(std::string(ident));
    if (it == table.end()) {
        return TokenKind::Ident;
    }
    return it->second;
}

const char* keyword_text(TokenKind kind) {
    for (const auto& kw : kKeywords) {
        if (kw.kind == kind) {
            return kw.text;
        }
    }
    return nullptr;
}

bool is_core_keyword(TokenKind kind) {
    for (const auto& kw : kKeywords) {
        if (kw.kind == kind) {
            return kw.core;
        }
    }
    return false;
}

bool is_forbidden_in_core(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwMacro:
        case TokenKind::KwTrait:
        case TokenKind::KwAsm:
        case TokenKind::KwUnsafe:
        case TokenKind::KwSelect:
        case TokenKind::KwField:
            return true;
        default:
            return false;
    }
}

std::vector<std::string> core_keywords() {
    std::vector<std::string> out;
    out.reserve(std::size(kKeywords));
    for (const auto& kw : kKeywords) {
        if (kw.core) {
            out.emplace_back(kw.text);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

int binary_precedence(TokenKind kind) {
    for (const auto& op : kBinaryOps) {
        if (op.token == kind) {
            return op.precedence;
        }
    }
    return 0;
}

bool is_binary_operator(TokenKind kind) {
    return binary_precedence(kind) > 0;
}

ast::BinaryOp to_binary_op(TokenKind kind) {
    for (const auto& op : kBinaryOps) {
        if (op.token == kind) {
            return op.op;
        }
    }
    return ast::BinaryOp::Add;
}

std::vector<std::string> core_binary_operators() {
    std::vector<std::string> out;
    out.reserve(std::size(kBinaryOps));
    for (const auto& op : kBinaryOps) {
        if (op.core) {
            out.emplace_back(op.lexeme);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

} // namespace vitte::frontend::tokens
