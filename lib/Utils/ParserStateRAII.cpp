//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#include "cling/Utils/ParserStateRAII.h"

#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"

using namespace clang;

cling::ParserStateRAII::ParserStateRAII(Parser& p, bool skipToEOF)
  : P(&p), PP(p.getPreprocessor()),
    PPDiagHadErrors(PP.getDiagnostics().hasErrorOccurred()),
    SemaDiagHadErrors(P->getActions().getDiagnostics().hasErrorOccurred()),
    OldSuppressAllDiagnostics(P->getActions().getDiagnostics()
                              .getSuppressAllDiagnostics()),
    OldPPSuppressAllDiagnostics(p.getPreprocessor().getDiagnostics()
                                .getSuppressAllDiagnostics()),
    OldSpellChecking(p.getPreprocessor().getLangOpts().SpellChecking),
  OldTok(p.Tok), OldPrevTokLocation(p.PrevTokLocation),
  OldParenCount(p.ParenCount), OldBracketCount(p.BracketCount),
  OldBraceCount(p.BraceCount),
  OldTemplateParameterDepth(p.TemplateParameterDepth),
  OldInNonInstantiationSFINAEContext(P->getActions()
                                     .InNonInstantiationSFINAEContext),
  SkipToEOF(skipToEOF),
  ResetExprEvalCtx(p.getActions(), clang::Sema::ExpressionEvaluationContext::PotentiallyEvaluated)
{
  // Set to defaults, reset to previous values by ~ParserStateRAII().
  OldTemplateIds.swap(P->TemplateIds);
  P->ParenCount = 0;
  P->BracketCount = 0;
  P->BraceCount = 0;
  P->TemplateParameterDepth = 0;
  P->getActions().InNonInstantiationSFINAEContext = false;
}

cling::ParserStateRAII::~ParserStateRAII() {
  //
  // Advance the parser to the end of the file, and pop the include stack.
  //
  // Note: Consuming the EOF token will pop the include stack.
  //
  {
    // Forcefully clean up the TemplateIds, ignoring additional checks in
    // MaybeDestroyTemplateIds called from DestroyTemplateIdAnnotationsRAIIObj,
    // before swapping the previous set back.
    P->DestroyTemplateIds();
  }
  P->TemplateIds.swap(OldTemplateIds);
  assert(OldTemplateIds.empty());
  if (SkipToEOF)
    P->SkipUntil(tok::annot_repl_input_end);
  else
    P->Tok = OldTok;
  if (!SemaDiagHadErrors) {
    // Doesn't reset the diagnostic mappings
    P->getActions().getDiagnostics().Reset(/*soft=*/true);
  }
  P->getActions().getDiagnostics().setSuppressAllDiagnostics(OldSuppressAllDiagnostics);
  if (!PPDiagHadErrors)
    PP.getDiagnostics().Reset(/*soft=*/true);
  PP.getDiagnostics().setSuppressAllDiagnostics(OldPPSuppressAllDiagnostics);
  const_cast<LangOptions&>(PP.getLangOpts()).SpellChecking =
    OldSpellChecking;

  P->PrevTokLocation = OldPrevTokLocation;
  P->ParenCount = OldParenCount;
  P->BracketCount = OldBracketCount;
  P->BraceCount = OldBraceCount;
  P->TemplateParameterDepth = OldTemplateParameterDepth;
  P->getActions().InNonInstantiationSFINAEContext =
    OldInNonInstantiationSFINAEContext;
}
