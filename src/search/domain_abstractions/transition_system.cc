#include "transition_system.h"

#include "../utils/logging.h"
#include "../task_utils/task_properties.h"

#include <algorithm>
#include <map>

using namespace std;

namespace domain_abstractions {

    static vector <vector<FactPair>> get_preconditions_by_operator(
            const OperatorsProxy &ops) {
        /*
         * Seems to return one vector per operator. Each vector is containing the preconditions for applying the
         * operator.
         * */
        vector<vector<FactPair>> preconditions_by_operator;
        preconditions_by_operator.reserve(ops.size());
        for (OperatorProxy op: ops) {
            vector<FactPair> preconditions = task_properties::get_fact_pairs(op.get_preconditions());
            sort(preconditions.begin(), preconditions.end());
            preconditions_by_operator.push_back(move(preconditions));
        }
        return preconditions_by_operator;
    }

    static vector <FactPair> get_variable_assignments_for_operator(
            const OperatorProxy &op) {
        // Use map to obtain sorted postconditions.
        map<int, int> var_to_post;
        // get precondition fact pairs
        for (FactProxy fact: op.get_preconditions()) {
            var_to_post[fact.get_variable().get_id()] = fact.get_value();
        }
        // get postcondition fact pairs
        for (EffectProxy effect: op.get_effects()) {
            FactPair fact = effect.get_fact().get_pair();
            var_to_post[fact.var] = fact.value;
        }
        vector<FactPair> postconditions;
        postconditions.reserve(var_to_post.size());
        for (const pair<const int, int> &fact: var_to_post) {
            postconditions.emplace_back(fact.first, fact.second);
        }
        return postconditions;
    }

    static vector <vector<FactPair>> get_postconditions_by_operator(
            const OperatorsProxy &ops) {
        /*
         * Returns a vector of FactPair Vectors. Each vector is representing the postconditions that hold
         * after the according operator has been applied. Uses get_variable_assignments_for_operator method.
         * */
        vector<vector<FactPair>> postconditions_by_operator;
        postconditions_by_operator.reserve(ops.size());
        for (OperatorProxy op: ops) {
            postconditions_by_operator.push_back(get_variable_assignments_for_operator(op));
        }
        return postconditions_by_operator;
    }


    TransitionSystem::TransitionSystem(const OperatorsProxy &ops, TaskProxy proxy, utils::LogProxy &log)
            : preconditions_by_operator(get_preconditions_by_operator(ops)),
              postconditions_by_operator(get_postconditions_by_operator(ops)),
              originalTask(proxy), log(log),
              concreteInitialState(originalTask.get_initial_state().get_unpacked_values()),
              goalFacts(task_properties::get_fact_pairs(originalTask.get_goals())) {
    }

    vector <FactPair> TransitionSystem::transitionApplicable(vector<int> currentState, Transition toApply) const {
        /*
         * Checks whether a Transition is applicable in the original Task! If Yes, an empty vector is returned, if not
         * The facts that are needed for fulfillment == all preconditions
         * */
        vector<FactPair> neededAssignments = preconditions_by_operator[toApply.op_id];
        vector<FactPair> missedFacts;
        assert(!neededAssignments.empty());
        for (FactPair &pair: neededAssignments) {
            if (pair.value != currentState.at(pair.var)) {
                missedFacts.push_back(pair);
            }
        }
        return missedFacts;
    }

    vector <FactPair> TransitionSystem::isGoal(const vector<int> &currentState) {
        /*
         * Checks whether a Transition is applicable in the original Task! If Yes, an empty vector is returned, if not
         * The facts that are needed for fulfillment == all preconditions
         * */
        vector<FactPair> missedFacts;
        //log << "loop over needed assignments" << endl;
        for (FactPair &pair: goalFacts) {
            if (pair.value != currentState.at(pair.var)) {
                missedFacts.push_back(pair);
            }
        }
        return missedFacts;
    }

    vector<int> TransitionSystem::applyOperator(vector<int> currentValues, int op_id) const {
        /*
         * CurrentState is modified by changing values implied by postconditions of a given operator(with op_id).
         * */
        for (int i = 0; i < (int) currentValues.size(); i++) {
            int newVal = get_postcondition_value(op_id, i);
            if (newVal != -1) {
                currentValues[i] = newVal;
            }
        }
        return currentValues;
    }

    int TransitionSystem::get_precondition_value(int op_id, int var) const {
        return lookup_value(preconditions_by_operator[op_id], var);
    }

    int TransitionSystem::get_postcondition_value(int op_id, int var) const {
        return lookup_value(postconditions_by_operator[op_id], var);
    }

    vector <FactPair> TransitionSystem::get_precondition_assignments_for_operator(int operatorID) const {
        return preconditions_by_operator[operatorID];
    }

    vector <FactPair> TransitionSystem::get_postcondition_assignments_for_operator(int operatorID) const {
        return postconditions_by_operator[operatorID];
    }

    int TransitionSystem::get_num_operators() const {
        return (int) preconditions_by_operator.size();
    }

    std::vector<int> TransitionSystem::getInitialState() {
        return concreteInitialState;
    }

    std::vector<FactPair> TransitionSystem::getGoalFacts() {
        return goalFacts;
    }
}
