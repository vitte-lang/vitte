// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool Parser::is_expr_start(TokenKind kind) const {
    return is_unary_start(kind);
}

bool Parser::is_unary_start(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Bang:
        case TokenKind::Minus:
        case TokenKind::Tilde:
        case TokenKind::KwNot:
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
        case TokenKind::IntLit:
        case TokenKind::FloatLit:
        case TokenKind::CharLit:
        case TokenKind::StringLit:
        case TokenKind::BytesLit:
        case TokenKind::Ident:
        case TokenKind::LParen:
        case TokenKind::LBracket:
            return true;
        default:
            return false;
    }
}

bool Parser::is_binary_op(TokenKind kind) const {
    return tokens::is_binary_operator(kind);
}

BinaryOp Parser::to_binary_op(TokenKind kind) const {
    return tokens::to_binary_op(kind);
}
