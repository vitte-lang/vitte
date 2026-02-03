// ============================================================
// vitte_codegen_llvm::lower::expr
// Lowering des expressions MIR -> LLVM IR
// ============================================================

#include "vitte/codegen/llvm/context.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"

#include <cassert>

namespace vitte::codegen::llvm_backend::lower {

using namespace vitte::mir;


// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

static llvm::Value* lowerExpr(FunctionContext& ctx,
                              const Expr& expr);


// ------------------------------------------------------------
// Constantes
// ------------------------------------------------------------

static llvm::Value* lowerConst(FunctionContext& ctx,
                               const Expr& expr)
{
    const auto& cst = expr.constValue; // hypothèse MIR
    llvm::LLVMContext& llctx = ctx.cg.llvmContext;

    switch (cst.kind) {
        case ConstValue::Int:
            return llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(llctx),
                cst.asInt64(),
                true
            );

        case ConstValue::Bool:
            return llvm::ConstantInt::get(
                llvm::Type::getInt1Ty(llctx),
                cst.asBool()
            );

        case ConstValue::Float:
            return llvm::ConstantFP::get(
                llvm::Type::getDoubleTy(llctx),
                cst.asDouble()
            );

        case ConstValue::Null:
            return llvm::ConstantPointerNull::get(
                llvm::Type::getInt8PtrTy(llctx)
            );

        default:
            return nullptr;
    }
}


// ------------------------------------------------------------
// Variables locales
// ------------------------------------------------------------

static llvm::Value* lowerLocal(FunctionContext& ctx,
                               const Expr& expr)
{
    auto* allocaInst = ctx.getLocal(expr.localId);
    assert(allocaInst && "use of undeclared local");

    return ctx.cg.builder.CreateLoad(
        allocaInst->getType()->getPointerElementType(),
        allocaInst,
        "load_local"
    );
}


// ------------------------------------------------------------
// Opérations binaires
// ------------------------------------------------------------

static llvm::Value* lowerBinary(FunctionContext& ctx,
                                const Expr& expr)
{
    llvm::Value* lhs = lowerExpr(ctx, *expr.binary.lhs);
    llvm::Value* rhs = lowerExpr(ctx, *expr.binary.rhs);

    switch (expr.binary.op) {
        case BinOp::Add:
            return ctx.cg.builder.CreateAdd(lhs, rhs, "add");

        case BinOp::Sub:
            return ctx.cg.builder.CreateSub(lhs, rhs, "sub");

        case BinOp::Mul:
            return ctx.cg.builder.CreateMul(lhs, rhs, "mul");

        case BinOp::Div:
            return ctx.cg.builder.CreateSDiv(lhs, rhs, "div");

        case BinOp::Eq:
            return ctx.cg.builder.CreateICmpEQ(lhs, rhs, "eq");

        case BinOp::Ne:
            return ctx.cg.builder.CreateICmpNE(lhs, rhs, "ne");

        case BinOp::Lt:
            return ctx.cg.builder.CreateICmpSLT(lhs, rhs, "lt");

        case BinOp::Le:
            return ctx.cg.builder.CreateICmpSLE(lhs, rhs, "le");

        case BinOp::Gt:
            return ctx.cg.builder.CreateICmpSGT(lhs, rhs, "gt");

        case BinOp::Ge:
            return ctx.cg.builder.CreateICmpSGE(lhs, rhs, "ge");

        case BinOp::And:
            return ctx.cg.builder.CreateAnd(lhs, rhs, "and");

        case BinOp::Or:
            return ctx.cg.builder.CreateOr(lhs, rhs, "or");

        default:
            return nullptr;
    }
}


// ------------------------------------------------------------
// Opérations unaires
// ------------------------------------------------------------

static llvm::Value* lowerUnary(FunctionContext& ctx,
                               const Expr& expr)
{
    llvm::Value* val = lowerExpr(ctx, *expr.unary.expr);

    switch (expr.unary.op) {
        case UnOp::Neg:
            return ctx.cg.builder.CreateNeg(val, "neg");

        case UnOp::Not:
            return ctx.cg.builder.CreateNot(val, "not");

        default:
            return nullptr;
    }
}


// ------------------------------------------------------------
// Appels de fonction
// ------------------------------------------------------------

