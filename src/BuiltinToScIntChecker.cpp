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

// Check whether RD is, or derives from, sc_dt::sc_int_base or
// sc_dt::sc_uint_base.
bool ScIntAssignVisitor::isScIntOrUintClass(const CXXRecordDecl *RD) {
  if (!RD)
    return false;

  // Get the fully-qualified name for direct match.
  std::string QualName = RD->getQualifiedNameAsString();
  if (QualName == "sc_dt::sc_int_base" || QualName == "sc_dt::sc_uint_base")
    return true;

  // Check base classes recursively.
  if (!RD->hasDefinition())
    return false;
  for (const auto &Base : RD->bases()) {
    const auto *BaseRD =
        Base.getType()->getAsCXXRecordDecl();
    if (isScIntOrUintClass(BaseRD))
      return true;
  }
  return false;
}

bool ScIntAssignVisitor::VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
  // Only interested in assignment operators (=, +=, -=, etc.)
  if (!E->isAssignmentOp())
    return true;

  SourceManager &SM = Context.getSourceManager();
  SourceLocation Loc = E->getOperatorLoc();

  // Skip assignments inside system headers (e.g. SystemC internals).
  if (SM.isInSystemHeader(Loc))
    return true;

  // The callee must be a CXXMethodDecl belonging to an sc_int/sc_uint class.
  const auto *Method = dyn_cast_or_null<CXXMethodDecl>(E->getCalleeDecl());
  if (!Method)
    return true;

  const CXXRecordDecl *ParentClass = Method->getParent();
  if (!isScIntOrUintClass(ParentClass))
    return true;

  // Arg 0 is the implicit object (LHS). Arg 1 is the RHS value.
  if (E->getNumArgs() < 2)
    return true;

  const Expr *RHS = E->getArg(1)->IgnoreImpCasts();
  const auto *RHSRef = dyn_cast<DeclRefExpr>(RHS);
  if (!RHSRef)
    return true;

  // Only flag when the RHS variable has a builtin type.
  if (!RHSRef->getType()->isBuiltinType())
    return true;

  // Emit the warning.
  DiagnosticsEngine &Diag = Context.getDiagnostics();

  std::string VarName = RHSRef->getNameInfo().getAsString();
  QualType LHSType = E->getArg(0)->getType();
  QualType RHSType = RHSRef->getType();

  unsigned DiagID = Diag.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "assignment of builtin-type variable '%0' (type '%1') to "
      "sc_int/sc_uint type '%2' is not allowed");

  Diag.Report(Loc, DiagID) << VarName << RHSType.getAsString()
                            << LHSType.getAsString();

  return true;
}

// --- ASTConsumer ---

void ScIntAssignConsumer::HandleTranslationUnit(ASTContext &Ctx) {
  ScIntAssignVisitor Visitor(Ctx);
  Visitor.TraverseAST(Ctx);
}

// --- PluginASTAction ---

std::unique_ptr<ASTConsumer>
ScIntAssignAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  return std::make_unique<ScIntAssignConsumer>();
}

bool ScIntAssignAction::ParseArgs(const CompilerInstance &CI,
                                  const std::vector<std::string> &Args) {
  return true;
}

// Register the plugin with clang.
static FrontendPluginRegistry::Add<ScIntAssignAction>
    X("sc-int-assign-checker",
      "Checks builtin-type variable assignment to sc_int/sc_uint");
