#include "storm/modelchecker/rpatl/helper/internal/GameViHelper.h"

#include "storm/environment/Environment.h"
#include "storm/environment/solver/GameSolverEnvironment.h"
#include "storm/environment/solver/SolverEnvironment.h"
#include "storm/storage/Scheduler.h"
#include "storm/utility/SignalHandler.h"
#include "storm/utility/macros.h"
#include "storm/utility/vector.h"

namespace storm {
namespace modelchecker {
namespace helper {
namespace internal {

template<typename ValueType>
GameViHelper<ValueType>::GameViHelper(storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::BitVector statesOfCoalition)
    : _transitionMatrix(transitionMatrix), _statesOfCoalition(statesOfCoalition) {}

template<typename ValueType>
void GameViHelper<ValueType>::prepareSolversAndMultipliers(const Environment& env) {
    _multiplier = storm::solver::MultiplierFactory<ValueType>().create(env, _transitionMatrix);
    _x1IsCurrent = false;
}

template<typename ValueType>
void GameViHelper<ValueType>::performValueIteration(Environment const& env, std::vector<ValueType>& x, std::vector<ValueType> b,
                                                    storm::solver::OptimizationDirection const dir, std::vector<ValueType>& constrainedChoiceValues) {
    // Set per-state minimizer/maximizer flags based on coalition membership and direction.
    // statesOfCoalition has already been complemented in the model checker, so it marks the environment (non-coalition) states.
    // When maximizing: environment states minimize; when minimizing: environment states maximize.
    _minimizerStates = (dir == OptimizationDirection::Maximize) ? _statesOfCoalition : ~_statesOfCoalition;

    prepareSolversAndMultipliers(env);

    ValueType precision = storm::utility::convertNumber<ValueType>(env.solver().game().getPrecision());
    uint64_t maxIter = env.solver().game().getMaximalNumberOfIterations();
    _b = b;
    _x1 = x;
    _x2 = _x1;

    if (this->isProduceSchedulerSet()) {
        if (!this->_producedOptimalChoices.is_initialized()) {
            this->_producedOptimalChoices.emplace();
        }
        this->_producedOptimalChoices->resize(this->_transitionMatrix.getRowGroupCount());
    }

    uint64_t iter = 0;
    constrainedChoiceValues = std::vector<ValueType>(b.size(), storm::utility::zero<ValueType>());

    while (iter < maxIter) {
        performIterationStep(env, dir);
        if (checkConvergence(precision)) {
            _multiplier->multiply(env, xNew(), &_b, constrainedChoiceValues);
            break;
        }
        if (storm::utility::resources::isTerminate()) {
            break;
        }
        ++iter;
    }
    x = xNew();

    if (isProduceSchedulerSet()) {
        performIterationStep(env, dir, &_producedOptimalChoices.get());
    }
}

template<typename ValueType>
void GameViHelper<ValueType>::performIterationStep(Environment const& env, storm::solver::OptimizationDirection const dir, std::vector<uint64_t>* choices) {
    if (!_multiplier) {
        prepareSolversAndMultipliers(env);
    }
    _x1IsCurrent = !_x1IsCurrent;

    // Compute per-action values: Q(s,a) = sum_j P(s,a,j) * xOld(j) + b(s,a)
    std::vector<ValueType> choiceVals(_transitionMatrix.getRowCount(), storm::utility::zero<ValueType>());
    _multiplier->multiply(env, xOld(), &_b, choiceVals);

    // Reduce per-action to per-state using per-state optimization direction
    auto const& rowGroupIndices = _transitionMatrix.getRowGroupIndices();
    for (uint64_t state = 0; state + 1 < rowGroupIndices.size(); ++state) {
        uint64_t begin = rowGroupIndices[state];
        uint64_t end = rowGroupIndices[state + 1];
        auto itBegin = choiceVals.begin() + begin;
        auto itEnd = choiceVals.begin() + end;
        if (_minimizerStates[state]) {
            auto it = std::min_element(itBegin, itEnd);
            xNew()[state] = *it;
            if (choices != nullptr) {
                (*choices)[state] = static_cast<uint64_t>(it - itBegin);
            }
        } else {
            auto it = std::max_element(itBegin, itEnd);
            xNew()[state] = *it;
            if (choices != nullptr) {
                (*choices)[state] = static_cast<uint64_t>(it - itBegin);
            }
        }
    }
}

template<typename ValueType>
bool GameViHelper<ValueType>::checkConvergence(ValueType threshold) const {
    STORM_LOG_ASSERT(_multiplier, "tried to check for convergence without doing an iteration first.");
    STORM_LOG_ASSERT(threshold > storm::utility::zero<ValueType>(), "Did not expect a non-positive threshold.");
    auto x1It = xOld().begin();
    auto x1Ite = xOld().end();
    auto x2It = xNew().begin();
    ValueType maxDiff = (*x2It - *x1It);
    ValueType minDiff = maxDiff;
    for (++x1It, ++x2It; x1It != x1Ite; ++x1It, ++x2It) {
        ValueType diff = (*x2It - *x1It);
        bool skipCheck = false;
        if (maxDiff < diff) {
            maxDiff = diff;
        } else if (minDiff > diff) {
            minDiff = diff;
        } else {
            skipCheck = true;
        }
        if (!skipCheck && (maxDiff - minDiff) > threshold) {
            return false;
        }
    }
    return true;
}

template<typename ValueType>
void GameViHelper<ValueType>::setProduceScheduler(bool value) {
    _produceScheduler = value;
}

template<typename ValueType>
bool GameViHelper<ValueType>::isProduceSchedulerSet() const {
    return _produceScheduler;
}

template<typename ValueType>
void GameViHelper<ValueType>::updateTransitionMatrix(storm::storage::SparseMatrix<ValueType> newTransitionMatrix) {
    _transitionMatrix = newTransitionMatrix;
}

template<typename ValueType>
void GameViHelper<ValueType>::updateStatesOfCoalition(storm::storage::BitVector newStatesOfCoalition) {
    _statesOfCoalition = newStatesOfCoalition;
}

template<typename ValueType>
std::vector<uint64_t> const& GameViHelper<ValueType>::getProducedOptimalChoices() const {
    STORM_LOG_ASSERT(this->isProduceSchedulerSet(), "Trying to get the produced optimal choices although no scheduler was requested.");
    STORM_LOG_ASSERT(this->_producedOptimalChoices.is_initialized(), "Trying to get the produced optimal choices but none were available. Was there a computation call before?");
    return this->_producedOptimalChoices.get();
}

template<typename ValueType>
std::vector<uint64_t>& GameViHelper<ValueType>::getProducedOptimalChoices() {
    STORM_LOG_ASSERT(this->isProduceSchedulerSet(), "Trying to get the produced optimal choices although no scheduler was requested.");
    STORM_LOG_ASSERT(this->_producedOptimalChoices.is_initialized(), "Trying to get the produced optimal choices but none were available. Was there a computation call before?");
    return this->_producedOptimalChoices.get();
}

template<typename ValueType>
storm::storage::Scheduler<ValueType> GameViHelper<ValueType>::extractScheduler() const {
    auto const& optimalChoices = getProducedOptimalChoices();
    storm::storage::Scheduler<ValueType> scheduler(optimalChoices.size());
    for (uint64_t state = 0; state < optimalChoices.size(); ++state) {
        scheduler.setChoice(optimalChoices[state], state);
    }
    return scheduler;
}

template<typename ValueType>
void GameViHelper<ValueType>::getChoiceValues(Environment const& env, std::vector<ValueType> const& x, std::vector<ValueType>& choiceValues) {
    _multiplier->multiply(env, x, &_b, choiceValues);
}

template<typename ValueType>
void GameViHelper<ValueType>::fillChoiceValuesVector(std::vector<ValueType>& choiceValues, storm::storage::BitVector psiStates,
                                                     std::vector<storm::storage::SparseMatrix<double>::index_type> rowGroupIndices) {
    std::vector<ValueType> allChoices(rowGroupIndices.at(rowGroupIndices.size() - 1), storm::utility::zero<ValueType>());
    auto choice_it = choiceValues.begin();
    for (uint state = 0; state + 1 < rowGroupIndices.size(); ++state) {
        uint rowGroupSize = rowGroupIndices[state + 1] - rowGroupIndices[state];
        if (psiStates.get(state)) {
            for (uint choice = 0; choice < rowGroupSize; ++choice, ++choice_it) {
                allChoices.at(rowGroupIndices.at(state) + choice) = *choice_it;
            }
        }
    }
    choiceValues = allChoices;
}

template<typename ValueType>
std::vector<ValueType>& GameViHelper<ValueType>::xNew() {
    return _x1IsCurrent ? _x1 : _x2;
}

template<typename ValueType>
std::vector<ValueType> const& GameViHelper<ValueType>::xNew() const {
    return _x1IsCurrent ? _x1 : _x2;
}

template<typename ValueType>
std::vector<ValueType>& GameViHelper<ValueType>::xOld() {
    return _x1IsCurrent ? _x2 : _x1;
}

template<typename ValueType>
std::vector<ValueType> const& GameViHelper<ValueType>::xOld() const {
    return _x1IsCurrent ? _x2 : _x1;
}

template class GameViHelper<double>;
template class GameViHelper<storm::RationalNumber>;

}  // namespace internal
}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
