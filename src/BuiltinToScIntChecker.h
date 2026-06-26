#ifndef BUILTIN_TO_SC_INT_CHECKER_H
#define BUILTIN_TO_SC_INT_CHECKER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include <optional>

// RecursiveASTVisitor that checks for builtin-type variable assignments
// to sc_int<W>/sc_uint<W> types.
class ScIntAssignVisitor
    : public clang::RecursiveASTVisitor<ScIntAssignVisitor> {
public:
  explicit ScIntAssignVisitor(clang::ASTContext &Context) : Context(Context) {}

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *Expr);

private:
  bool isScIntOrUintClass(const clang::CXXRecordDecl *Record);
  clang::ASTContext &Context;
};

// ASTConsumer that drives the visitor over the translation unit.
class ScIntAssignConsumer : public clang::ASTConsumer {
public:
  bool HandleTopLevelDecl(clang::DeclGroupRef DG) override;
};

// PluginASTAction registered with clang's plugin registry.
class ScIntAssignAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Ci,
                    llvm::StringRef InFile) override;

  bool ParseArgs(const clang::CompilerInstance &Ci,
                 const std::vector<std::string> &Args) override;

  ActionType getActionType() override { return AddBeforeMainAction; }
};

// RecursiveASTVisitor that tags declarations typed with sc_dt::* types using
// an implicit AnnotateAttr value "sc_dt::<type>".
class ScDtTypeAnnotatorVisitor
    : public clang::RecursiveASTVisitor<ScDtTypeAnnotatorVisitor> {
public:
  explicit ScDtTypeAnnotatorVisitor(clang::ASTContext &Context)
      : Context(Context) {}

  bool VisitVarDecl(clang::VarDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitParmVarDecl(clang::ParmVarDecl *Decl);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *Decl);

private:
  std::optional<std::string> getScDtTypeName(clang::QualType Type) const;
  void maybeAnnotateDecl(clang::NamedDecl *Decl, clang::QualType Type);
  clang::ASTContext &Context;
};

// ASTConsumer that drives sc_dt type annotation over the translation unit.
class ScDtTypeAnnotatorConsumer : public clang::ASTConsumer {
public:
  bool HandleTopLevelDecl(clang::DeclGroupRef DG) override;
};

// PluginASTAction for adding sc_dt::<type> annotations to declarations.
class ScDtTypeAnnotatorAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Ci,
                    llvm::StringRef InFile) override;

  bool ParseArgs(const clang::CompilerInstance &Ci,
                 const std::vector<std::string> &Args) override;

  ActionType getActionType() override { return AddBeforeMainAction; }
};

#endif // BUILTIN_TO_SC_INT_CHECKER_H
