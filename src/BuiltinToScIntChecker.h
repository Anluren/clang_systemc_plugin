#ifndef BUILTIN_TO_SC_INT_CHECKER_H
#define BUILTIN_TO_SC_INT_CHECKER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"

// RecursiveASTVisitor that checks for builtin-type variable assignments
// to sc_int<W>/sc_uint<W> types.
class ScIntAssignVisitor
    : public clang::RecursiveASTVisitor<ScIntAssignVisitor> {
public:
  explicit ScIntAssignVisitor(clang::ASTContext &Context) : Context(Context) {}

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *E);

private:
  bool isScIntOrUintClass(const clang::CXXRecordDecl *RD);
  clang::ASTContext &Context;
};

// ASTConsumer that drives the visitor over the translation unit.
class ScIntAssignConsumer : public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext &Ctx) override;
};

// PluginASTAction registered with clang's plugin registry.
class ScIntAssignAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI,
                    llvm::StringRef InFile) override;

  bool ParseArgs(const clang::CompilerInstance &CI,
                 const std::vector<std::string> &Args) override;

  ActionType getActionType() override { return AddBeforeMainAction; }
};

#endif // BUILTIN_TO_SC_INT_CHECKER_H
