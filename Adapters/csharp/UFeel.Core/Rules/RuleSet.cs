#nullable enable

using System;
using System.Collections.Generic;

namespace UFeel
{
    public readonly struct RuleKey : IEquatable<RuleKey>
    {
        internal RuleKey(int id) => Id = id;

        internal int Id { get; }

        public bool Equals(RuleKey other) => Id == other.Id;
        public override bool Equals(object? obj) => obj is RuleKey other && Equals(other);
        public override int GetHashCode() => Id;
    }

    /// <summary>
    /// Not thread-safe: add, remove and evaluate from the same (main) thread.
    /// Additions and removals apply on the next evaluation.
    /// </summary>
    public sealed class RuleSet
    {
        private readonly List<Rule> rules = new List<Rule>();
        private readonly List<Rule> pendingAdds = new List<Rule>();
        private readonly HashSet<int> pendingRemovals = new HashSet<int>();
        private int nextId;

        /// <summary>Receives exceptions from conditions and actions. Rethrown when null.</summary>
        public Action<Exception>? ErrorHandler { get; set; }

        public RuleKey Add(Func<bool> condition, Action action, bool onlyOnce)
        {
            if (condition is null)
                throw new ArgumentNullException(nameof(condition));
            if (action is null)
                throw new ArgumentNullException(nameof(action));

            var rule = new Rule(nextId++, condition, action, onlyOnce);
            pendingAdds.Add(rule);
            return new RuleKey(rule.Id);
        }

        public void Remove(RuleKey key) => pendingRemovals.Add(key.Id);

        public void Clear()
        {
            foreach (Rule rule in rules)
                pendingRemovals.Add(rule.Id);

            pendingAdds.Clear();
        }

        public void Evaluate()
        {
            ApplyPendingChanges();

            foreach (Rule rule in rules)
            {
                if (pendingRemovals.Contains(rule.Id))
                    continue;

                try
                {
                    if (!rule.Condition())
                        continue;

                    if (rule.OnlyOnce)
                        pendingRemovals.Add(rule.Id);

                    rule.Action();
                }
                catch (Exception exception) when (ErrorHandler != null)
                {
                    ErrorHandler(exception);
                }
            }

            ApplyPendingChanges();
        }

        private void ApplyPendingChanges()
        {
            if (pendingAdds.Count > 0)
            {
                rules.AddRange(pendingAdds);
                pendingAdds.Clear();
            }

            if (pendingRemovals.Count > 0)
            {
                rules.RemoveAll(rule => pendingRemovals.Contains(rule.Id));
                pendingRemovals.Clear();
            }
        }

        private sealed class Rule
        {
            public Rule(int id, Func<bool> condition, Action action, bool onlyOnce)
            {
                Id = id;
                Condition = condition;
                Action = action;
                OnlyOnce = onlyOnce;
            }

            public int Id { get; }
            public Func<bool> Condition { get; }
            public Action Action { get; }
            public bool OnlyOnce { get; }
        }
    }
}
