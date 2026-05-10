#pragma once

#include <memory>
#include <vector>

#include "storm/storage/BitVector.h"
#include "storm/storage/Scheduler.h"

namespace storm {
namespace modelchecker {
namespace helper {

template<typename ValueType>
struct SMGSparseModelCheckingHelperReturnType {
    SMGSparseModelCheckingHelperReturnType(SMGSparseModelCheckingHelperReturnType const&) = delete;
    SMGSparseModelCheckingHelperReturnType(SMGSparseModelCheckingHelperReturnType&&) = default;

    SMGSparseModelCheckingHelperReturnType(std::vector<ValueType>&& values, storm::storage::BitVector&& relevantStates,
                                           std::unique_ptr<storm::storage::Scheduler<ValueType>>&& scheduler,
                                           std::vector<ValueType>&& choiceValues)
        : values(std::move(values)),
          relevantStates(std::move(relevantStates)),
          scheduler(std::move(scheduler)),
          choiceValues(std::move(choiceValues)) {}

    virtual ~SMGSparseModelCheckingHelperReturnType() = default;

    std::vector<ValueType> values;
    storm::storage::BitVector relevantStates;
    std::unique_ptr<storm::storage::Scheduler<ValueType>> scheduler;
    std::vector<ValueType> choiceValues;
};

}  // namespace helper
}  // namespace modelchecker
}  // namespace storm
