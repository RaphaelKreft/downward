#ifndef DOMAIN_ABSTRACTIONS_TRANSITION_SYSTEM_H
#define DOMAIN_ABSTRACTIONS_TRANSITION_SYSTEM_H

#include "data_structures.h"
#include "../task_proxy.h"

#include <vector>

struct FactPair;

class OperatorsProxy;

namespace utils {
    class LogProxy;
}

namespace domain_abstractions {

    class TransitionSystem {
        const std::vector<std::vector<FactPair>> preconditions_by_operator;
        const std::vector<std::vector<FactPair>> postconditions_by_operator;
        TaskProxy originalTask;
        utils::LogProxy &log;
        std::vector<int> concreteInitialState;
        std::vector<FactPair> goalFacts;

    public:
        explicit TransitionSystem(const OperatorsProxy &ops, TaskProxy proxy, utils::LogProxy &log);

        int get_precondition_value(int op_id, int var) const;

        int get_postcondition_value(int op_id, int var) const;

        std::vector<FactPair> get_precondition_assignments_for_operator(int operatorID) const;

        std::vector<FactPair> get_postcondition_assignments_for_operator(int operatorID) const;

        // Difference to those methods in task_properties is, that there return the facts that have been missed!
        // = Facts that must have been true additionally for the wanted outcome!
        std::vector<FactPair> transitionApplicable(std::vector<int> currentState, Transition toApply) const;

        std::vector<FactPair> isGoal(const std::vector<int> &currentState);

        std::vector<int> applyOperator(std::vector<int> currenValues, int op_id) const;

        int get_num_operators() const;

        std::vector<int> getInitialState();

        std::vector<FactPair> getGoalFacts();

        static int lookup_value(const std::vector<FactPair> &facts, int var) {
            /*
             * Given a vector of FactPairs and a variable number this method returns the value of this variable
             * in the vector or -1 if var not found. Used for ex:
             * */
            //assert(is_sorted(facts.begin(), facts.end()));
            for (const FactPair &fact: facts) {
                if (fact.var == var) {
                    return fact.value;
                } else if (fact.var > var) {
                    return -1;
                }
            }
            return -1;
        }
    };
}

#endif
