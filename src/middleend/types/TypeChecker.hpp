#ifndef LSM_TYPE_CHECKER_HPP
#define LSM_TYPE_CHECKER_HPP

#include "../../frontend/ast/AST.hpp"
#include "../../core/Result.hpp"
#include "../../backend/regalloc/PhysicalRegister.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class TypeKind {
    Void, Int32, Int64, Float64, String, Bool,
    Ptr, Ptr32, Array, Slice, Record, Interface, Class,
    Channel, GenericParam, Dynamic, Error
};

struct TypeInfo {
    TypeKind kind = TypeKind::Void;
    std::string name;
    std::string innerType;
    std::vector<std::pair<std::string, std::string>> fields;
    std::vector<std::string> methods;

    size_t totalSizeBytes = 0;
    std::unordered_map<std::string, size_t> fieldOffsets;

    bool operator==(const TypeInfo& other) const {
        if (kind == TypeKind::Dynamic || other.kind == TypeKind::Dynamic) return true;
        if (kind != other.kind) return false;
        if (kind == TypeKind::Record || kind == TypeKind::Class || kind == TypeKind::Interface) {
            return name == other.name;
        }
        return true;
    }
    bool operator!=(const TypeInfo& other) const { return !(*this == other); }
};

class TypeChecker {
private:
    std::unordered_map<std::string, TypeInfo> typeTable;
    std::unordered_map<std::string, TypeInfo> functionReturnTypes;
    std::vector<std::unordered_map<std::string, TypeInfo>> scopeStack;
    std::vector<std::string> errors;
    ArchType targetArch = ArchType::X86_64;

    void enterScope() { scopeStack.emplace_back(); }
    void exitScope() { if (!scopeStack.empty()) scopeStack.pop_back(); }

    bool declareVar(const std::string& name, const TypeInfo& type) {
        if (scopeStack.empty()) return false;
        scopeStack.back()[name] = type;
        return true;
    }

    TypeInfo lookupVar(const std::string& name) {
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return {TypeKind::Error, "Unknown variable: " + name};
    }

    TypeInfo resolveTypeName(const std::string& name) {
        if (name == "Int32" || name == "i32") return {TypeKind::Int32, "Int32"};
        if (name == "Int" || name == "int" || name == "i64") return {TypeKind::Int64, "Int"};
        if (name == "Float" || name == "float" || name == "f64") return {TypeKind::Float64, "Float"};
        if (name == "String" || name == "str") return {TypeKind::String, "String"};
        if (name == "Bool" || name == "bool") return {TypeKind::Bool, "Bool"};
        if (name == "void" || name.empty()) return {TypeKind::Void, "void"};
        auto it = typeTable.find(name);
        if (it != typeTable.end()) return it->second;
        return {TypeKind::Dynamic, name};
    }

    size_t getTypeSizeBytes(TypeKind kind) {
        switch (kind) {
            case TypeKind::Int32: return 4;
            case TypeKind::Int64:
            case TypeKind::Float64:
            case TypeKind::String: return 8;
            case TypeKind::Bool: return 1;
            case TypeKind::Ptr: return (targetArch == ArchType::X86_32) ? 4 : 8;
            case TypeKind::Ptr32: return 4;
            default: return 8;
        }
    }

public:
    TypeChecker(ArchType arch = ArchType::X86_64) : targetArch(arch) { enterScope(); }

    void setTargetArch(ArchType arch) { targetArch = arch; }

    void computeRecordLayout(RecDeclNode* rec) {
        if (!rec) return;

        size_t currentOffset = 0;
        size_t maxAlign = (rec->alignment > 0) ? rec->alignment : 8;

        for (const auto& fieldName : rec->fields) {
            size_t fieldSize = 8;
            size_t align = (maxAlign < fieldSize) ? maxAlign : fieldSize;

            currentOffset = (currentOffset + (align - 1)) & ~(align - 1);
            rec->fieldOffsets[fieldName] = currentOffset;
            currentOffset += fieldSize;
        }

        rec->totalSizeBytes = (currentOffset + (maxAlign - 1)) & ~(maxAlign - 1);

        if (typeTable.count(rec->name)) {
            typeTable[rec->name].totalSizeBytes = rec->totalSizeBytes;
            typeTable[rec->name].fieldOffsets = rec->fieldOffsets;
        }
    }

