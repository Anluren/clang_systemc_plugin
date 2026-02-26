#include "BuiltinToScIntChecker.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory
    ScIntCheckerCategory("sc_int assignment checker options");

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
    "\nDetects assignments of builtin-type variables to "
    "sc_int<W>/sc_uint<W> types.\n");

int main(int argc, const char **argv) {
  auto OptionsParser =
      CommonOptionsParser::create(argc, argv, ScIntCheckerCategory);

  if (!OptionsParser) {
    llvm::errs() << OptionsParser.takeError();
    return 1;
  }

  ClangTool Tool(OptionsParser->getCompilations(),
                 OptionsParser->getSourcePathList());

  BuiltinToScIntChecker Checker;
  clang::ast_matchers::MatchFinder Finder;
  Checker.registerMatchers(Finder);

  int Result = Tool.run(newFrontendActionFactory(&Finder).get());

  if (Checker.getViolationCount() > 0) {
    llvm::errs() << "\nFound " << Checker.getViolationCount()
                 << " builtin-to-sc_int/sc_uint assignment violation(s).\n";
    return 1;
  }

  return Result;
}
