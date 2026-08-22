#ifndef LSM_AST_HPP
#define LSM_AST_HPP

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include "../../middleend/ssa/SSAInstruction.hpp"

struct SourceSpan {
    size_t startByte = 0, endByte = 0, line = 1, col = 1;
    SourceSpan() = default;
    SourceSpan(size_t l) : line(l) {}
    SourceSpan(size_t sb, size_t eb, size_t l, size_t c) : startByte(sb), endByte(eb), line(l), col(c) {}
};

enum class ASTNodeType {
    IntLit, FloatLit, StringLit, Var, NilLit, Binary, Unary, Ternary, ArrayLit,
    Block, ExprStmt, VarAssign, Print, Input, Return, Break, Continue, Defer, ErrorNode,
    If, While, For, Switch, Match, FuncDecl, ExternFuncDecl, FuncCall, Lambda,
    ClassDecl, MethodCall, NewExpr, MemberAccess, MemberAssign, InterfaceDecl, EnumDecl,
    TypeAlias, RecDecl, ArrDecl, ArrAccess, ArrAssign, ArrayAlloc, TryCatch, Throw,
    ResultOk, ResultErr, OptionSome, OptionNone, GoStmt, ChanDecl, ChanOp,
    NativeSetReg, NativeGetReg, NativeHalt, NativeCli, NativeSti,
    NativePortOut, NativePortIn, NativeMmioWrite, NativeMmioRead,
    IOPortOp, AtomicOp, InterruptOp, Attribute, CompileTimeIf, RegisterVar,
    Import, CImport, UnsafeBlock, InlineAsm, VolatileStore, VolatileLoad,
    PtrCast, PtrAddr, PtrDeref, Slice, Range, MultiReturn, SafeAccess, 
    CastNode,
    OffsetOfNode
};

struct ASTNode {
    ASTNodeType type;
    SourceSpan span;
    size_t line = 1;

    explicit ASTNode(ASTNodeType t, SourceSpan s = {}) : type(t), span(s), line(s.line) {}
    virtual ~ASTNode() = default;
};

struct ErrorASTNode : ASTNode {
    std::string errorMessage;
    explicit ErrorASTNode(std::string msg, SourceSpan s = {}) : ASTNode(ASTNodeType::ErrorNode, s), errorMessage(std::move(msg)) {}
};

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> stmts;
    explicit BlockNode(SourceSpan s = {}) : ASTNode(ASTNodeType::Block, s) {}
};

struct ProgramNode : BlockNode {
    bool fullDev = false;
    explicit ProgramNode(SourceSpan s = {}) : BlockNode(s) {}
};

struct IntLitNode : ASTNode {
    int64_t val;
    IntLitNode(int64_t v, SourceSpan s = {}) : ASTNode(ASTNodeType::IntLit, s), val(v) {}
};

struct FloatLitNode : ASTNode {
    double val;
    FloatLitNode(double v, SourceSpan s = {}) : ASTNode(ASTNodeType::FloatLit, s), val(v) {}
};

struct StringLitNode : ASTNode {
    std::string val;
    StringLitNode(std::string v, SourceSpan s = {}) : ASTNode(ASTNodeType::StringLit, s), val(std::move(v)) {}
};

struct VarNode : ASTNode {
    std::string name;
    VarNode(std::string n, SourceSpan s = {}) : ASTNode(ASTNodeType::Var, s), name(std::move(n)) {}
};

struct NilLitNode : ASTNode {
    explicit NilLitNode(SourceSpan s = {}) : ASTNode(ASTNodeType::NilLit, s) {}
};

struct UnaryNode : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> operand;
    UnaryNode(std::string o, std::unique_ptr<ASTNode> opnd, SourceSpan s = {}) : ASTNode(ASTNodeType::Unary, s), op(std::move(o)), operand(std::move(opnd)) {}
};

struct BinaryNode : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left, right;
    BinaryNode(std::string o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, SourceSpan s = {}) : ASTNode(ASTNodeType::Binary, s), op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

struct TernaryNode : ASTNode {
    std::unique_ptr<ASTNode> cond, thenExpr, elseExpr;
    TernaryNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::Ternary, s), cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
};

struct ArrayLitNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elems;
    explicit ArrayLitNode(std::vector<std::unique_ptr<ASTNode>> e, SourceSpan s = {}) : ASTNode(ASTNodeType::ArrayLit, s), elems(std::move(e)) {}
};