    void registerFunction(const std::string& name, const std::string& retT) {
        functionReturnTypes[name] = resolveTypeName(retT);
    }

    void registerAllFunctions(ProgramNode* program) {
        if (!program) return;
        for (const auto& stmt : program->stmts) {
            if (stmt->type == ASTNodeType::FuncDecl) {
                auto fn = static_cast<FuncDeclNode*>(stmt.get());
                registerFunction(fn->name, fn->returnType);
            } else if (stmt->type == ASTNodeType::ExternFuncDecl) {
                auto fn = static_cast<ExternFuncDeclNode*>(stmt.get());
                registerFunction(fn->name, fn->returnType);
            }
        }
    }

    Result<void, std::vector<std::string>> checkProgram(ProgramNode* program) {
        errors.clear();
        if (!program) return Result<void, std::vector<std::string>>::Ok();

        registerAllFunctions(program);

        for (const auto& stmt : program->stmts) {
            if (!stmt) continue;
            if (stmt->type == ASTNodeType::InterfaceDecl) {
                auto iface = static_cast<InterfaceDeclNode*>(stmt.get());
                typeTable[iface->name] = {TypeKind::Interface, iface->name, "", {}, iface->methods};
            } else if (stmt->type == ASTNodeType::RecDecl) {
                auto rec = static_cast<RecDeclNode*>(stmt.get());
                typeTable[rec->name] = {TypeKind::Record, rec->name, "", {}, {}};
                computeRecordLayout(rec);
            }
        }

        for (const auto& stmt : program->stmts) {
            checkNode(stmt.get());
        }

        if (!errors.empty()) {
            return Result<void, std::vector<std::string>>::Err(errors);
        }
        return Result<void, std::vector<std::string>>::Ok();
    }

