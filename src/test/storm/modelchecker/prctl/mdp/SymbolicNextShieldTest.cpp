#include "storm-config.h"
#include "test/storm_gtest.h"

#include "storm-parsers/api/properties.h"
#include "storm-parsers/parser/PrismParser.h"
#include "storm/api/builder.h"
#include "storm/api/properties.h"
#include "storm/environment/Environment.h"
#include "storm/environment/modelchecker/ModelCheckerEnvironment.h"
#include "storm/logic/Formulas.h"
#include "storm/modelchecker/CheckTask.h"
#include "storm/modelchecker/prctl/SymbolicMdpPrctlModelChecker.h"
#include "storm/modelchecker/results/SymbolicQuantitativeCheckResult.h"
#include "storm/models/symbolic/Mdp.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr storm::dd::DdType DdType = storm::dd::DdType::Sylvan;
using Model = storm::models::symbolic::Mdp<DdType, double>;
using Result = storm::modelchecker::SymbolicQuantitativeCheckResult<DdType, double>;

struct ShieldFixtureData {
    std::shared_ptr<Model> model;
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas;
    storm::expressions::Variable stateVariable;
    storm::expressions::Variable actionA;
    storm::expressions::Variable actionB;
    uint64_t actionABit = 0;
    uint64_t actionBBit = 0;
};

ShieldFixtureData buildShieldFixtureData(std::string const& formulaString = "") {
    storm::prism::Program program = storm::parser::PrismParser::parse(STORM_TEST_RESOURCES_DIR "/mdp/symbolic_shield.nm");
    ShieldFixtureData result;
    if (!formulaString.empty()) {
        result.formulas = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram(formulaString, program));
    }
    result.model = storm::api::buildSymbolicModel<DdType, double>(program, result.formulas)->template as<Model>();

    bool foundStateVariable = false;
    for (auto const& variable : result.model->getRowVariables()) {
        if (variable.getName() == "s") {
            result.stateVariable = variable;
            foundStateVariable = true;
        }
    }
    if (!foundStateVariable) {
        throw std::runtime_error("Expected the symbolic model to contain row variable s.");
    }

    uint64_t bitValue = 1;
    for (auto const& variable : result.model->getNondeterminismVariables()) {
        if (variable.getName().find("nondet") == 0) {
            continue;
        }
        if (variable.getName() == "a") {
            result.actionA = variable;
            result.actionABit = bitValue;
        } else if (variable.getName() == "b") {
            result.actionB = variable;
            result.actionBBit = bitValue;
        }
        bitValue *= 2;
    }
    if (result.actionABit == 0 || result.actionBBit == 0) {
        throw std::runtime_error("Expected the symbolic model to contain action labels a and b.");
    }

    return result;
}

Result& checkSymbolicMdp(std::shared_ptr<Model> const& model, storm::logic::Formula const& formula, std::unique_ptr<storm::modelchecker::CheckResult>& result) {
    storm::Environment env;
    env.modelchecker().setProduceChoiceValues(true);

    storm::modelchecker::SymbolicMdpPrctlModelChecker<Model> checker(*model);
    storm::modelchecker::CheckTask<storm::logic::Formula, double> task(formula);
    result = checker.check(env, task);
    return result->asSymbolicQuantitativeCheckResult<DdType, double>();
}

void expectStateValue(storm::dd::Add<DdType, double> const& add, storm::expressions::Variable const& stateVariable, int_fast64_t state, uint64_t expectedValue,
                      std::string const& what) {
    EXPECT_EQ(static_cast<double>(expectedValue), add.getValue({{stateVariable, state}})) << what << " at state " << state;
}

void addChoiceValueSupport(storm::dd::Add<DdType, double>& choiceValues, Model const& model) {
    choiceValues.addMetaVariables(model.getNondeterminismVariables());
}

}  // namespace