struct ArrayAllocNode : ASTNode {
    std::string elemType;
    int64_t size;
    ArrayAllocNode(std::string t, int64_t s, SourceSpan sp = {}) : ASTNode(ASTNodeType::ArrayAlloc, sp), elemType(std::move(t)), size(s) {}
};

struct ExprStmtNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ExprStmtNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::ExprStmt, s), expr(std::move(e)) {}
};

struct VarAssignNode : ASTNode {
    std::string name, explicitType, op;
    std::unique_ptr<ASTNode> value;
    VarAssignNode(std::string n, std::string t, std::unique_ptr<ASTNode> v, std::string o = "=", SourceSpan s = {}) : ASTNode(ASTNodeType::VarAssign, s), name(std::move(n)), explicitType(std::move(t)), value(std::move(v)), op(std::move(o)) {}
};

struct PrintNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    PrintNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::Print, s), expr(std::move(e)) {}
};

struct InputNode : ASTNode { explicit InputNode(SourceSpan s = {}) : ASTNode(ASTNodeType::Input, s) {} };

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    explicit ReturnNode(std::unique_ptr<ASTNode> e = nullptr, SourceSpan s = {}) : ASTNode(ASTNodeType::Return, s), expr(std::move(e)) {}
};

struct BreakNode : ASTNode { explicit BreakNode(SourceSpan s = {}) : ASTNode(ASTNodeType::Break, s) {} };
struct ContinueNode : ASTNode { explicit ContinueNode(SourceSpan s = {}) : ASTNode(ASTNodeType::Continue, s) {} };

struct DeferNode : ASTNode {
    std::unique_ptr<ASTNode> call;
    DeferNode(std::unique_ptr<ASTNode> c, SourceSpan s = {}) : ASTNode(ASTNodeType::Defer, s), call(std::move(c)) {}
};

struct MemberAssignNode : ASTNode {
    std::unique_ptr<ASTNode> target, value;
    std::string op;
    MemberAssignNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> v, std::string o = "=", SourceSpan s = {}) : ASTNode(ASTNodeType::MemberAssign, s), target(std::move(t)), value(std::move(v)), op(std::move(o)) {}
};

struct ArrAssignNode : ASTNode {
    std::unique_ptr<ASTNode> arr, index, value;
    std::string op;
    ArrAssignNode(std::unique_ptr<ASTNode> a, std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> v, std::string o = "=", SourceSpan s = {}) : ASTNode(ASTNodeType::ArrAssign, s), arr(std::move(a)), index(std::move(i)), value(std::move(v)), op(std::move(o)) {}
};

struct FuncParam { std::string name, type; };

struct FuncDeclNode : ASTNode {
    std::string name, returnType, section;
    std::vector<std::string> typeParams;
    std::vector<FuncParam> params;
    std::unique_ptr<ASTNode> body;
    bool isProc = false, isInline = false, isNaked = false, isInterrupt = false, isNoMangle = false;

    FuncDeclNode(std::string n, std::vector<std::string> tp, std::vector<FuncParam> p, std::string retT, std::unique_ptr<ASTNode> b, bool proc, SourceSpan s = {}) : ASTNode(ASTNodeType::FuncDecl, s), name(std::move(n)), typeParams(std::move(tp)), params(std::move(p)), returnType(std::move(retT)), body(std::move(b)), isProc(proc) {}
};

struct ExternFuncDeclNode : ASTNode {
    std::string library, name, returnType;
    std::vector<FuncParam> params;
    ExternFuncDeclNode(std::string lib, std::string n, std::vector<FuncParam> p, std::string retT, SourceSpan s = {}) : ASTNode(ASTNodeType::ExternFuncDecl, s), library(std::move(lib)), name(std::move(n)), params(std::move(p)), returnType(std::move(retT)) {}
};

struct FuncCallNode : ASTNode {
    std::unique_ptr<ASTNode> callee;
    std::vector<std::string> typeArgs;
    std::vector<std::unique_ptr<ASTNode>> args;
    FuncCallNode(std::unique_ptr<ASTNode> c, std::vector<std::string> tArgs, std::vector<std::unique_ptr<ASTNode>> a, SourceSpan s = {}) : ASTNode(ASTNodeType::FuncCall, s), callee(std::move(c)), typeArgs(std::move(tArgs)), args(std::move(a)) {}
};

