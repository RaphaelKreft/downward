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
/*
  Rewire transitions after each split.
*/
    class TransitionSystem {
        const std::vector<std::vector<FactPair>> preconditions_by_operator;
        const std::vector<std::vector<FactPair>> postconditions_by_operator;
        TaskProxy originalTask;
        utils::LogProxy &log;

        // Transitions from and to other abstract states.
        std::vector<Transitions> incoming;
        std::vector<Transitions> outgoing;

    public:
        explicit TransitionSystem(const OperatorsProxy &ops, TaskProxy proxy, utils::LogProxy &log);

        const std::vector<Transitions> &get_incoming_transitions() const;

        const std::vector<Transitions> &get_outgoing_transitions() const;

        int get_precondition_value(int op_id, int var) const;

        int get_postcondition_value(int op_id, int var) const;

        std::vector<FactPair> get_precondition_assignments_for_operator(int operatorID) const;

        std::vector<FactPair> get_postcondition_assignments_for_operator(int operatorID) const;

        // Difference to those methods in task_properties is, that there return the facts that have been missed!
        // = Facts that must have been true additionally for the wanted outcome!
        std::vector<FactPair> transitionApplicable(std::vector<int> currentState, Transition toApply) const;

        std::vector<FactPair> isGoal(const std::vector<int>& currentState);

        std::vector<int> applyOperator(std::vector<int> currenValues, int op_id) const;

        int get_num_states() const;

        int get_num_operators() const;

        static int lookup_value(const std::vector<FactPair> &facts, int var) {
            /*
             * Given a vector of FactPairs and a variable number this method returns the value of this variable
             * in the vector or -1 if var not found. Used for ex:
             * */
            assert(is_sorted(facts.begin(), facts.end()));
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
