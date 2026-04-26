#pragma once

#include <boost/optional.hpp>
#include <string>

#include "storm/environment/Environment.h"
#include "storm/environment/SubEnvironment.h"
#include "storm/modelchecker/helper/infinitehorizon/SteadyStateDistributionAlgorithm.h"
#include "storm/settings/modules/ConditionalSettings.h"

namespace storm {

// Forward declare subenvironments
class ConditionalModelCheckerEnvironment;
class MultiObjectiveModelCheckerEnvironment;

class ModelCheckerEnvironment {
   public:
    ModelCheckerEnvironment();
    ~ModelCheckerEnvironment();

    ConditionalModelCheckerEnvironment& conditional();
    ConditionalModelCheckerEnvironment const& conditional() const;

    MultiObjectiveModelCheckerEnvironment& multi();
    MultiObjectiveModelCheckerEnvironment const& multi() const;

    SteadyStateDistributionAlgorithm getSteadyStateDistributionAlgorithm() const;
    void setSteadyStateDistributionAlgorithm(SteadyStateDistributionAlgorithm value);

    bool isLtl2daToolSet() const;
    std::string const& getLtl2daTool() const;
    void setLtl2daTool(std::string const& value);
    void unsetLtl2daTool();

    void setProduceChoiceValues(bool const& value);
    bool getProduceChoiceValues() const;

   private:
    SubEnvironment<ConditionalModelCheckerEnvironment> conditionalModelCheckerEnvironment;
    SubEnvironment<MultiObjectiveModelCheckerEnvironment> multiObjectiveModelCheckerEnvironment;
    boost::optional<std::string> ltl2daTool;
    SteadyStateDistributionAlgorithm steadyStateDistributionAlgorithm;

    bool produceChoiceValues = false;
    ConditionalAlgorithmSetting conditionalAlgorithmSetting;
};
}  // namespace storm