struct LambdaNode : ASTNode {
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
    bool isProc;
    LambdaNode(std::vector<std::string> p, std::unique_ptr<ASTNode> b, bool proc, SourceSpan s = {}) : ASTNode(ASTNodeType::Lambda, s), params(std::move(p)), body(std::move(b)), isProc(proc) {}
};

struct ClassDeclNode : ASTNode {
    std::string name, parent;
    std::vector<std::string> typeParams, fields;
    std::vector<std::unique_ptr<FuncDeclNode>> methods;
    ClassDeclNode(std::string n, std::vector<std::string> tp, std::string p, std::vector<std::string> f, std::vector<std::unique_ptr<FuncDeclNode>> m, SourceSpan s = {}) : ASTNode(ASTNodeType::ClassDecl, s), name(std::move(n)), typeParams(std::move(tp)), parent(std::move(p)), fields(std::move(f)), methods(std::move(m)) {}
};

struct MethodCallNode : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string method;
    std::vector<std::string> typeArgs;
    std::vector<std::unique_ptr<ASTNode>> args;
    MethodCallNode(std::unique_ptr<ASTNode> o, std::string m, std::vector<std::string> tArgs, std::vector<std::unique_ptr<ASTNode>> a, SourceSpan s = {}) : ASTNode(ASTNodeType::MethodCall, s), object(std::move(o)), method(std::move(m)), typeArgs(std::move(tArgs)), args(std::move(a)) {}
};

struct NewExprNode : ASTNode {
    std::string className;
    std::vector<std::string> typeArgs;
    std::vector<std::unique_ptr<ASTNode>> args;
    NewExprNode(std::string n, std::vector<std::string> tArgs, std::vector<std::unique_ptr<ASTNode>> a, SourceSpan s = {}) : ASTNode(ASTNodeType::NewExpr, s), className(std::move(n)), typeArgs(std::move(tArgs)), args(std::move(a)) {}
};

struct MemberAccessNode : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string member;
    MemberAccessNode(std::unique_ptr<ASTNode> o, std::string m, SourceSpan s = {}) : ASTNode(ASTNodeType::MemberAccess, s), object(std::move(o)), member(std::move(m)) {}
};

struct InterfaceDeclNode : ASTNode {
    std::string name;
    std::vector<std::string> typeParams, methods;
    InterfaceDeclNode(std::string n, std::vector<std::string> tp, std::vector<std::string> m, SourceSpan s = {}) : ASTNode(ASTNodeType::InterfaceDecl, s), name(std::move(n)), typeParams(std::move(tp)), methods(std::move(m)) {}
};

struct EnumDeclNode : ASTNode {
    std::string name;
    std::vector<std::string> variants;
    EnumDeclNode(std::string n, std::vector<std::string> v, SourceSpan s = {}) : ASTNode(ASTNodeType::EnumDecl, s), name(std::move(n)), variants(std::move(v)) {}
};

struct TypeAliasNode : ASTNode {
    std::string name, aliasedType;
    TypeAliasNode(std::string n, std::string a, SourceSpan s = {}) : ASTNode(ASTNodeType::TypeAlias, s), name(std::move(n)), aliasedType(std::move(a)) {}
};

struct RecDeclNode : ASTNode {
    std::string name;
    std::vector<std::string> typeParams;
    std::vector<std::string> fields;
    int alignment = 0;

    size_t totalSizeBytes = 0;
    std::unordered_map<std::string, size_t> fieldOffsets;

    RecDeclNode(std::string n, std::vector<std::string> tp, std::vector<std::string> f, SourceSpan s = {}) 
        : ASTNode(ASTNodeType::RecDecl, s), name(std::move(n)), typeParams(std::move(tp)), fields(std::move(f)) {}
};

struct OffsetOfNode : ASTNode {
    std::string recordName;
    std::string fieldName;
    size_t computedOffset = 0;

    OffsetOfNode(std::string rec, std::string field, SourceSpan s = {}) 
        : ASTNode(ASTNodeType::OffsetOfNode, s), recordName(std::move(rec)), fieldName(std::move(field)) {}
};

