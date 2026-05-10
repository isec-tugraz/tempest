#pragma once

#include <vector>

#include "storm/modelchecker/hints/ModelCheckerHint.h"
#include "storm/modelchecker/rpatl/helper/SMGModelCheckingHelperReturnType.h"
#include "storm/solver/SolveGoal.h"
#include "storm/storage/BitVector.h"
#include "storm/storage/SparseMatrix.h"

namespace storm {

class Environment;

namespace models {
namespace sparse {
template<typename ValueType>
class StandardRewardModel;
}
}

namespace modelchecker {
class CheckResult;

namespace helper {

template<typename ValueType>
class SparseSmgRpatlHelper {
   public:
    static SMGSparseModelCheckingHelperReturnType<ValueType> computeUntilProbabilities(
        Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
        storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
        storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
        ModelCheckerHint const& hint = ModelCheckerHint());

    static SMGSparseModelCheckingHelperReturnType<ValueType> computeUntilProbabilitiesSound(
        Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
        storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
        storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
        ModelCheckerHint const& hint);

    static SMGSparseModelCheckingHelperReturnType<ValueType> computeGloballyProbabilities(
        Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
        storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& psiStates, bool qualitative,
        storm::storage::BitVector statesOfCoalition, bool produceScheduler, ModelCheckerHint const& hint = ModelCheckerHint());

    static SMGSparseModelCheckingHelperReturnType<ValueType> computeNextProbabilities(
        Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
        storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& psiStates, bool qualitative,
        storm::storage::BitVector statesOfCoalition, bool produceScheduler, ModelCheckerHint const& hint);

    static SMGSparseModelCheckingHelperReturnType<ValueType> computeBoundedUntilProbabilities(
        Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
        storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
        storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
        ModelCheckerHint const& hint, uint64_t lowerBound, uint64_t upperBound);

   private:
    static storm::storage::Scheduler<ValueType> expandScheduler(storm::storage::Scheduler<ValueType> scheduler, storm::storage::BitVector psiStates,
                                                                 storm::storage::BitVector notPhiStates, bool sound = false);
};

}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
