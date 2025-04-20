#pragma once

#include "ParserRuleContext.h"

using namespace std;

enum class ErrorType
{
  Error,
  Warning
};

class ErrorListenerVisitor
{
public:
  static inline bool hasError() { return mHasError; }
  static void addError(antlr4::ParserRuleContext *ctx,
                       const string &message,
                       ErrorType errorType = ErrorType::Error);
  static void addError(const string &message, int line,
                       ErrorType errorType = ErrorType::Error);
  static void addError(const string &message,
                       ErrorType errorType = ErrorType::Error);

protected:
  static bool mHasError;
};