TEST(SymbolicShield, ComputesMaxMaskAndFallbackAdds) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData();

    fixture.model->getManager().execute([&]() {
        auto const& manager = fixture.model->getManager();
        storm::dd::Add<DdType, double> choiceValues = manager.template getAddZero<double>();

        choiceValues += (manager.getEncoding(fixture.stateVariable, 0) && manager.getEncoding(fixture.actionA, 1)).template toAdd<double>() *
                        manager.getConstant(1.0);

        choiceValues += (manager.getEncoding(fixture.stateVariable, 1) && manager.getEncoding(fixture.actionA, 1)).template toAdd<double>() *
                        manager.getConstant(1.0);

        addChoiceValueSupport(choiceValues, *fixture.model);

        auto const& transitionMatrix = fixture.model->getTransitionMatrix();
        storm::dd::Add<DdType, double> shieldMask =
            Result::computeMaskAdd(*fixture.model, transitionMatrix, choiceValues, 0.5, storm::OptimizationDirection::Maximize);
        storm::dd::Add<DdType, double> fallback =
            Result::computeFallbackAdd(*fixture.model, transitionMatrix, choiceValues, 1.1, storm::OptimizationDirection::Maximize);

        // test state 0
        expectStateValue(shieldMask, fixture.stateVariable, 0, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionABit, "fallback");

        // test state 1
        expectStateValue(shieldMask, fixture.stateVariable, 1, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 1, fixture.actionABit, "fallback");

        // test state 2
        expectStateValue(shieldMask, fixture.stateVariable, 2, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 2, fixture.actionBBit, "fallback");
    });
}

TEST(SymbolicShield, FallbackContainsAllTiedOptimalChoices) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData();

    fixture.model->getManager().execute([&]() {
        auto const& manager = fixture.model->getManager();
        storm::dd::Add<DdType, double> choiceValues = manager.template getAddZero<double>();
        choiceValues +=
            (manager.getEncoding(fixture.stateVariable, 0) && manager.getEncoding(fixture.actionA, 1) && manager.getEncoding(fixture.actionB, 0))
                .template toAdd<double>() *
            manager.getConstant(1.0);
        choiceValues +=
            (manager.getEncoding(fixture.stateVariable, 0) && manager.getEncoding(fixture.actionA, 0) && manager.getEncoding(fixture.actionB, 1))
                .template toAdd<double>() *
            manager.getConstant(1.0);
        addChoiceValueSupport(choiceValues, *fixture.model);

        storm::dd::Add<DdType, double> fallback = Result::computeFallbackAdd(
            *fixture.model, fixture.model->getTransitionMatrix(), choiceValues, 1.1, storm::OptimizationDirection::Maximize);

        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionABit | fixture.actionBBit, "fallback");
    });
}

TEST(SymbolicShield, ComputesMinMaskAndMasksInvalidFallbackChoices) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData("Pmin=? [ X \"goal\" ]");

    fixture.model->getManager().execute([&]() {
        std::unique_ptr<storm::modelchecker::CheckResult> rawResult;
        Result& result = checkSymbolicMdp(fixture.model, *fixture.formulas.front(), rawResult);
        ASSERT_TRUE(result.hasChoiceValues());

        auto const& transitionMatrix = fixture.model->getTransitionMatrix();
        storm::dd::Add<DdType, double> shieldMask =
            Result::computeMaskAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 0.5, storm::OptimizationDirection::Minimize);
        storm::dd::Add<DdType, double> fallback =
            Result::computeFallbackAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), -0.1, storm::OptimizationDirection::Minimize);

        expectStateValue(shieldMask, fixture.stateVariable, 0, fixture.actionBBit, "mask");
        expectStateValue(shieldMask, fixture.stateVariable, 1, 0, "mask");
        expectStateValue(shieldMask, fixture.stateVariable, 2, fixture.actionBBit, "mask");

        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionBBit, "fallback");
        expectStateValue(fallback, fixture.stateVariable, 1, fixture.actionABit, "fallback");
        expectStateValue(fallback, fixture.stateVariable, 2, fixture.actionBBit, "fallback");
    });
}

