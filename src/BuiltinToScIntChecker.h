#ifndef BUILTIN_TO_SC_INT_CHECKER_H
#define BUILTIN_TO_SC_INT_CHECKER_H

#include "clang/ASTMatchers/ASTMatchFinder.h"

class BuiltinToScIntChecker
    : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void registerMatchers(clang::ast_matchers::MatchFinder &Finder);

  unsigned getViolationCount() const { return ViolationCount; }

private:
  unsigned ViolationCount = 0;
};

#endif // BUILTIN_TO_SC_INT_CHECKER_H
