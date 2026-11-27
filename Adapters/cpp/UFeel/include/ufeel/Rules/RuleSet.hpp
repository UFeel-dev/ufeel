#ifndef UFEEL_RULES_RULESET_HPP
#define UFEEL_RULES_RULESET_HPP

#include <cstdint>
#include <exception>
#include <functional>
#include <unordered_set>
#include <vector>

namespace ufeel
{

class RuleKey
{
public:

    auto operator==(const RuleKey&) const -> bool = default;

private:

    friend class RuleSet;

    explicit RuleKey(std::uint64_t id);

    std::uint64_t id_;
};

class RuleSet
{
public:

    using Condition = std::function<bool()>;
    using Action = std::function<void()>;
    using ErrorHandler = std::function<void(std::exception_ptr)>;

    auto add(Condition condition, Action action, bool onlyOnce) -> RuleKey;
    void remove(RuleKey key);
    void clear();
    void evaluate();

    void setErrorHandler(ErrorHandler handler);

private:

    struct Rule
    {
        std::uint64_t id;
        Condition condition;
        Action action;
        bool onlyOnce;
    };

    void applyPendingChanges();

    std::vector<Rule> rules_;
    std::vector<Rule> pendingAdds_;
    std::unordered_set<std::uint64_t> pendingRemovals_;
    std::uint64_t nextId_ = 0;
    ErrorHandler errorHandler_;
    bool evaluating_ = false;
};

} // namespace ufeel

#endif // UFEEL_RULES_RULESET_HPP
