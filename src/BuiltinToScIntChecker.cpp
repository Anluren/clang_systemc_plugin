#include "BuiltinToScIntChecker.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Type.h"
#include "clang/Basic/ParsedAttrInfo.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"

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

bool ScIntAssignConsumer::HandleTopLevelDecl(clang::DeclGroupRef DG) {
  for (Decl *D : DG) {
    if (!D)
      continue;
    ScIntAssignVisitor Visitor(D->getASTContext());
    Visitor.TraverseDecl(D);
  }
  return true;
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

// --- sc_dt type annotator visitor ---

namespace {
bool isInScDtNamespace(const DeclContext *DC) {
  const DeclContext *Current = DC;
  while (Current) {
    const auto *Ns = dyn_cast<NamespaceDecl>(Current);
    if (Ns && Ns->getName() == "sc_dt")
      return true;
    Current = Current->getParent();
  }
  return false;
}
} // namespace

namespace {
std::optional<std::string> classifyScDtType(ASTContext &Context,
                                            ScDtTypeCache &Cache,
                                            QualType Type) {
  QualType CanonicalType = Context.getCanonicalType(Type);
  const clang::Type *TypeKey = CanonicalType.getTypePtrOrNull();
  if (!TypeKey)
    return std::nullopt;

  if (auto It = Cache.Known.find(TypeKey); It != Cache.Known.end())
    return It->second;

  if (Cache.Negative.contains(TypeKey))
    return std::nullopt;

  const CXXRecordDecl *Record = CanonicalType->getAsCXXRecordDecl();
  if (!Record) {
    Cache.Negative.insert(TypeKey);
    return std::nullopt;
  }

  const CXXRecordDecl *CanonicalRecord = Record->getCanonicalDecl();
  if (!CanonicalRecord) {
    Cache.Negative.insert(TypeKey);
    return std::nullopt;
  }

  if (!isInScDtNamespace(CanonicalRecord->getDeclContext())) {
    Cache.Negative.insert(TypeKey);
    return std::nullopt;
  }

  std::string TypeName = CanonicalRecord->getNameAsString();
  if (TypeName.empty()) {
    Cache.Negative.insert(TypeKey);
    return std::nullopt;
  }

  Cache.Known.try_emplace(TypeKey, TypeName);
  return TypeName;
}
} // namespace

namespace {
class ScDtCustomAttrInfo final : public ParsedAttrInfo {
public:
  ScDtCustomAttrInfo() {
    static constexpr Spelling SupportedSpellings[] = {
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_int"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_uint"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_bigint"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_biguint"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_bv"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_lv"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_fixed"},
        {AttributeCommonInfo::AS_CXX11, "sc_dt::sc_ufixed"},
    };
    Spellings = SupportedSpellings;
    NumArgs = 0;
    OptArgs = 0;
  }

  AttrHandling handleDeclAttribute(Sema &S, Decl *D,
                                   const ParsedAttr &Attr) const override {
    auto *Named = dyn_cast<NamedDecl>(D);
    if (!Named)
      return AttributeNotApplied;

    if (!Attr.checkAtMostNumArgs(S, 0))
      return AttributeNotApplied;

    std::string Annotation = Attr.getNormalizedFullName();

    for (const auto *A : Named->specific_attrs<AnnotateAttr>()) {
      if (A->getAnnotation() == Annotation)
        return AttributeApplied;
    }

    Named->addAttr(AnnotateAttr::CreateImplicit(S.Context, Annotation, nullptr,
                                                0));
    return AttributeApplied;
  }
};
} // namespace

void ScDtTypeCollectorVisitor::collectType(QualType Type) {
  if (Type.isNull())
    return;
  (void)classifyScDtType(Context, Cache, Type);
}

bool ScDtTypeCollectorVisitor::VisitVarDecl(VarDecl *Decl) {
  collectType(Decl->getType());
  return true;
}

bool ScDtTypeCollectorVisitor::VisitFieldDecl(FieldDecl *Decl) {
  collectType(Decl->getType());
  return true;
}

bool ScDtTypeCollectorVisitor::VisitParmVarDecl(ParmVarDecl *Decl) {
  collectType(Decl->getType());
  return true;
}

bool ScDtTypeCollectorVisitor::VisitFunctionDecl(FunctionDecl *Decl) {
  if (!Decl)
    return true;

  collectType(Decl->getReturnType());
  for (const ParmVarDecl *Param : Decl->parameters()) {
    if (Param)
      collectType(Param->getType());
  }
  return true;
}

bool ScDtTypeCollectorVisitor::VisitTypedefNameDecl(TypedefNameDecl *Decl) {
  collectType(Decl->getUnderlyingType());
  return true;
}

std::optional<std::string>
ScDtTypeAnnotatorVisitor::getScDtTypeName(QualType Type) const {
  return classifyScDtType(Context, Cache, Type);
}

void ScDtTypeAnnotatorVisitor::maybeRecordTypeUse(const NamedDecl *Decl,
                                                  QualType Type) {
  if (!Decl || Type.isNull())
    return;

  SourceManager &SourceMgr = Context.getSourceManager();
  SourceLocation Loc = Decl->getLocation();
  if (Loc.isInvalid() || SourceMgr.isInSystemHeader(Loc))
    return;

  std::optional<std::string> TypeName = getScDtTypeName(Type);
  if (!TypeName.has_value())
    return;

  if (!ReportSideTable)
    return;

  const clang::Type *TypeKey = Context.getCanonicalType(Type).getTypePtrOrNull();
  if (!TypeKey || !Cache.Reported.insert(TypeKey).second)
    return;

  DiagnosticsEngine &Diag = Context.getDiagnostics();
  unsigned DiagId = Diag.getCustomDiagID(
      DiagnosticsEngine::Remark,
      "side table cached sc_dt type '%0'");
  Diag.Report(Loc, DiagId) << ("sc_dt::" + *TypeName);
}

bool ScDtTypeAnnotatorVisitor::VisitVarDecl(VarDecl *Decl) {
  maybeRecordTypeUse(Decl, Decl->getType());
  return true;
}

bool ScDtTypeAnnotatorVisitor::VisitFieldDecl(FieldDecl *Decl) {
  maybeRecordTypeUse(Decl, Decl->getType());
  return true;
}

bool ScDtTypeAnnotatorVisitor::VisitParmVarDecl(ParmVarDecl *Decl) {
  maybeRecordTypeUse(Decl, Decl->getType());
  return true;
}

bool ScDtTypeAnnotatorVisitor::VisitFunctionDecl(FunctionDecl *Decl) {
  if (!Decl || !Decl->doesThisDeclarationHaveABody())
    return true;

  if (getScDtTypeName(Decl->getReturnType()).has_value()) {
    maybeRecordTypeUse(Decl, Decl->getReturnType());
    return true;
  }

  for (const ParmVarDecl *Param : Decl->parameters()) {
    if (!Param)
      continue;
    if (getScDtTypeName(Param->getType()).has_value()) {
      maybeRecordTypeUse(Decl, Param->getType());
      return true;
    }
  }

  return true;
}

bool ScDtTypeAnnotatorVisitor::VisitTypedefNameDecl(TypedefNameDecl *Decl) {
  maybeRecordTypeUse(Decl, Decl->getUnderlyingType());
  return true;
}

std::optional<std::string> ScDtTypeLookupVisitor::queryCache(QualType Type) {
  QualType CanonicalType = Context.getCanonicalType(Type);
  const clang::Type *TypeKey = CanonicalType.getTypePtrOrNull();
  if (!TypeKey)
    return std::nullopt;

  auto It = Cache.Known.find(TypeKey);
  if (It != Cache.Known.end())
    return It->second;

  return std::nullopt;
}

void ScDtTypeLookupVisitor::maybeReportCacheLookup(const NamedDecl *Decl,
                                                    QualType Type) {
  if (!Decl || Type.isNull() || !ReportCacheLookups)
    return;

  SourceManager &SourceMgr = Context.getSourceManager();
  SourceLocation Loc = Decl->getLocation();
  if (Loc.isInvalid() || SourceMgr.isInSystemHeader(Loc))
    return;

  std::optional<std::string> TypeName = queryCache(Type);
  if (!TypeName.has_value())
    return;

  DiagnosticsEngine &Diag = Context.getDiagnostics();
  unsigned DiagId = Diag.getCustomDiagID(
      DiagnosticsEngine::Remark,
      "cache lookup found sc_dt type '%0'");
  Diag.Report(Loc, DiagId) << ("sc_dt::" + *TypeName);
}

bool ScDtTypeLookupVisitor::VisitVarDecl(VarDecl *Decl) {
  maybeReportCacheLookup(Decl, Decl->getType());
  return true;
}

bool ScDtTypeLookupVisitor::VisitFieldDecl(FieldDecl *Decl) {
  maybeReportCacheLookup(Decl, Decl->getType());
  return true;
}

bool ScDtTypeLookupVisitor::VisitParmVarDecl(ParmVarDecl *Decl) {
  maybeReportCacheLookup(Decl, Decl->getType());
  return true;
}

bool ScDtTypeLookupVisitor::VisitFunctionDecl(FunctionDecl *Decl) {
  if (!Decl)
    return true;

  maybeReportCacheLookup(Decl, Decl->getReturnType());
  for (const ParmVarDecl *Param : Decl->parameters()) {
    if (Param)
      maybeReportCacheLookup(Param, Param->getType());
  }
  return true;
}

bool ScDtTypeLookupVisitor::VisitTypedefNameDecl(TypedefNameDecl *Decl) {
  maybeReportCacheLookup(Decl, Decl->getUnderlyingType());
  return true;
}

// --- ASTConsumer ---

bool ScDtTypeAnnotatorConsumer::HandleTopLevelDecl(clang::DeclGroupRef DG) {
  for (Decl *D : DG) {
    if (!D)
      continue;

    ScDtTypeCollectorVisitor Collector(D->getASTContext(), Cache);
    Collector.TraverseDecl(D);

    ScDtTypeAnnotatorVisitor Visitor(D->getASTContext(), Cache,
                                     ReportSideTable);
    Visitor.TraverseDecl(D);

    ScDtTypeLookupVisitor Lookup(D->getASTContext(), Cache,
                                 ReportCacheLookups);
    Lookup.TraverseDecl(D);
  }
  return true;
}

// --- PluginASTAction ---

std::unique_ptr<ASTConsumer> ScDtTypeAnnotatorAction::CreateASTConsumer(
    CompilerInstance &Ci, StringRef InFile) {
  return std::make_unique<ScDtTypeAnnotatorConsumer>(ReportSideTable,
                                                      ReportCacheLookups);
}

bool ScDtTypeAnnotatorAction::ParseArgs(const CompilerInstance &Ci,
                                        const std::vector<std::string> &Args) {
  for (const std::string &Arg : Args) {
    if (Arg == "report-side-table") {
      ReportSideTable = true;
      continue;
    }
    if (Arg == "report-cache-lookups") {
      ReportCacheLookups = true;
      continue;
    }
    DiagnosticsEngine &Diag = Ci.getDiagnostics();
    unsigned DiagId = Diag.getCustomDiagID(
        DiagnosticsEngine::Error,
        "unknown sc-dt-type-annotator argument '%0'");
    Diag.Report(DiagId) << Arg;
    return false;
  }
  return true;
}

// Register the plugin with clang.
static FrontendPluginRegistry::Add<ScIntAssignAction>
    X("sc-int-assign-checker",
      "Checks builtin-type variable assignment to sc_int/sc_uint");

static FrontendPluginRegistry::Add<ScDtTypeAnnotatorAction>
    Y("sc-dt-type-annotator",
      "Annotates declarations with sc_dt::<type> attributes");

static ParsedAttrInfoRegistry::Add<ScDtCustomAttrInfo>
    Z("sc-dt-custom-attr",
      "Handles [[sc_dt::...]] attributes and maps them to annotate metadata");
