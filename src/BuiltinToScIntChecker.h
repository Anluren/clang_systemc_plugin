#ifndef BUILTIN_TO_SC_INT_CHECKER_H
#define BUILTIN_TO_SC_INT_CHECKER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <optional>
#include <string>

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

// Shared state for sc_dt type classification across multiple passes in the
// same frontend invocation.
struct ScDtTypeCache {
  llvm::DenseMap<const clang::Type *, std::string> Known;
  llvm::DenseSet<const clang::Type *> Negative;
  llvm::DenseSet<const clang::Type *> Reported;
};

// RecursiveASTVisitor that populates the shared sc_dt type cache as soon as
// declarations are available from the frontend.
class ScDtTypeCollectorVisitor
    : public clang::RecursiveASTVisitor<ScDtTypeCollectorVisitor> {
public:
  ScDtTypeCollectorVisitor(clang::ASTContext &Context, ScDtTypeCache &Cache)
      : Context(Context), Cache(Cache) {}

  bool VisitVarDecl(clang::VarDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitParmVarDecl(clang::ParmVarDecl *Decl);
  bool VisitFunctionDecl(clang::FunctionDecl *Decl);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *Decl);

private:
  void collectType(clang::QualType Type);
  clang::ASTContext &Context;
  ScDtTypeCache &Cache;
};

// RecursiveASTVisitor that consumes the shared sc_dt type cache for later
// passes in the same frontend invocation.
class ScDtTypeAnnotatorVisitor
    : public clang::RecursiveASTVisitor<ScDtTypeAnnotatorVisitor> {
public:
  ScDtTypeAnnotatorVisitor(clang::ASTContext &Context, ScDtTypeCache &Cache,
                           bool ReportSideTable)
      : Context(Context), Cache(Cache),
        ReportSideTable(ReportSideTable) {}

  bool VisitVarDecl(clang::VarDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitParmVarDecl(clang::ParmVarDecl *Decl);
  bool VisitFunctionDecl(clang::FunctionDecl *Decl);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *Decl);

private:
  std::optional<std::string> getScDtTypeName(clang::QualType Type) const;
  void maybeRecordTypeUse(const clang::NamedDecl *Decl, clang::QualType Type);
  clang::ASTContext &Context;
  ScDtTypeCache &Cache;
  bool ReportSideTable;
};

// RecursiveASTVisitor demonstrating how later passes in the same frontend
// invocation consume the shared sc_dt type cache without re-classifying.
class ScDtTypeLookupVisitor
    : public clang::RecursiveASTVisitor<ScDtTypeLookupVisitor> {
public:
  ScDtTypeLookupVisitor(clang::ASTContext &Context, ScDtTypeCache &Cache,
                        bool ReportCacheLookups)
      : Context(Context), Cache(Cache), ReportCacheLookups(ReportCacheLookups) {}

  bool VisitVarDecl(clang::VarDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitParmVarDecl(clang::ParmVarDecl *Decl);
  bool VisitFunctionDecl(clang::FunctionDecl *Decl);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *Decl);

private:
  std::optional<std::string> queryCache(clang::QualType Type);
  void maybeReportCacheLookup(const clang::NamedDecl *Decl,
                              clang::QualType Type);
  clang::ASTContext &Context;
  ScDtTypeCache &Cache;
  bool ReportCacheLookups;
};

// ASTConsumer that drives sc_dt type annotation over the translation unit.
class ScDtTypeAnnotatorConsumer : public clang::ASTConsumer {
public:
  explicit ScDtTypeAnnotatorConsumer(bool ReportSideTable,
                                      bool ReportCacheLookups = false)
      : ReportSideTable(ReportSideTable), ReportCacheLookups(ReportCacheLookups) {}

  bool HandleTopLevelDecl(clang::DeclGroupRef DG) override;

private:
  ScDtTypeCache Cache;
  bool ReportSideTable;
  bool ReportCacheLookups;

  friend class ScDtTypeAnnotatorAction;
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

private:
  bool ReportSideTable = false;
  bool ReportCacheLookups = false;
};

#endif // BUILTIN_TO_SC_INT_CHECKER_H