struct ArrDeclNode : ASTNode {
    std::string name, elemType;
    int size;
    bool isSlice;
    ArrDeclNode(std::string n, int s, std::string e, bool slice, SourceSpan sp = {}) : ASTNode(ASTNodeType::ArrDecl, sp), name(std::move(n)), size(s), elemType(std::move(e)), isSlice(slice) {}
};

struct ArrAccessNode : ASTNode {
    std::unique_ptr<ASTNode> arr, index;
    ArrAccessNode(std::unique_ptr<ASTNode> a, std::unique_ptr<ASTNode> i, SourceSpan s = {}) : ASTNode(ASTNodeType::ArrAccess, s), arr(std::move(a)), index(std::move(i)) {}
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> cond, thenBranch, elseBranch;
    IfNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> e = nullptr, SourceSpan s = {}) : ASTNode(ASTNodeType::If, s), cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> cond, body;
    WhileNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> b, SourceSpan s = {}) : ASTNode(ASTNodeType::While, s), cond(std::move(c)), body(std::move(b)) {}
};

struct ForNode : ASTNode {
    std::string var;
    std::unique_ptr<ASTNode> iterable, body;
    ForNode(std::string v, std::unique_ptr<ASTNode> iter, std::unique_ptr<ASTNode> b, SourceSpan s = {}) : ASTNode(ASTNodeType::For, s), var(std::move(v)), iterable(std::move(iter)), body(std::move(b)) {}
};

struct SwitchCase { std::unique_ptr<ASTNode> value, body; };
struct SwitchNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    std::vector<SwitchCase> cases;
    SwitchNode(std::unique_ptr<ASTNode> e, std::vector<SwitchCase> c, SourceSpan s = {}) : ASTNode(ASTNodeType::Switch, s), expr(std::move(e)), cases(std::move(c)) {}
};

struct MatchCase {
    std::string patternType, bindVariable;
    std::unique_ptr<ASTNode> literalPattern, body;
};
struct MatchNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    std::vector<MatchCase> cases;
    MatchNode(std::unique_ptr<ASTNode> e, std::vector<MatchCase> c, SourceSpan s = {}) : ASTNode(ASTNodeType::Match, s), expr(std::move(e)), cases(std::move(c)) {}
};

struct TryCatchNode : ASTNode {
    std::unique_ptr<ASTNode> tryBlock, catchBlock;
    std::string errVar;
    TryCatchNode(std::unique_ptr<ASTNode> t, std::string e, std::unique_ptr<ASTNode> c, SourceSpan s = {}) : ASTNode(ASTNodeType::TryCatch, s), tryBlock(std::move(t)), errVar(std::move(e)), catchBlock(std::move(c)) {}
};

struct ThrowNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ThrowNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::Throw, s), expr(std::move(e)) {}
};

struct ResultOkNode : ASTNode { std::unique_ptr<ASTNode> value; ResultOkNode(std::unique_ptr<ASTNode> v, SourceSpan s = {}) : ASTNode(ASTNodeType::ResultOk, s), value(std::move(v)) {} };
struct ResultErrNode : ASTNode { std::unique_ptr<ASTNode> error; ResultErrNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::ResultErr, s), error(std::move(e)) {} };
struct OptionSomeNode : ASTNode { std::unique_ptr<ASTNode> value; OptionSomeNode(std::unique_ptr<ASTNode> v, SourceSpan s = {}) : ASTNode(ASTNodeType::OptionSome, s), value(std::move(v)) {} };
struct OptionNoneNode : ASTNode { explicit OptionNoneNode(SourceSpan s = {}) : ASTNode(ASTNodeType::OptionNone, s) {} };

struct GoStmtNode : ASTNode { std::unique_ptr<ASTNode> call; GoStmtNode(std::unique_ptr<ASTNode> c, SourceSpan s = {}) : ASTNode(ASTNodeType::GoStmt, s), call(std::move(c)) {} };

struct ChanDeclNode : ASTNode {
    std::string name, elemType;
    bool isBuffered; int capacity;
    ChanDeclNode(std::string n, std::string e, bool buf, int cap, SourceSpan s = {}) : ASTNode(ASTNodeType::ChanDecl, s), name(std::move(n)), elemType(std::move(e)), isBuffered(buf), capacity(cap) {}
};