    TypeInfo checkNode(ASTNode* node) {
        if (!node) return {TypeKind::Void, "void"};

        switch (node->type) {
            case ASTNodeType::IntLit:   return {TypeKind::Int64, "Int"};
            case ASTNodeType::FloatLit: return {TypeKind::Float64, "Float"};
            case ASTNodeType::StringLit:return {TypeKind::String, "String"};
            case ASTNodeType::NilLit:   return {TypeKind::Ptr, "nil"};

            case ASTNodeType::OffsetOfNode: {
                auto offNode = static_cast<OffsetOfNode*>(node);
                if (typeTable.count(offNode->recordName)) {
                    const auto& info = typeTable[offNode->recordName];
                    if (info.fieldOffsets.count(offNode->fieldName)) {
                        offNode->computedOffset = info.fieldOffsets.at(offNode->fieldName);
                    } else {
                        errors.push_back("Field '" + offNode->fieldName + "' does not exist in Record '" + offNode->recordName + "'");
                    }
                } else {
                    errors.push_back("Record '" + offNode->recordName + "' not found for @offsetof");
                }
                return {TypeKind::Int64, "Int"};
            }

            case ASTNodeType::NativeGetReg:
            case ASTNodeType::NativePortIn:
            case ASTNodeType::NativeMmioRead:
                return {TypeKind::Int64, "Int"};

            case ASTNodeType::NativeSetReg:
            case ASTNodeType::NativePortOut:
            case ASTNodeType::NativeMmioWrite:
            case ASTNodeType::NativeHalt:
            case ASTNodeType::NativeCli:
            case ASTNodeType::NativeSti:
                return {TypeKind::Void, "void"};

            case ASTNodeType::Var: {
                auto v = static_cast<VarNode*>(node);
                return lookupVar(v->name);
            }

            case ASTNodeType::ArrAccess: {
                auto a = static_cast<ArrAccessNode*>(node);
                checkNode(a->index.get());
                return {TypeKind::Int64, "Int"};
            }

            case ASTNodeType::FuncCall: {
                auto call = static_cast<FuncCallNode*>(node);
                std::string fnName = "";
                if (call->callee && call->callee->type == ASTNodeType::Var) {
                    fnName = static_cast<VarNode*>(call->callee.get())->name;
                }
                if (functionReturnTypes.count(fnName)) {
                    return functionReturnTypes[fnName];
                }
                return {TypeKind::Dynamic, "dynamic"};
            }

            case ASTNodeType::VarAssign: {
                auto assign = static_cast<VarAssignNode*>(node);
                TypeInfo rhsType = checkNode(assign->value.get());
                TypeInfo varType = rhsType;

                if (!assign->explicitType.empty()) {
                    TypeInfo declared = resolveTypeName(assign->explicitType);
                    if (declared.kind != TypeKind::Dynamic && rhsType.kind != TypeKind::Dynamic && 
                        declared.kind != rhsType.kind && rhsType.kind != TypeKind::Error) {
                        errors.push_back("Type mismatch on assignment to '" + assign->name + "'.");
                    }
                    varType = declared;
                }
                declareVar(assign->name, varType);
                return varType;
            }

            case ASTNodeType::ArrAssign: {
                auto assign = static_cast<ArrAssignNode*>(node);
                checkNode(assign->index.get());
                return checkNode(assign->value.get());
            }

            case ASTNodeType::Binary: {
                auto bin = static_cast<BinaryNode*>(node);
                TypeInfo left = checkNode(bin->left.get());
                TypeInfo right = checkNode(bin->right.get());

                if (left.kind == TypeKind::Float64 || right.kind == TypeKind::Float64) {
                    return {TypeKind::Float64, "Float"};
                }
                if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == "<=" || bin->op == ">" || bin->op == ">=") {
                    return {TypeKind::Bool, "Bool"};
                }
                return left;
            }

            case ASTNodeType::FuncDecl: {
                auto fn = static_cast<FuncDeclNode*>(node);
                enterScope();
                for (const auto& param : fn->params) {
                    declareVar(param.name, resolveTypeName(param.type));
                }
                checkNode(fn->body.get());
                exitScope();
                return resolveTypeName(fn->returnType);
            }

            case ASTNodeType::Block: {
                auto block = static_cast<BlockNode*>(node);
                enterScope();
                for (const auto& s : block->stmts) checkNode(s.get());
                exitScope();
                return {TypeKind::Void, "void"};
            }

            case ASTNodeType::If: {
                auto ifNode = static_cast<IfNode*>(node);
                checkNode(ifNode->cond.get());
                checkNode(ifNode->thenBranch.get());
                if (ifNode->elseBranch) checkNode(ifNode->elseBranch.get());
                return {TypeKind::Void, "void"};
            }

            case ASTNodeType::While: {
                auto whileNode = static_cast<WhileNode*>(node);
                checkNode(whileNode->cond.get());
                checkNode(whileNode->body.get());
                return {TypeKind::Void, "void"};
            }

            case ASTNodeType::For: {
                auto forNode = static_cast<ForNode*>(node);
                enterScope();
                declareVar(forNode->var, {TypeKind::Int64, "Int"});
                checkNode(forNode->iterable.get());
                checkNode(forNode->body.get());
                exitScope();
                return {TypeKind::Void, "void"};
            }

            case ASTNodeType::Return: {
                auto ret = static_cast<ReturnNode*>(node);
                if (ret->expr) return checkNode(ret->expr.get());
                return {TypeKind::Void, "void"};
            }

            default: break;
        }
        return {TypeKind::Dynamic, "dynamic"};
    }
};

#endif 