static llvm::Value* lowerCall(FunctionContext& ctx,
                              const Expr& expr)
{
    auto it = ctx.cg.functions.find(expr.call.fnId);
    assert(it != ctx.cg.functions.end() &&
           "call to undeclared function");

    llvm::Function* callee = it->second;

    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.call.args)
        args.push_back(lowerExpr(ctx, *arg));

    return ctx.cg.builder.CreateCall(
        callee,
        args,
        "call"
    );
}


// ------------------------------------------------------------
// Casts
// ------------------------------------------------------------

static llvm::Value* lowerCast(FunctionContext& ctx,
                              const Expr& expr)
{
    llvm::Value* val = lowerExpr(ctx, *expr.cast.expr);
    llvm::Type* dstTy = val->getType(); // placeholder

    // Simplifié : cast bitwise
    return ctx.cg.builder.CreateBitCast(
        val,
        dstTy,
        "cast"
    );
}


// ------------------------------------------------------------
// If-expression
// ------------------------------------------------------------

static llvm::Value* lowerIf(FunctionContext& ctx,
                            const Expr& expr)
{
    llvm::Value* cond = lowerExpr(ctx, *expr.ifExpr.cond);

    llvm::Function* fn = ctx.function;
    llvm::LLVMContext& llctx = ctx.cg.llvmContext;

    auto* thenBB =
        llvm::BasicBlock::Create(llctx, "then", fn);
    auto* elseBB =
        llvm::BasicBlock::Create(llctx, "else");
    auto* mergeBB =
        llvm::BasicBlock::Create(llctx, "ifcont");

    ctx.cg.builder.CreateCondBr(cond, thenBB, elseBB);

    // then
    ctx.cg.builder.SetInsertPoint(thenBB);
    llvm::Value* thenVal =
        lowerExpr(ctx, *expr.ifExpr.thenExpr);
    ctx.cg.builder.CreateBr(mergeBB);
    thenBB = ctx.cg.builder.GetInsertBlock();

    // else
    fn->getBasicBlockList().push_back(elseBB);
    ctx.cg.builder.SetInsertPoint(elseBB);
    llvm::Value* elseVal =
        lowerExpr(ctx, *expr.ifExpr.elseExpr);
    ctx.cg.builder.CreateBr(mergeBB);
    elseBB = ctx.cg.builder.GetInsertBlock();

    // merge
    fn->getBasicBlockList().push_back(mergeBB);
    ctx.cg.builder.SetInsertPoint(mergeBB);

    auto* phi = ctx.cg.builder.CreatePHI(
        thenVal->getType(),
        2,
        "iftmp"
    );
    phi->addIncoming(thenVal, thenBB);
    phi->addIncoming(elseVal, elseBB);

    return phi;
}


// ------------------------------------------------------------
// Block-expression
// ------------------------------------------------------------

static llvm::Value* lowerBlockExpr(FunctionContext& ctx,
                                   const Expr& expr)
{
    llvm::Value* last = nullptr;

    for (const auto& stmt : expr.block.stmts) {
        // hypothèse : lowerStmt existe
        last = lowerExpr(ctx, *stmt);
    }

    return last;
}


// ------------------------------------------------------------
// Entrée principale
// ------------------------------------------------------------

static llvm::Value* lowerExpr(FunctionContext& ctx,
                              const Expr& expr)
{
    switch (expr.kind) {
        case ExprKind::Const:
            return lowerConst(ctx, expr);

        case ExprKind::Local:
            return lowerLocal(ctx, expr);

        case ExprKind::Binary:
            return lowerBinary(ctx, expr);

        case ExprKind::Unary:
            return lowerUnary(ctx, expr);

        case ExprKind::Call:
            return lowerCall(ctx, expr);

        case ExprKind::Cast:
            return lowerCast(ctx, expr);

        case ExprKind::If:
            return lowerIf(ctx, expr);

        case ExprKind::Block:
            return lowerBlockExpr(ctx, expr);

        default:
            return nullptr;
    }
}


// ------------------------------------------------------------
// API publique
// ------------------------------------------------------------

llvm::Value* lowerExpression(FunctionContext& ctx,
                             const Expr& expr)
{
    llvm::Value* v = lowerExpr(ctx, expr);
    assert(v && "lowerExpression produced null");
    return v;
}

} // namespace vitte::codegen::llvm_backend::lower