struct ChanOpNode : ASTNode {
    std::unique_ptr<ASTNode> channel, value;
    bool isSend;
    ChanOpNode(std::unique_ptr<ASTNode> ch, std::unique_ptr<ASTNode> val, bool send, SourceSpan s = {}) : ASTNode(ASTNodeType::ChanOp, s), channel(std::move(ch)), value(std::move(val)), isSend(send) {}
};

struct NativeSetRegNode : ASTNode { std::string regName; std::unique_ptr<ASTNode> value; NativeSetRegNode(std::string r, std::unique_ptr<ASTNode> v, SourceSpan s = {}) : ASTNode(ASTNodeType::NativeSetReg, s), regName(std::move(r)), value(std::move(v)) {} };
struct NativeGetRegNode : ASTNode { std::string regName; NativeGetRegNode(std::string r, SourceSpan s = {}) : ASTNode(ASTNodeType::NativeGetReg, s), regName(std::move(r)) {} };
struct NativeHaltNode : ASTNode { explicit NativeHaltNode(SourceSpan s = {}) : ASTNode(ASTNodeType::NativeHalt, s) {} };
struct NativeCliNode : ASTNode { explicit NativeCliNode(SourceSpan s = {}) : ASTNode(ASTNodeType::NativeCli, s) {} };
struct NativeStiNode : ASTNode { explicit NativeStiNode(SourceSpan s = {}) : ASTNode(ASTNodeType::NativeSti, s) {} };
struct NativePortOutNode : ASTNode { std::unique_ptr<ASTNode> port, value; NativePortOutNode(std::unique_ptr<ASTNode> p, std::unique_ptr<ASTNode> v, SourceSpan s = {}) : ASTNode(ASTNodeType::NativePortOut, s), port(std::move(p)), value(std::move(v)) {} };
struct NativePortInNode : ASTNode { std::unique_ptr<ASTNode> port; NativePortInNode(std::unique_ptr<ASTNode> p, SourceSpan s = {}) : ASTNode(ASTNodeType::NativePortIn, s), port(std::move(p)) {} };
struct NativeMmioWriteNode : ASTNode { std::unique_ptr<ASTNode> address, value; NativeMmioWriteNode(std::unique_ptr<ASTNode> addr, std::unique_ptr<ASTNode> val, SourceSpan s = {}) : ASTNode(ASTNodeType::NativeMmioWrite, s), address(std::move(addr)), value(std::move(val)) {} };
struct NativeMmioReadNode : ASTNode { std::unique_ptr<ASTNode> address; NativeMmioReadNode(std::unique_ptr<ASTNode> addr, SourceSpan s = {}) : ASTNode(ASTNodeType::NativeMmioRead, s), address(std::move(addr)) {} };

