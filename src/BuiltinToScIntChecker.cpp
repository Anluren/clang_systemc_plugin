#include "BuiltinToScIntChecker.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

static const char *BindAssignment = "assignment";
static const char *BindRHS = "rhs";

void BuiltinToScIntChecker::registerMatchers(MatchFinder &Finder) {
  // Match CXXOperatorCallExpr for assignment operators (=, +=, -=, etc.)
  // where the callee method belongs to a class derived from sc_int_base or
  // sc_uint_base, and the RHS is a variable reference with a builtin type.
  auto ScIntOrUintBase = cxxRecordDecl(isSameOrDerivedFrom(
      hasAnyName("::sc_dt::sc_int_base", "::sc_dt::sc_uint_base")));

  Finder.addMatcher(
      cxxOperatorCallExpr(
          isAssignmentOperator(),
          callee(cxxMethodDecl(ofClass(ScIntOrUintBase))),
          hasArgument(
              1, ignoringImpCasts(
                     declRefExpr(hasType(builtinType())).bind(BindRHS))))
          .bind(BindAssignment),
      this);
}

void BuiltinToScIntChecker::run(const MatchFinder::MatchResult &Result) {
  const auto *Assignment =
      Result.Nodes.getNodeAs<CXXOperatorCallExpr>(BindAssignment);
  const auto *RHSVar = Result.Nodes.getNodeAs<DeclRefExpr>(BindRHS);

  if (!Assignment || !RHSVar)
    return;

  SourceManager &SM = *Result.SourceManager;
  SourceLocation Loc = Assignment->getOperatorLoc();

  // Skip matches in system headers (e.g. inside SystemC's own headers)
  if (SM.isInSystemHeader(Loc))
    return;

  DiagnosticsEngine &Diag = Result.Context->getDiagnostics();

  std::string VarName = RHSVar->getNameInfo().getAsString();
  QualType LHSType = Assignment->getArg(0)->getType();
  QualType RHSType = RHSVar->getType();

  unsigned DiagID = Diag.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "assignment of builtin-type variable '%0' (type '%1') to "
      "sc_int/sc_uint type '%2' is not allowed");

  Diag.Report(Loc, DiagID) << VarName << RHSType.getAsString()
                            << LHSType.getAsString();

  ++ViolationCount;
}
