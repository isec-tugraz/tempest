#pragma once

#include <boost/optional.hpp>

#include "storm/solver/multiplier/Multiplier.h"
#include "storm/storage/BitVector.h"
#include "storm/storage/MaximalEndComponentDecomposition.h"
#include "storm/storage/SparseMatrix.h"

namespace storm {
class Environment;

namespace storage {
template<typename VT>
class Scheduler;
}

namespace modelchecker {
namespace helper {
namespace internal {

template<typename ValueType>
class SoundGameViHelper {
   public:
    SoundGameViHelper(storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions,
                      std::vector<ValueType> b, storm::storage::BitVector statesOfCoalition, storm::storage::BitVector psiStates,
                      OptimizationDirection const& optimizationDirection);

    void prepareSolversAndMultipliers(const Environment& env);

    void performValueIteration(Environment const& env, std::vector<ValueType>& xL, std::vector<ValueType>& xU, storm::solver::OptimizationDirection const dir,
                               std::vector<ValueType>& constrainedChoiceValues);

    void setProduceScheduler(bool value);
    bool isProduceSchedulerSet() const;

    void updateTransitionMatrix(storm::storage::SparseMatrix<ValueType> newTransitionMatrix);
    void updateStatesOfCoalition(storm::storage::BitVector newStatesOfCoalition);

    storm::storage::Scheduler<ValueType> extractScheduler() const;

    void getChoiceValues(Environment const& env, std::vector<ValueType> const& x, std::vector<ValueType>& choiceValues);

    void fillChoiceValuesVector(std::vector<ValueType>& choiceValues, storm::storage::BitVector psiStates,
                                std::vector<storm::storage::SparseMatrix<double>::index_type> rowGroupIndices);

    void deflate(storm::storage::MaximalEndComponentDecomposition<ValueType> const MECD, storage::SparseMatrix<ValueType> const restrictedMatrix,
                 std::vector<ValueType>& xU, std::vector<ValueType> choiceValues);

    void reduceChoiceValues(std::vector<ValueType>& choiceValues, storm::storage::BitVector* result, std::vector<ValueType>& x);

   private:
    void performIterationStep(Environment const& env, storm::solver::OptimizationDirection const dir, std::vector<uint64_t>* choices = nullptr);

    bool checkConvergence(ValueType precision) const;

    std::vector<ValueType>& xNewL();
    std::vector<ValueType> const& xNewL() const;
    std::vector<ValueType>& xOldL();
    std::vector<ValueType> const& xOldL() const;

    std::vector<ValueType>& xNewU();
    std::vector<ValueType> const& xNewU() const;
    std::vector<ValueType>& xOldU();
    std::vector<ValueType> const& xOldU() const;

    bool _x1IsCurrent;

    storm::storage::BitVector _minimizerStates;

    std::vector<uint64_t> const& getProducedOptimalChoices() const;
    std::vector<uint64_t>& getProducedOptimalChoices();

    std::unique_ptr<storm::solver::Multiplier<ValueType>> _multiplier;

    storm::storage::SparseMatrix<ValueType> _transitionMatrix;
    storm::storage::SparseMatrix<ValueType> _backwardTransitions;
    storm::storage::SparseMatrix<ValueType> _restrictedTransitions;
    storm::storage::BitVector _oldPolicy;
    storm::storage::BitVector _statesOfCoalition;
    storm::storage::BitVector _psiStates;
    std::vector<ValueType> _x, _x1L, _x2L, _x1U, _x2U, _b;
    OptimizationDirection _optimizationDirection;

    storm::storage::MaximalEndComponentDecomposition<ValueType> _MSECs;

    bool _produceScheduler = false;
    boost::optional<std::vector<uint64_t>> _producedOptimalChoices;

    std::vector<double> _timing;
};

}  // namespace internal
}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
