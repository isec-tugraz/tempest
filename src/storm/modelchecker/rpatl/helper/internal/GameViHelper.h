#pragma once

#include <boost/optional.hpp>

#include "storm/solver/multiplier/Multiplier.h"
#include "storm/storage/BitVector.h"
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
class GameViHelper {
   public:
    GameViHelper(storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::BitVector statesOfCoalition);

    void prepareSolversAndMultipliers(const Environment& env);

    void performValueIteration(Environment const& env, std::vector<ValueType>& x, std::vector<ValueType> b, storm::solver::OptimizationDirection const dir,
                               std::vector<ValueType>& constrainedChoiceValues);

    void setProduceScheduler(bool value);
    bool isProduceSchedulerSet() const;

    void updateTransitionMatrix(storm::storage::SparseMatrix<ValueType> newTransitionMatrix);
    void updateStatesOfCoalition(storm::storage::BitVector newStatesOfCoalition);

    storm::storage::Scheduler<ValueType> extractScheduler() const;

    void getChoiceValues(Environment const& env, std::vector<ValueType> const& x, std::vector<ValueType>& choiceValues);

    void fillChoiceValuesVector(std::vector<ValueType>& choiceValues, storm::storage::BitVector psiStates,
                                std::vector<storm::storage::SparseMatrix<double>::index_type> rowGroupIndices);

   private:
    void performIterationStep(Environment const& env, storm::solver::OptimizationDirection const dir, std::vector<uint64_t>* choices = nullptr);

    bool checkConvergence(ValueType precision) const;

    std::vector<ValueType>& xNew();
    std::vector<ValueType> const& xNew() const;
    std::vector<ValueType>& xOld();
    std::vector<ValueType> const& xOld() const;

    bool _x1IsCurrent;

    std::vector<uint64_t> const& getProducedOptimalChoices() const;
    std::vector<uint64_t>& getProducedOptimalChoices();

    storm::storage::SparseMatrix<ValueType> _transitionMatrix;
    storm::storage::BitVector _statesOfCoalition;
    storm::storage::BitVector _minimizerStates;
    std::vector<ValueType> _x, _x1, _x2, _b;
    std::unique_ptr<storm::solver::Multiplier<ValueType>> _multiplier;

    bool _produceScheduler = false;
    boost::optional<std::vector<uint64_t>> _producedOptimalChoices;
};

}  // namespace internal
}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
