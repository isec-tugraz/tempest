#include "storm/modelchecker/rpatl/helper/SparseSmgRpatlHelper.h"

#include "storm/environment/Environment.h"
#include "storm/environment/solver/GameSolverEnvironment.h"
#include "storm/environment/solver/MinMaxSolverEnvironment.h"
#include "storm/modelchecker/rpatl/helper/internal/GameViHelper.h"
#include "storm/modelchecker/rpatl/helper/internal/SoundGameViHelper.h"
#include "storm/solver/MinMaxLinearEquationSolver.h"
#include "storm/solver/multiplier/Multiplier.h"
#include "storm/utility/graph.h"
#include "storm/utility/vector.h"

namespace storm {
namespace modelchecker {
namespace helper {

template<typename ValueType>
SMGSparseModelCheckingHelperReturnType<ValueType> SparseSmgRpatlHelper<ValueType>::computeUntilProbabilities(
    Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
    storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
    storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
    ModelCheckerHint const& hint) {
    auto solverEnv = env;
    solverEnv.solver().minMax().setMethod(storm::solver::MinMaxMethod::ValueIteration, false);

    storm::storage::BitVector relevantStates = phiStates & ~psiStates;
    std::vector<ValueType> x(relevantStates.getNumberOfSetBits(), storm::utility::zero<ValueType>());
    std::vector<ValueType> result(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    std::vector<ValueType> b = transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, psiStates);
    std::vector<ValueType> constrainedChoiceValues(b.size(), storm::utility::zero<ValueType>());
    std::unique_ptr<storm::storage::Scheduler<ValueType>> scheduler;

    storm::storage::BitVector clippedStatesOfCoalition(relevantStates.getNumberOfSetBits());
    clippedStatesOfCoalition.setClippedStatesOfCoalition(relevantStates, statesOfCoalition);

    if (!relevantStates.empty()) {
        storm::storage::SparseMatrix<ValueType> submatrix = transitionMatrix.getSubmatrix(true, relevantStates, relevantStates, false);
        storm::modelchecker::helper::internal::GameViHelper<ValueType> viHelper(submatrix, clippedStatesOfCoalition);
        if (produceScheduler) {
            viHelper.setProduceScheduler(true);
        }
        viHelper.performValueIteration(solverEnv, x, b, goal.direction(), constrainedChoiceValues);
        viHelper.fillChoiceValuesVector(constrainedChoiceValues, relevantStates, transitionMatrix.getRowGroupIndices());

        if (produceScheduler) {
            scheduler = std::make_unique<storm::storage::Scheduler<ValueType>>(expandScheduler(viHelper.extractScheduler(), psiStates, ~phiStates));
        }
    }

    storm::utility::vector::setVectorValues(result, relevantStates, x);
    storm::utility::vector::setVectorValues(result, psiStates, storm::utility::one<ValueType>());
    return SMGSparseModelCheckingHelperReturnType<ValueType>(std::move(result), std::move(relevantStates), std::move(scheduler),
                                                             std::move(constrainedChoiceValues));
}

template<typename ValueType>
SMGSparseModelCheckingHelperReturnType<ValueType> SparseSmgRpatlHelper<ValueType>::computeUntilProbabilitiesSound(
    Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
    storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
    storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
    ModelCheckerHint const& hint) {
    storm::storage::BitVector probGreater0 = storm::utility::graph::performProbGreater0(backwardTransitions, phiStates, psiStates);
    std::unique_ptr<storm::storage::Scheduler<ValueType>> scheduler;
    storm::storage::BitVector relevantStates = phiStates;

    // xL initialized to 1 for psiStates, 0 elsewhere (clipped to relevantStates)
    std::vector<ValueType> xL(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    for (uint64_t s = 0; s < xL.size(); ++s) {
        if (psiStates[s])
            xL[s] = storm::utility::one<ValueType>();
    }
    size_t i = 0;
    xL.erase(std::remove_if(xL.begin(), xL.end(), [&relevantStates, &i](const ValueType&) { return !relevantStates[i++]; }), xL.end());
    xL.resize(relevantStates.getNumberOfSetBits());

    // xU initialized to 1 for states with prob > 0, 0 elsewhere (clipped to relevantStates)
    std::vector<ValueType> xU(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    for (uint64_t s = 0; s < xU.size(); ++s) {
        if (probGreater0[s])
            xU[s] = storm::utility::one<ValueType>();
    }
    i = 0;
    xU.erase(std::remove_if(xU.begin(), xU.end(), [&relevantStates, &i](const ValueType&) { return !relevantStates[i++]; }), xU.end());
    xU.resize(relevantStates.getNumberOfSetBits());

    storm::storage::BitVector clippedPsiStates(relevantStates.getNumberOfSetBits());
    clippedPsiStates.setClippedStatesOfCoalition(relevantStates, psiStates);

    std::vector<ValueType> b = transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, psiStates);
    std::vector<ValueType> result(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());

    storm::storage::BitVector clippedStatesOfCoalition(relevantStates.getNumberOfSetBits());
    clippedStatesOfCoalition.setClippedStatesOfCoalition(relevantStates, statesOfCoalition);
    std::vector<ValueType> constrainedChoiceValues(b.size(), storm::utility::zero<ValueType>());

    if (!relevantStates.empty()) {
        storm::storage::SparseMatrix<ValueType> submatrix = transitionMatrix.getSubmatrix(true, relevantStates, relevantStates, false);
        storm::modelchecker::helper::internal::SoundGameViHelper<ValueType> viHelper(submatrix, submatrix.transpose(), b, clippedStatesOfCoalition,
                                                                                     clippedPsiStates, goal.direction());
        if (produceScheduler) {
            viHelper.setProduceScheduler(true);
        }
        viHelper.performValueIteration(env, xL, xU, goal.direction(), constrainedChoiceValues);
        viHelper.fillChoiceValuesVector(constrainedChoiceValues, relevantStates, transitionMatrix.getRowGroupIndices());
        storm::utility::vector::setVectorValues(result, relevantStates, xL);

        if (produceScheduler) {
            scheduler = std::make_unique<storm::storage::Scheduler<ValueType>>(expandScheduler(viHelper.extractScheduler(), psiStates, ~phiStates, true));
        }
    }

    return SMGSparseModelCheckingHelperReturnType<ValueType>(std::move(result), std::move(relevantStates), std::move(scheduler),
                                                             std::move(constrainedChoiceValues));
}

template<typename ValueType>
storm::storage::Scheduler<ValueType> SparseSmgRpatlHelper<ValueType>::expandScheduler(storm::storage::Scheduler<ValueType> scheduler,
                                                                                       storm::storage::BitVector psiStates,
                                                                                       storm::storage::BitVector notPhiStates, bool sound) {
    storm::storage::Scheduler<ValueType> completeScheduler(psiStates.size());
    uint_fast64_t maybeStatesCounter = 0;
    uint schedulerSize = psiStates.size();
    for (uint stateCounter = 0; stateCounter < schedulerSize; ++stateCounter) {
        if (psiStates.get(stateCounter)) {
            completeScheduler.setChoice(0, stateCounter);
            if (sound) {
                ++maybeStatesCounter;
            }
        } else if (notPhiStates.get(stateCounter)) {
            completeScheduler.setChoice(0, stateCounter);
        } else {
            completeScheduler.setChoice(scheduler.getChoice(maybeStatesCounter), stateCounter);
            ++maybeStatesCounter;
        }
    }
    return completeScheduler;
}

template<typename ValueType>
SMGSparseModelCheckingHelperReturnType<ValueType> SparseSmgRpatlHelper<ValueType>::computeGloballyProbabilities(
    Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
    storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& psiStates, bool qualitative,
    storm::storage::BitVector statesOfCoalition, bool produceScheduler, ModelCheckerHint const& hint) {
    // G psi = not(F(not psi)) = not(true U (not psi))
    storm::storage::BitVector notPsiStates = ~psiStates;
    statesOfCoalition.complement();

    if (env.solver().isForceSoundness()) {
        auto result = computeUntilProbabilitiesSound(env, std::move(goal), transitionMatrix, backwardTransitions,
                                                     storm::storage::BitVector(transitionMatrix.getRowGroupCount(), true), notPsiStates, qualitative,
                                                     statesOfCoalition, produceScheduler, hint);
        for (auto& element : result.values)
            element = storm::utility::one<ValueType>() - element;
        for (auto& element : result.choiceValues)
            element = storm::utility::one<ValueType>() - element;
        return result;
    }

    auto result = computeUntilProbabilities(env, std::move(goal), transitionMatrix, backwardTransitions,
                                            storm::storage::BitVector(transitionMatrix.getRowGroupCount(), true), notPsiStates, qualitative,
                                            statesOfCoalition, produceScheduler, hint);
    for (auto& element : result.values)
        element = storm::utility::one<ValueType>() - element;
    for (auto& element : result.choiceValues)
        element = storm::utility::one<ValueType>() - element;
    return result;
}

template<typename ValueType>
SMGSparseModelCheckingHelperReturnType<ValueType> SparseSmgRpatlHelper<ValueType>::computeNextProbabilities(
    Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
    storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& psiStates, bool qualitative,
    storm::storage::BitVector statesOfCoalition, bool produceScheduler, ModelCheckerHint const& hint) {
    std::vector<ValueType> result(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    storm::storage::BitVector allStates(transitionMatrix.getRowGroupCount(), true);
    std::vector<ValueType> b = transitionMatrix.getConstrainedRowGroupSumVector(allStates, psiStates);
    std::vector<ValueType> choiceValues(transitionMatrix.getRowCount(), storm::utility::zero<ValueType>());
    statesOfCoalition.complement();

    if (produceScheduler) {
        STORM_LOG_WARN("Next formula does not expect that produceScheduler is set to true.");
    }

    // Use a zero x-vector so multiplyAndReduce just reduces b over row groups
    std::vector<ValueType> zeros(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    auto multiplier = storm::solver::MultiplierFactory<ValueType>().create(env, transitionMatrix);
    multiplier->multiplyAndReduce(env, goal.direction(), zeros, &b, result);
    choiceValues = b;

    return SMGSparseModelCheckingHelperReturnType<ValueType>(std::move(result), std::move(allStates), nullptr, std::move(choiceValues));
}

template<typename ValueType>
SMGSparseModelCheckingHelperReturnType<ValueType> SparseSmgRpatlHelper<ValueType>::computeBoundedUntilProbabilities(
    Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix,
    storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates,
    storm::storage::BitVector const& psiStates, bool qualitative, storm::storage::BitVector statesOfCoalition, bool produceScheduler,
    ModelCheckerHint const& hint, uint64_t lowerBound, uint64_t upperBound) {
    auto solverEnv = env;
    solverEnv.solver().minMax().setMethod(storm::solver::MinMaxMethod::ValueIteration, false);

    storm::storage::BitVector relevantStates = phiStates & ~psiStates;
    std::vector<ValueType> x(relevantStates.getNumberOfSetBits(), storm::utility::zero<ValueType>());
    std::vector<ValueType> b = transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, psiStates);
    std::vector<ValueType> result(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
    std::vector<ValueType> constrainedChoiceValues(transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, psiStates).size(),
                                                   storm::utility::zero<ValueType>());
    std::unique_ptr<storm::storage::Scheduler<ValueType>> scheduler;

    storm::storage::BitVector clippedStatesOfCoalition(relevantStates.getNumberOfSetBits());
    clippedStatesOfCoalition.setClippedStatesOfCoalition(relevantStates, statesOfCoalition);

    if (!relevantStates.empty() && upperBound > 0) {
        storm::storage::SparseMatrix<ValueType> submatrix = transitionMatrix.getSubmatrix(true, relevantStates, relevantStates, false);
        storm::modelchecker::helper::internal::GameViHelper<ValueType> viHelper(submatrix, clippedStatesOfCoalition);
        if (produceScheduler) {
            viHelper.setProduceScheduler(true);
        }
        if (lowerBound == 0) {
            solverEnv.solver().game().setMaximalNumberOfIterations(upperBound);
            viHelper.performValueIteration(solverEnv, x, b, goal.direction(), constrainedChoiceValues);
        } else {
            solverEnv.solver().game().setMaximalNumberOfIterations(upperBound - lowerBound);
            viHelper.performValueIteration(solverEnv, x, b, goal.direction(), constrainedChoiceValues);

            std::vector<ValueType> subResult(transitionMatrix.getRowGroupCount(), storm::utility::zero<ValueType>());
            storm::utility::vector::setVectorValues(subResult, relevantStates, x);
            storm::utility::vector::setVectorValues(subResult, psiStates, storm::utility::one<ValueType>());

            storm::storage::BitVector newPsiStates(subResult.size(), false);
            storm::utility::vector::setNonzeroIndices(subResult, newPsiStates);

            relevantStates = storm::storage::BitVector(phiStates.size(), true);
            submatrix = transitionMatrix.getSubmatrix(true, relevantStates, relevantStates, false);
            viHelper.updateTransitionMatrix(submatrix);
            viHelper.updateStatesOfCoalition(statesOfCoalition);

            constrainedChoiceValues =
                std::vector<ValueType>(transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, newPsiStates).size(), storm::utility::zero<ValueType>());
            b = std::vector<ValueType>(transitionMatrix.getConstrainedRowGroupSumVector(relevantStates, newPsiStates).size(), storm::utility::zero<ValueType>());

            solverEnv.solver().game().setMaximalNumberOfIterations(lowerBound);
            viHelper.performValueIteration(solverEnv, subResult, b, goal.direction(), constrainedChoiceValues);
            x = subResult;
        }
        viHelper.fillChoiceValuesVector(constrainedChoiceValues, relevantStates, transitionMatrix.getRowGroupIndices());
        if (produceScheduler) {
            scheduler = std::make_unique<storm::storage::Scheduler<ValueType>>(expandScheduler(viHelper.extractScheduler(), relevantStates, ~relevantStates));
        }
        storm::utility::vector::setVectorValues(result, relevantStates, x);
    }

    storm::utility::vector::setVectorValues(result, psiStates, storm::utility::one<ValueType>());
    return SMGSparseModelCheckingHelperReturnType<ValueType>(std::move(result), std::move(relevantStates), std::move(scheduler),
                                                             std::move(constrainedChoiceValues));
}

template class SparseSmgRpatlHelper<double>;
template class SparseSmgRpatlHelper<storm::RationalNumber>;

}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
