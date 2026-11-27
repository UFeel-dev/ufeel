#include "ufeel/Rules/RuleSet.hpp"

#include <cstdint>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ufeel
{

RuleKey::RuleKey(
    std::uint64_t id
)
    : id_(id)
{
}

auto RuleSet::add(
    Condition condition, Action action, bool onlyOnce
) -> RuleKey
{
    if (!condition || !action)
    {
        throw std::invalid_argument("A rule needs a condition and an action.");
    }

    const std::uint64_t id = nextId_++;
    pendingAdds_.push_back({
        .id = id,
        .condition = std::move(condition),
        .action = std::move(action),
        .onlyOnce = onlyOnce,
    });
    return RuleKey(id);
}

void RuleSet::remove(
    RuleKey key
)
{
    pendingRemovals_.insert(key.id_);
}

void RuleSet::clear()
{
    for (const Rule& rule : rules_)
    {
        pendingRemovals_.insert(rule.id);
    }
    pendingAdds_.clear();
}

void RuleSet::evaluate()
{
    if (evaluating_)
    {
        throw std::logic_error("A rule cannot evaluate its own rule set.");
    }

    evaluating_ = true;
    try
    {
        applyPendingChanges();
        for (const Rule& rule : rules_)
        {
            if (pendingRemovals_.contains(rule.id))
            {
                continue;
            }

            try
            {
                if (!rule.condition())
                {
                    continue;
                }

                if (rule.onlyOnce)
                {
                    pendingRemovals_.insert(rule.id);
                }
                rule.action();
            }
            catch (...)
            {
                if (!errorHandler_)
                {
                    throw;
                }
                errorHandler_(std::current_exception());
            }
        }
        applyPendingChanges();
    }
    catch (...)
    {
        evaluating_ = false;
        throw;
    }
    evaluating_ = false;
}

void RuleSet::setErrorHandler(
    ErrorHandler handler
)
{
    errorHandler_ = std::move(handler);
}

void RuleSet::applyPendingChanges()
{
    rules_.insert(
        rules_.end(),
        std::make_move_iterator(pendingAdds_.begin()),
        std::make_move_iterator(pendingAdds_.end())
    );
    pendingAdds_.clear();

    std::erase_if(rules_, [this](const Rule& rule) -> bool {
        return pendingRemovals_.contains(rule.id);
    });
    pendingRemovals_.clear();
}

} // namespace ufeel
