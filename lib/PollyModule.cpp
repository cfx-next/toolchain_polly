//===------ ModuleInitializers.cpp - Initialize the Polly Module ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "polly/RegisterPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {

/// @brief Initialize Polly passes when library is loaded.
///
/// We use the constructor of a statically declared object to initialize the
/// different Polly passes right after the Polly library is loaded. This ensures
/// that the Polly passes are available e.g. in the 'opt' tool.
class StaticInitializer {
public:
  StaticInitializer() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    polly::initializePollyPasses(Registry);
  }
};
static StaticInitializer InitializeEverything;

static void
registerPollyEarlyAsPossiblePasses(const llvm::PassManagerBuilder &Builder,
                                   llvm::PassManagerBase &PM) {

  if (!polly::shouldEnablePolly())
    return;

  polly::registerPollyPasses(PM);
}

/// @brief Register Polly to be available as an optimizer
///
/// We currently register Polly such that it runs as early as possible. This has
/// several implications:
///
///   1) We need to schedule more canonicalization passes
///
///   As nothing is run before Polly, it is necessary to run a set of preparing
///   transformations before Polly to canonicalize the LLVM-IR and to allow
///   Polly to detect and understand the code.
///
///   2) LICM and LoopIdiom pass have not yet been run
///
///   Loop invariant code motion as well as the loop idiom recognition pass make
///   it more difficult for Polly to transform code. LICM may introduce
///   additional data dependences that are hard to eliminate and the loop idiom
///   recognition pass may introduce calls to memset that we currently do not
///   understand. By running Polly early enough (meaning before these passes) we
///   avoid difficulties that may be introduced by these passes.
///
///   3) We get the full -O3 optimization sequence after Polly
///
///   The LLVM-IR that is generated by Polly has been optimized on a high level,
///   but it may be rather inefficient on the lower/scalar level. By scheduling
///   Polly before all other passes, we have the full sequence of -O3
///   optimizations behind us, such that inefficiencies on the low level can
///   be optimized away.
static llvm::RegisterStandardPasses
RegisterPollyOptimizer(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                       registerPollyEarlyAsPossiblePasses);
} // end of anonymous namespace.