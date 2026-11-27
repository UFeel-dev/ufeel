#include "ufeel/Rules/RuleSet.hpp"

#include <exception>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

TEST(
    RuleSetTest, RunsActionsWhoseConditionHolds
)
{
    ufeel::RuleSet rules;
    int runs = 0;
    bool enabled = false;
    rules.add([&enabled] -> bool { return enabled; }, [&runs] -> void { ++runs; }, false);

    rules.evaluate();
    enabled = true;
    rules.evaluate();
    rules.evaluate();

    EXPECT_EQ(runs, 2);
}

TEST(
    RuleSetTest, RunsOnceOnlyRulesOnce
)
{
    ufeel::RuleSet rules;
    int runs = 0;
    rules.add([] -> bool { return true; }, [&runs] -> void { ++runs; }, true);

    rules.evaluate();
    rules.evaluate();

    EXPECT_EQ(runs, 1);
}

TEST(
    RuleSetTest, RemovesAndClearsRules
)
{
    ufeel::RuleSet rules;
    int runs = 0;
    const auto removed = rules.add([] -> bool { return true; }, [&runs] -> void { ++runs; }, false);
    rules.remove(removed);
    rules.evaluate();

    rules.add([] -> bool { return true; }, [&runs] -> void { ++runs; }, false);
    rules.evaluate();
    rules.clear();
    rules.evaluate();

    EXPECT_EQ(runs, 1);
}

TEST(
    RuleSetTest, AppliesChangesMadeByActionsOnTheNextEvaluation
)
{
    ufeel::RuleSet rules;
    int added = 0;
    rules.add(
        [] -> bool { return true; },
        [&rules, &added] -> void {
            rules.add([] -> bool { return true; }, [&added] -> void { ++added; }, false);
        },
        true
    );

    rules.evaluate();
    EXPECT_EQ(added, 0);

    rules.evaluate();
    EXPECT_EQ(added, 1);
}

TEST(
    RuleSetTest, ReportsErrorsToTheHandlerAndKeepsEvaluating
)
{
    ufeel::RuleSet rules;
    std::string error;
    int runs = 0;
    rules.setErrorHandler([&error](const std::exception_ptr& exception) -> void {
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception& caught)
        {
            error = caught.what();
        }
    });
    rules.add([] -> bool { throw std::runtime_error("broken condition"); }, [] -> void {}, false);
    rules.add([] -> bool { return true; }, [&runs] -> void { ++runs; }, false);

    rules.evaluate();

    EXPECT_EQ(error, "broken condition");
    EXPECT_EQ(runs, 1);
}

TEST(
    RuleSetTest, RethrowsErrorsWithoutAHandler
)
{
    ufeel::RuleSet rules;
    rules.add(
        [] -> bool { return true; }, [] -> void { throw std::runtime_error("broken action"); }, true
    );

    EXPECT_THROW(rules.evaluate(), std::runtime_error);
    EXPECT_NO_THROW(rules.evaluate());
}

TEST(
    RuleSetTest, RejectsNestedEvaluation
)
{
    ufeel::RuleSet rules;
    rules.add([] -> bool { return true; }, [&rules] -> void { rules.evaluate(); }, true);

    EXPECT_THROW(rules.evaluate(), std::logic_error);
}

TEST(
    RuleSetTest, RejectsIncompleteRules
)
{
    ufeel::RuleSet rules;

    EXPECT_THROW(rules.add({}, [] -> void {}, false), std::invalid_argument);
    EXPECT_THROW(rules.add([] -> bool { return true; }, {}, false), std::invalid_argument);
}