struct IOPortOpNode : ASTNode { std::string op; std::unique_ptr<ASTNode> port, value; IOPortOpNode(std::string o, std::unique_ptr<ASTNode> p, std::unique_ptr<ASTNode> v = nullptr, SourceSpan s = {}) : ASTNode(ASTNodeType::IOPortOp, s), op(std::move(o)), port(std::move(p)), value(std::move(v)) {} };
struct AtomicOpNode : ASTNode { std::string op; std::unique_ptr<ASTNode> address, value, expected; AtomicOpNode(std::string o, std::unique_ptr<ASTNode> addr, std::unique_ptr<ASTNode> val = nullptr, std::unique_ptr<ASTNode> exp = nullptr, SourceSpan s = {}) : ASTNode(ASTNodeType::AtomicOp, s), op(std::move(o)), address(std::move(addr)), value(std::move(val)), expected(std::move(exp)) {} };
struct InterruptOpNode : ASTNode { std::string op; explicit InterruptOpNode(std::string o, SourceSpan s = {}) : ASTNode(ASTNodeType::InterruptOp, s), op(std::move(o)) {} };
struct RegisterVarNode : ASTNode { std::string regName, varName, type; RegisterVarNode(std::string reg, std::string name, std::string t, SourceSpan s = {}) : ASTNode(ASTNodeType::RegisterVar, s), regName(std::move(reg)), varName(std::move(name)), type(std::move(t)) {} };
struct AttributeNode : ASTNode { std::string name; std::vector<std::string> args; AttributeNode(std::string n, std::vector<std::string> a = {}, SourceSpan s = {}) : ASTNode(ASTNodeType::Attribute, s), name(std::move(n)), args(std::move(a)) {} };
struct CompileTimeIfNode : ASTNode { std::unique_ptr<ASTNode> condition, thenBranch, elseBranch; CompileTimeIfNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> thenBr, std::unique_ptr<ASTNode> elseBr = nullptr, SourceSpan s = {}) : ASTNode(ASTNodeType::CompileTimeIf, s), condition(std::move(cond)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {} };
struct SliceNode : ASTNode { std::unique_ptr<ASTNode> array, start, end; SliceNode(std::unique_ptr<ASTNode> a, std::unique_ptr<ASTNode> s, std::unique_ptr<ASTNode> e, SourceSpan sp = {}) : ASTNode(ASTNodeType::Slice, sp), array(std::move(a)), start(std::move(s)), end(std::move(e)) {} };
struct RangeNode : ASTNode { std::unique_ptr<ASTNode> start, end; bool inclusive; RangeNode(std::unique_ptr<ASTNode> s, std::unique_ptr<ASTNode> e, bool inc, SourceSpan sp = {}) : ASTNode(ASTNodeType::Range, sp), start(std::move(s)), end(std::move(e)), inclusive(inc) {} };
struct MultiReturnNode : ASTNode { std::vector<std::unique_ptr<ASTNode>> values; MultiReturnNode(std::vector<std::unique_ptr<ASTNode>> v, SourceSpan s = {}) : ASTNode(ASTNodeType::MultiReturn, s), values(std::move(v)) {} };
struct SafeAccessNode : ASTNode { std::unique_ptr<ASTNode> expr; SafeAccessNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::SafeAccess, s), expr(std::move(e)) {} };
struct ImportNode : ASTNode { std::string path; bool isCHeader; ImportNode(std::string p, bool cHead, SourceSpan s = {}) : ASTNode(cHead ? ASTNodeType::CImport : ASTNodeType::Import, s), path(std::move(p)), isCHeader(cHead) {} };
struct UnsafeBlockNode : ASTNode { std::unique_ptr<ASTNode> body; UnsafeBlockNode(std::unique_ptr<ASTNode> b, SourceSpan s = {}) : ASTNode(ASTNodeType::UnsafeBlock, s), body(std::move(b)) {} };
struct InlineAsmNode : ASTNode { std::string code; InlineAsmNode(std::string c, SourceSpan s = {}) : ASTNode(ASTNodeType::InlineAsm, s), code(std::move(c)) {} };


struct VolatileStoreNode : ASTNode { 
    std::unique_ptr<ASTNode> address, value; 
    int bitSize = 8;
    VolatileStoreNode(std::unique_ptr<ASTNode> a, std::unique_ptr<ASTNode> v, int sz = 8, SourceSpan s = {}) : ASTNode(ASTNodeType::VolatileStore, s), address(std::move(a)), value(std::move(v)), bitSize(sz) {} 
};

struct VolatileLoadNode : ASTNode { 
    std::unique_ptr<ASTNode> address; 
    int bitSize = 8;
    VolatileLoadNode(std::unique_ptr<ASTNode> a, int sz = 8, SourceSpan s = {}) : ASTNode(ASTNodeType::VolatileLoad, s), address(std::move(a)), bitSize(sz) {} 
};

struct PtrCastNode : ASTNode { std::unique_ptr<ASTNode> expr; std::string targetType; PtrCastNode(std::unique_ptr<ASTNode> e, std::string t, SourceSpan s = {}) : ASTNode(ASTNodeType::PtrCast, s), expr(std::move(e)), targetType(std::move(t)) {} };
struct PtrAddrNode : ASTNode { std::unique_ptr<ASTNode> expr; PtrAddrNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::PtrAddr, s), expr(std::move(e)) {} };
struct PtrDerefNode : ASTNode { std::unique_ptr<ASTNode> expr; PtrDerefNode(std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::PtrDeref, s), expr(std::move(e)) {} };

struct CastNode : ASTNode {
    LsmStaticType targetType;
    std::unique_ptr<ASTNode> expr;
    CastNode(LsmStaticType t, std::unique_ptr<ASTNode> e, SourceSpan s = {}) : ASTNode(ASTNodeType::CastNode, s), targetType(t), expr(std::move(e)) {}
};

#endif 