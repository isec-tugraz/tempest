#ifndef STORM_MODELCHECKER_SYMBOLICQUANTITATIVECHECKRESULT_H_
#define STORM_MODELCHECKER_SYMBOLICQUANTITATIVECHECKRESULT_H_

#include <set>

#include <boost/optional.hpp>

#include "storm/modelchecker/results/QuantitativeCheckResult.h"
#include "storm/storage/dd/Add.h"
#include "storm/storage/dd/DdType.h"
#include "storm/storage/expressions/Variable.h"

namespace storm {
namespace models {
namespace symbolic {
template<storm::dd::DdType Type, typename ValueType>
class NondeterministicModel;
}
}  // namespace models
namespace modelchecker {
// fwd
template<storm::dd::DdType Type>
class SymbolicQualitativeCheckResult;

template<storm::dd::DdType Type, typename ValueType = double>
class SymbolicQuantitativeCheckResult : public QuantitativeCheckResult<ValueType> {
   public:
    SymbolicQuantitativeCheckResult() = default;
    SymbolicQuantitativeCheckResult(storm::dd::Bdd<Type> const& reachableStates, storm::dd::Add<Type, ValueType> const& values);
    SymbolicQuantitativeCheckResult(storm::dd::Bdd<Type> const& reachableStates, storm::dd::Bdd<Type> const& states,
                                    storm::dd::Add<Type, ValueType> const& values);

    SymbolicQuantitativeCheckResult(SymbolicQuantitativeCheckResult const& other) = default;
    SymbolicQuantitativeCheckResult& operator=(SymbolicQuantitativeCheckResult const& other) = default;
    SymbolicQuantitativeCheckResult(SymbolicQuantitativeCheckResult&& other) = default;
    SymbolicQuantitativeCheckResult& operator=(SymbolicQuantitativeCheckResult&& other) = default;
    SymbolicQuantitativeCheckResult(SymbolicQualitativeCheckResult<Type> const& other);

    virtual std::unique_ptr<CheckResult> clone() const override;

    virtual std::unique_ptr<CheckResult> compareAgainstBound(storm::logic::ComparisonType comparisonType, ValueType const& bound) const override;

    virtual bool isSymbolic() const override;
    virtual bool isResultForAllStates() const override;

    virtual bool isSymbolicQuantitativeCheckResult() const override;

    storm::dd::Add<Type, ValueType> const& getValueVector() const;
    storm::dd::Bdd<Type> const& getStates() const;
    storm::dd::Bdd<Type> const& getReachableStates() const;
    bool hasChoiceValues() const;
    storm::dd::Add<Type, ValueType> const& getChoiceValues() const;
    storm::dd::Add<Type, ValueType>& getChoiceValues();
    void setChoiceValues(storm::dd::Add<Type, ValueType>&& choiceValues);
    bool hasShieldMask() const;
    storm::dd::Add<Type, ValueType> const& getShieldMask() const;
    storm::dd::Add<Type, ValueType>& getShieldMask();
    void setShieldMask(storm::dd::Add<Type, ValueType>&& shieldMask);
    storm::dd::Add<Type, ValueType> computeShieldMask(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                      storm::dd::Add<Type, ValueType> const& transitionMatrix, ValueType const& threshold) const;
    storm::dd::Add<Type, ValueType> computeShieldMask(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                      ValueType const& threshold) const;
    void computeAndSetShieldMask(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                 storm::dd::Add<Type, ValueType> const& transitionMatrix, ValueType const& threshold);
    void computeAndSetShieldMask(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model, ValueType const& threshold);
    static storm::dd::Add<Type, ValueType> constructShieldMaskAdd(storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model,
                                                                  storm::dd::Add<Type, ValueType> const& transitionMatrix,
                                                                  storm::dd::Add<Type, ValueType> const& choiceValues, ValueType const& threshold);

    virtual std::ostream& writeToStream(std::ostream& out) const override;

    virtual void filter(QualitativeCheckResult const& filter) override;

    virtual ValueType getMin() const override;
    virtual ValueType getMax() const override;

    virtual ValueType average() const override;
    virtual ValueType sum() const override;

    virtual void oneMinus() override;

   private:
    static std::set<storm::expressions::Variable> getActionLabelVariables(
        storm::models::symbolic::NondeterministicModel<Type, ValueType> const& model);

    bool hasValueType(std::type_info const& t) const override {
        return t == typeid(ValueType);
    }

    // The set of all reachable states.
    storm::dd::Bdd<Type> reachableStates;

    // The set of states for which this check result contains values.
    storm::dd::Bdd<Type> states;

    // The values of the quantitative check result.
    storm::dd::Add<Type, ValueType> values;

    // The values for each state and choice pair.
    boost::optional<storm::dd::Add<Type, ValueType>> choiceValues;

    // The state-to-action-bitmask ADD derived from the choice values.
    boost::optional<storm::dd::Add<Type, ValueType>> shieldMask;
};
}  // namespace modelchecker
}  // namespace storm

#endif /* STORM_MODELCHECKER_SYMBOLICQUANTITATIVECHECKRESULT_H_ */