TEST(SymbolicShield, NextProducesChoiceValuesForShielding) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData("Pmax=? [ X \"goal\" ]");

    fixture.model->getManager().execute([&]() {
        std::unique_ptr<storm::modelchecker::CheckResult> rawResult;
        Result& result = checkSymbolicMdp(fixture.model, *fixture.formulas.front(), rawResult);
        ASSERT_TRUE(result.hasChoiceValues());

        auto const& transitionMatrix = fixture.model->getTransitionMatrix();
        storm::dd::Add<DdType, double> shieldMask =
            Result::computeMaskAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 0.5, storm::OptimizationDirection::Maximize);
        storm::dd::Add<DdType, double> fallback =
            Result::computeFallbackAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 1.1, storm::OptimizationDirection::Maximize);

        expectStateValue(shieldMask, fixture.stateVariable, 0, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionABit, "fallback");

        expectStateValue(shieldMask, fixture.stateVariable, 1, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 1, fixture.actionABit, "fallback");

        expectStateValue(shieldMask, fixture.stateVariable, 2, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 2, fixture.actionBBit, "fallback");
    });
}

TEST(SymbolicShield, UntilProducesChoiceValuesForShielding) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData("Pmax=? [ true U \"goal\" ]");

    fixture.model->getManager().execute([&]() {
        std::unique_ptr<storm::modelchecker::CheckResult> rawResult;
        Result& result = checkSymbolicMdp(fixture.model, *fixture.formulas.front(), rawResult);
        ASSERT_TRUE(result.hasChoiceValues());

        auto const& transitionMatrix = fixture.model->getTransitionMatrix();
        storm::dd::Add<DdType, double> shieldMask =
            Result::computeMaskAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 0.5, storm::OptimizationDirection::Maximize);
        storm::dd::Add<DdType, double> fallback =
            Result::computeFallbackAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 1.1, storm::OptimizationDirection::Maximize);

        expectStateValue(shieldMask, fixture.stateVariable, 0, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionABit, "fallback");

        // The goal state is terminal in a model built for an until formula and has no choices.
        expectStateValue(shieldMask, fixture.stateVariable, 1, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 1, 0, "fallback");

        expectStateValue(shieldMask, fixture.stateVariable, 2, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 2, fixture.actionBBit, "fallback");
    });
}

TEST(SymbolicShield, BoundedUntilProducesChoiceValuesForShielding) {
#ifndef STORM_HAVE_SYLVAN
    GTEST_SKIP() << "Library Sylvan not available.";
#endif

    ShieldFixtureData fixture = buildShieldFixtureData("Pmax=? [ true U<=1 \"goal\" ]");

    fixture.model->getManager().execute([&]() {
        std::unique_ptr<storm::modelchecker::CheckResult> rawResult;
        Result& result = checkSymbolicMdp(fixture.model, *fixture.formulas.front(), rawResult);
        ASSERT_TRUE(result.hasChoiceValues());

        auto const& transitionMatrix = fixture.model->getTransitionMatrix();
        storm::dd::Add<DdType, double> shieldMask =
            Result::computeMaskAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 0.5, storm::OptimizationDirection::Maximize);
        storm::dd::Add<DdType, double> fallback =
            Result::computeFallbackAdd(*fixture.model, transitionMatrix, result.getChoiceValues(), 1.1, storm::OptimizationDirection::Maximize);

        expectStateValue(shieldMask, fixture.stateVariable, 0, fixture.actionABit, "mask");
        expectStateValue(fallback, fixture.stateVariable, 0, fixture.actionABit, "fallback");

        // The goal state is terminal in a model built for an until formula and has no choices.
        expectStateValue(shieldMask, fixture.stateVariable, 1, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 1, 0, "fallback");

        expectStateValue(shieldMask, fixture.stateVariable, 2, 0, "mask");
        expectStateValue(fallback, fixture.stateVariable, 2, fixture.actionBBit, "fallback");
    });
}
