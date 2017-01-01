#ifndef AI_TOOLBOX_FACTORED_MDP_MULTI_OBJECTIVE_VARIABLE_ELIMINATION_HEADER_FILE
#define AI_TOOLBOX_FACTORED_MDP_MULTI_OBJECTIVE_VARIABLE_ELIMINATION_HEADER_FILE

#include "AIToolbox/FactoredMDP/Types.hpp"
#include "AIToolbox/FactoredMDP/Utils.hpp"
#include "AIToolbox/FactoredMDP/FactorGraph.hpp"

#include <AIToolbox/Utils.hpp>

namespace AIToolbox {
    namespace FactoredMDP {
        /**
         * @brief This class represents the Multi Objective Variable Elimination process.
         *
         * This class performs variable elimination on a factor graph. It first
         * builds the graph starting from a list of MOQFunctionRules. These
         * rules are sorted by the agents they affect, and each group is added
         * to a single factor connected to those agents.
         *
         * Each agent is then eliminated from the graph, and all rules
         * connected to it are processed in order to find out which actions the
         * agent being eliminated should take.
         *
         * When doing multi-objective elimination, there is no real best action
         * in general, since we do not know in advance the weights for the
         * objectives' rewards. Thus, we keep all action-rewards pairs we found
         * during the elimination and return them.
         *
         * This process is exponential in the maximum number of agents found
         * attached to the same factor (which could be higher than in the
         * original graph, as the elimination process can create bigger factors
         * than the initial ones). However, given that each factor is usually
         * linked to few agents, and that this process allows avoiding
         * considering the full factored Action at any one time, it is usually
         * much faster than a brute-force approach.
         */
        class MultiObjectiveVariableElimination {
            public:
                using Values = std::vector<std::tuple<PartialAction, Rewards>>;
                using Rule = std::tuple<PartialAction, Values>;
                using Rules = std::vector<Rule>;

                using Results = Values;

                struct Factor {
                    Rules rules_;
                };

                using Graph = FactorGraph<Factor>;

                /**
                 * @brief Basic constructor.
                 *
                 * This constructor initializes the internal graph with the
                 * number of needed agents.
                 *
                 * @param a The action space.
                 */
                MultiObjectiveVariableElimination(Action a);

                /**
                 * @brief This function finds the best Action-value pair for the provided MOQFunctionRules.
                 *
                 * @param rules An iterable object over MOQFunctionRules.
                 *
                 * @return The pair of best Action and its value over the input rules.
                 */
                template <typename Iterable>
                Results operator()(const Iterable & rules) {
                    auto comp = [](const Rule & lhs, const Rule & rhs) {
                        return veccmp(std::get<0>(lhs).second, std::get<0>(rhs).second) < 0;
                    };
                    // Should we reset the graph?
                    for (const MOQFunctionRule & rule : rules) {
                        auto & rules = graph_.getFactor(rule.a_.first)->f_.rules_;

                        // Here we keep everything sorted since it will turn up
                        // useful later when we have to crossSum and merge two
                        // lists. Having them sorted makes us to less work later.
                        auto newRule = std::make_tuple(rule.a_, Values{std::make_pair(PartialAction(), rule.values_)});
                        auto pos = std::upper_bound(std::begin(rules), std::end(rules), newRule, comp);
                        rules.emplace(pos, std::move(newRule));
                    }
                    return start();
                }

            private:
                /**
                 * @brief This function performs the actual agent elimination process.
                 *
                 * For each agent, its adjacent factors, and the agents
                 * adjacent to those are found. Then all possible action
                 * combinations between those other agents are tried in order
                 * to find the best action response be for the agent to be
                 * eliminated.
                 *
                 * All the responses found (possibly pruned) are added as Rules
                 * to a (possibly new) factor adjacent to the adjacent agents.
                 *
                 * The process is repeated until all agents are eliminated.
                 *
                 * What remains is then returned.
                 *
                 * @return All pairs of PartialAction, Rewards found during the elimination process.
                 */
                Results start();

                /**
                 * @brief This function performs the elimination of a single agent (and all factors next to it) from the internal graph.
                 *
                 * This function adds the resulting rules which do not depend
                 * on the eliminated action to the remaining factors.
                 *
                 * \sa start()
                 *
                 * @param agent The index of the agent to be removed from the graph.
                 */
                void removeAgent(size_t agent);

                Graph graph_;
                Action A;
                std::vector<Values> finalFactors_;
        };
    }
}

#endif
