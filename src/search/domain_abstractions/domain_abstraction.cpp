#include "domain_abstraction.h"

namespace domain_abstractions {

    void DomainAbstraction::reload(VariableGroupVectors newAbstraction) {
        variableGroupVectors = newAbstraction;
        nValuesForHash = computeNValues();
    }

    VariableGroupVectors DomainAbstraction::getAbstractDomains() {
        return variableGroupVectors;
    }

    int DomainAbstraction::abstractStateLookupIndex(vector<int> abstractStateRepresentation) {
        // maps All sates to a number in range {0,..., #Abstractstates-1}
        int index = 0;
        // loop over all abstraction parts/domains
        for (int i = 0; i < (int)abstractStateRepresentation.size(); i++) {
            index += (nValuesForHash.at(i) * abstractStateRepresentation.at(i));
        }
        return index;
    }

    const std::vector<FactPair> DomainAbstraction::getGoalFacts() {
        return goal_facts;
    }

    VariableGroupVector DomainAbstraction::getGroupAssignmentsForConcreteState(vector<int> stateValues) {
        /*
         * Given a concrete State this method returns an Abstract state aka group assignment
         * */
        VariableGroupVector abstractStateRepresentation;
        for (int i = 0; i < (int) stateValues.size(); i++) {
            //TODO: State unpacked values has values of all vars?
            FactPair tmpPair = FactPair(i, stateValues[i]);
            abstractStateRepresentation.push_back(getGroupForFact(tmpPair));
        }
        return abstractStateRepresentation;
    }

    // TODO: Generally what to do when error return ex -1
    int DomainAbstraction::getGroupForFact(FactPair fact) {
        // Returns group number inside abstract variable domain given a fact.
        return variableGroupVectors[fact.var][getDomainIndexOfVariableValue(fact.var, fact.value)];
    }

    int DomainAbstraction::getDomainIndexOfVariableValue(int variable, int value) {
        // Get index of a value inside variable domain -> ex we can use it to access abstract mapping.
        VariableProxy variableProxy = originalTask.get_variables()[variable];
        for (int domainIndex = 0; domainIndex < variableProxy.get_domain_size(); domainIndex++) {
            if (variableProxy.get_fact(domainIndex).get_value() == value) {
                return domainIndex;
            }
        }
        return -1; // NOT FOUND
    }

    vector<FactPair> DomainAbstraction::getVariableGroupFacts(int varIndex, int groupNumber) {
        /*
         * Function that returns the facts(variable assignments) for a specific variable and group.
         * */
        // get Variable proxy for specific variable
        VariableProxy variableProxy = originalTask.get_variables()[varIndex];
        VariableGroupVector domainToGroups = variableGroupVectors.at(varIndex);
        vector<FactPair> myFacts;
        // loop over domain
        for (int inDomainIndex = 0; inDomainIndex < (int)domainToGroups.size(); inDomainIndex++) {
            if (domainToGroups[inDomainIndex] == groupNumber) {
                // TODO does order matter?
                myFacts.push_back(variableProxy.get_fact(inDomainIndex).get_pair());
            }
        }
        return myFacts;
    }

    bool DomainAbstraction::groupAssignmentFulfillsFacts(const vector<int> &abstractCandidate, const vector<FactPair> &existingFactPairs) {
        /*
         * Checks whether given Abstract state(by its group assignment for variables) fulfills a list of fact pairs
         * (variable assignments). Can be used to check preconditions and isGoal.
         * */
        // loop over all group assignments
        for (int varIndex = 0; varIndex < (int)abstractCandidate.size(); varIndex++) {
            // get group number of abstract state
            int group = abstractCandidate.at(varIndex);
            // get group facts
            vector<FactPair> groupFacts = getVariableGroupFacts(varIndex, group);
            // check if all existing factPairs contained
            for (FactPair factPair: existingFactPairs) {
                if (std::find(groupFacts.begin(), groupFacts.end(), factPair) == groupFacts.end()) {
                    return false;
                }
            }
        }
        return true;
    }


