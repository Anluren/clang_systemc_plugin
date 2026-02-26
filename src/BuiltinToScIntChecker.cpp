#include "BuiltinToScIntChecker.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace clang;

// Check whether Record is, or derives from, sc_dt::sc_int_base or
// sc_dt::sc_uint_base.
bool ScIntAssignVisitor::isScIntOrUintClass(const CXXRecordDecl *Record) {
  if (!Record)
    return false;

  // Get the fully-qualified name for direct match.
  std::string QualName = Record->getQualifiedNameAsString();
  if (QualName == "sc_dt::sc_int_base" || QualName == "sc_dt::sc_uint_base")
    return true;

  // Check base classes recursively.
  if (!Record->hasDefinition())
    return false;
  for (const auto &Base : Record->bases()) {
    const auto *BaseRecord = Base.getType()->getAsCXXRecordDecl();
    if (isScIntOrUintClass(BaseRecord))
      return true;
  }
  return false;
}

bool ScIntAssignVisitor::VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Expr) {
  // Only interested in assignment operators (=, +=, -=, etc.)
  if (!Expr->isAssignmentOp())
    return true;

  SourceManager &SourceMgr = Context.getSourceManager();
  SourceLocation Loc = Expr->getOperatorLoc();

  // Skip assignments inside system headers (e.g. SystemC internals).
  if (SourceMgr.isInSystemHeader(Loc))
    return true;

  // The callee must be a CXXMethodDecl belonging to an sc_int/sc_uint class.
  const auto *Method = dyn_cast_or_null<CXXMethodDecl>(Expr->getCalleeDecl());
  if (!Method)
    return true;

  const CXXRecordDecl *ParentClass = Method->getParent();
  if (!isScIntOrUintClass(ParentClass))
    return true;

  // Arg 0 is the implicit object (LHS). Arg 1 is the RHS value.
  if (Expr->getNumArgs() < 2)
    return true;

  const clang::Expr *Rhs = Expr->getArg(1)->IgnoreImpCasts();
  const auto *RhsRef = dyn_cast<DeclRefExpr>(Rhs);
  if (!RhsRef)
    return true;

  // Only flag when the RHS variable has a builtin type.
  if (!RhsRef->getType()->isBuiltinType())
    return true;

  // Emit the warning.
  DiagnosticsEngine &Diag = Context.getDiagnostics();

  std::string VarName = RhsRef->getNameInfo().getAsString();
  QualType LhsType = Expr->getArg(0)->getType();
  QualType RhsType = RhsRef->getType();

  unsigned DiagId = Diag.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "assignment of builtin-type variable '%0' (type '%1') to "
      "sc_int/sc_uint type '%2' is not allowed");

  Diag.Report(Loc, DiagId) << VarName << RhsType.getAsString()
                           << LhsType.getAsString();

  return true;
}

// --- ASTConsumer ---

void ScIntAssignConsumer::HandleTranslationUnit(ASTContext &Ctx) {
  ScIntAssignVisitor Visitor(Ctx);
  Visitor.TraverseAST(Ctx);
}

// --- PluginASTAction ---

std::unique_ptr<ASTConsumer>
ScIntAssignAction::CreateASTConsumer(CompilerInstance &Ci, StringRef InFile) {
  return std::make_unique<ScIntAssignConsumer>();
}

bool ScIntAssignAction::ParseArgs(const CompilerInstance &Ci,
                                  const std::vector<std::string> &Args) {
  return true;
}

// Register the plugin with clang.
static FrontendPluginRegistry::Add<ScIntAssignAction>
    X("sc-int-assign-checker",
      "Checks builtin-type variable assignment to sc_int/sc_uint");