    DomainAbstraction::DomainAbstraction(VariableGroupVectors domainMap, TaskProxy originalTask, shared_ptr<TransitionSystem> transitionSystem) :
            transition_system(transitionSystem),
            concrete_initial_state(originalTask.get_initial_state()),
            goal_facts(task_properties::get_fact_pairs(originalTask.get_goals())),
            originalTask(originalTask),
            variableGroupVectors(domainMap){
        // precompute N-Values for perfect hash function that is needed for lookup
        nValuesForHash = computeNValues();
    }

    vector<int> DomainAbstraction::computeNValues() {
        /*
         * uses current AbstractDomains to calculate the N Values that are needed for the perfect hash function.
         * The perfect hash function maps Abstract states to an index which is used for lookup h values and to
         * store them when computed
         * */
        vector<int> newNvalues;
        // loop over all variables (we always have all variables considered in comparison to this hash for PDB)
        for (int i = 0; i < (int)variableGroupVectors.size(); i++) {
            int newValue = 1;
            // loop over groups in abstract domain -> TODO: need to consider group size instead of domain-size of var
            for (int j = 0; j <= i-1; j++) {
                VariableGroupVector varGroupMapping = variableGroupVectors.at(j);
                int numGroups = *max_element(varGroupMapping.begin(), varGroupMapping.end());;
                newValue *= numGroups;
            }
            newNvalues.push_back(newValue);
        }
        return newNvalues;
    }

    bool DomainAbstraction::isGoal(DomainAbstractedState* candidate) {
        /*
         * Check if the abstract state id is present in goalAbstractStates List -> if yes it is a goal state
         * */
        vector<int> candidateGroupAssignments = candidate->getGroupsAssignment();
        return groupAssignmentFulfillsFacts(candidateGroupAssignments , goal_facts);
    }


    DomainAbstractedStates DomainAbstraction::getSuccessors(DomainAbstractedState* state) {
        /*
         * Returns a List of Abstract states (DomainAbstractedState) (representation of one state = vec<int>) that are the
         * possible successors of the given Abstract state.
         * */
        DomainAbstractedStates assignmentsOfSuccessors;
        VariableGroupVector currentState = state->getGroupsAssignment();

        // loop over all operators TODO: Is get num sufficient -> Ids from 0 to num ops?
        for (int operatorIndex = 0; operatorIndex < transition_system->get_num_operators(); operatorIndex++) {
            const vector<FactPair> preconditionsForOperator = transition_system->get_precondition_assignments_for_operator(operatorIndex);
            // if operator is applicable create new abstract state out of it
            if(groupAssignmentFulfillsFacts(currentState, preconditionsForOperator)) {
                // TODO get postcondition assignments -> contain all assignments for every variable or just of that that were changed ??
                const vector<FactPair> postconditionsForOperator = transition_system->get_postcondition_assignments_for_operator(operatorIndex);
                VariableGroupVector successorGroupMapping;
                successorGroupMapping.reserve(currentState.size());
                // TODO For now assume all assignments included
                for (const FactPair &fact: postconditionsForOperator) {
                    successorGroupMapping.insert(successorGroupMapping.begin() + fact.var, getGroupForFact(fact));
                }
                DomainAbstractedState* successorState = new DomainAbstractedState(successorGroupMapping,
                                                                                  abstractStateLookupIndex(successorGroupMapping));
                successorState->set_operator_id(operatorIndex);
                assignmentsOfSuccessors.push_back(successorState);
            }
        }
        return assignmentsOfSuccessors;
    }

    DomainAbstractedState *DomainAbstraction::getInitialAbstractState() {
        /*
         * Uses concrete Initial state and current variable-group mapping to derive abstract state
         * */
        vector<int> variableValues = concrete_initial_state.get_unpacked_values();
        assert(originalTask.get_variables().size() == variableValues.size()); // STATE safes all variable values? even when one not assigned?
        // store group assignment for initial State
        vector<int> abstractStateGroups;
        abstractStateGroups.reserve(variableValues.size());
        // loop over variables in concrete init state == all variables
        for (int varIndex = 0; varIndex < (int)variableValues.size(); varIndex++) {
            // insert group number for variable x in the vector describing abstract
            int domainIndex = getDomainIndexOfVariableValue(varIndex, variableValues[varIndex]);
            abstractStateGroups.insert(abstractStateGroups.begin() + varIndex, variableGroupVectors[varIndex][domainIndex]);
        }
        return new DomainAbstractedState(abstractStateGroups);
    }

}