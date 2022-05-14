#include "domain_abstraction.h"

#include <unordered_set>
#include <utility>
#include <cassert>

using namespace std;

namespace domain_abstractions {

    void DomainAbstraction::reload(VariableGroupVectors newAbstraction) {
        variableGroupVectors = std::move(newAbstraction);
        //nValuesForHash.erase(nValuesForHash.begin(), nValuesForHash.end());
        //nValuesForHash.reserve(variableGroupVectors.size());
        //fill(nValuesForHash.begin(), nValuesForHash.end(), 0);
        computeNValues();
    }

    VariableGroupVectors DomainAbstraction::getAbstractDomains() {
        return variableGroupVectors;
    }

    long long DomainAbstraction::abstractStateLookupIndex(vector<int> &abstractStateRepresentation) {
        // maps All sates to a number in range {0,..., #Abstractstates-1}
        long long index = 0;
        assert((*min_element(nValuesForHash.begin(), nValuesForHash.end())) >=0);
        assert((*min_element(abstractStateRepresentation.begin(), abstractStateRepresentation.end())) >=0);
        // loop over all abstraction parts/domains
        for (int i = 0; i < (int) abstractStateRepresentation.size(); i++) {
            index += (nValuesForHash.at(i) * abstractStateRepresentation.at(i));
        }
        if (index < 0) {
            log << index << "-- in abstr state: " << abstractStateRepresentation << endl;
        }
        assert(index >= 0);
        return index;
    }

    VariableGroupVector DomainAbstraction::getGroupAssignmentsForConcreteState(vector<int> &stateValues) {
        /*
         * Given a concrete State this method returns an Abstract state aka group assignment
         * */
        VariableGroupVector abstractStateRepresentation;
        for (int i = 0; i < (int) stateValues.size(); i++) {
            FactPair tmpPair = FactPair(i, stateValues[i]);
            abstractStateRepresentation.push_back(getGroupForFact(tmpPair));
        }
        return abstractStateRepresentation;
    }

    // TODO: Generally what to do when error return ex -1
    int DomainAbstraction::getGroupForFact(FactPair fact) {
        // Returns group number inside abstract variable domain given a fact.
        int domainIndex = getDomainIndexOfVariableValue(fact.var, fact.value);
        assert(domainIndex >= 0);
        return variableGroupVectors[fact.var][domainIndex];
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
        for (int inDomainIndex = 0; inDomainIndex < (int) domainToGroups.size(); inDomainIndex++) {
            if (domainToGroups[inDomainIndex] == groupNumber) {
                // TODO does order matter?
                myFacts.push_back(variableProxy.get_fact(inDomainIndex).get_pair());
            }
        }
        return myFacts;
    }

    bool DomainAbstraction::groupAssignmentFulfillsFacts(vector<int> abstractCandidate,
                                                         const vector<FactPair> &factsToFulfill) {
        /*
         * Checks whether given Abstract state(by its group assignment for variables) fulfills a list of fact pairs
         * (variable assignments). Can be used to check preconditions and isGoal.
         * */
        for (auto wantedFact: factsToFulfill) {
            // get group number of abstract state
            int group = abstractCandidate.at(wantedFact.var);
            vector<FactPair> groupFacts = getVariableGroupFacts(wantedFact.var, group);
            //log << "Check fact " << wantedFact << endl;
            //log << "According group facts for group " << group  << ": " << groupFacts << endl;
            if (find(groupFacts.begin(), groupFacts.end(), wantedFact) == groupFacts.end()) {
                // if I cannot find the wanted fact in my group return false
                return false;
            }
        }
        return true;
    }


    DomainAbstraction::DomainAbstraction(VariableGroupVectors domainMap, utils::LogProxy &log, TaskProxy originalTask,
                                         shared_ptr<TransitionSystem> transitionSystem) :
            transition_system(std::move(transitionSystem)),
            concrete_initial_state(originalTask.get_initial_state()),
            goal_facts(task_properties::get_fact_pairs(originalTask.get_goals())),
            originalTask(originalTask),
            log(log),
            variableGroupVectors(std::move(domainMap)),
            nValuesForHash(variableGroupVectors.size(), 0){
        // precompute N-Values for perfect hash function that is needed for lookup
        computeNValues();
    }

    void DomainAbstraction::computeNValues() {
        /*
         * uses current AbstractDomains to calculate the N Values that are needed for the perfect hash function.
         * The perfect hash function maps Abstract states to an index which is used for lookup h values and to
         * store them when computed
         * */
        // loop over all variables (we always have all variables considered in comparison to this hash for PDB)
        long long newValue = 1;
        for (int i = 0; i < (int) variableGroupVectors.size(); i++) {
            newValue = 1;
            // loop over groups in abstract domain for current variable v_i
            for (int j = 0; j <= i - 1; j++) {
                VariableGroupVector varGroupMapping = variableGroupVectors[j];
                long long numGroups = (*max_element(varGroupMapping.begin(), varGroupMapping.end())) + 1;
                assert(numGroups > 0);
                newValue *= numGroups;
            }
            if (newValue < 0) {
                log << "Current domain map(on err nvalue): " << variableGroupVectors << endl;
            }
            assert(newValue >= 0);
            nValuesForHash[i] = newValue;
        }
        long long min = *min_element(nValuesForHash.begin(), nValuesForHash.end());
        //log << nValuesForHash << endl;
        assert(nValuesForHash.size() == variableGroupVectors.size());
        assert((min) >=0);
    }

    bool DomainAbstraction::isGoal(const shared_ptr<DomainAbstractedState>& candidate) {
        /*
         * Check if the abstract state id is present in goalAbstractStates List -> if yes it is a goal state
         * */
        //log << "Is " << candidate->getGroupsAssignment() << " goal?" << endl;
        vector<int> candidateGroupAssignments = candidate->getGroupsAssignment();
        return groupAssignmentFulfillsFacts(candidateGroupAssignments, goal_facts);
    }


    DomainAbstractedStates DomainAbstraction::getSuccessors(const shared_ptr<DomainAbstractedState>& state) {
        /*
         * Returns a List of Abstract states (DomainAbstractedState) (representation of one state = vec<int>) that are the
         * possible successors of the given Abstract state. It uses a map to store already found successors, so that duplicated are
         * */
        map<long long, shared_ptr<DomainAbstractedState>> alreadyFound; // store Id's of already found successors

        // loop over all operators
        for (int operatorIndex = 0; operatorIndex < transition_system->get_num_operators(); operatorIndex++) {
            vector<FactPair> preconditionsForOperator = transition_system->get_precondition_assignments_for_operator(
                    operatorIndex);
            //log << "Is op " << operatorIndex << " applicable for abstr-state " << state->getGroupsAssignment() << " : " << op_is_applicable << endl;
            if (groupAssignmentFulfillsFacts(state->getGroupsAssignment(), preconditionsForOperator)) {
                vector<FactPair> postconditionsForOperator = transition_system->get_postcondition_assignments_for_operator(
                        operatorIndex);
                //log << "APPLICABLE, apply postconditions: " << postconditionsForOperator << endl;
                VariableGroupVector successorGroupMapping(state->getGroupsAssignment());
                //successorGroupMapping.reserve(currentState.size());
                for (const FactPair &fact: postconditionsForOperator) {
                    successorGroupMapping.at(fact.var) = getGroupForFact(fact);
                }

                long long abstractStateIndex = abstractStateLookupIndex(successorGroupMapping);
                int newGValue = state->getGValue() + task_properties::get_operator_costs(originalTask)[operatorIndex];
                // If we have not already found this successor -> add it to results
                if ((alreadyFound.find(abstractStateIndex) == alreadyFound.end())) {
                    shared_ptr<DomainAbstractedState> successorState = make_shared<DomainAbstractedState>(successorGroupMapping,
                                                                                                          abstractStateIndex);
                    // just set operator id, not parent yet, as this is not clear until expanded during search
                    successorState->set_operator_id(operatorIndex);
                    successorState->setGValue(newGValue);
                    alreadyFound.insert(pair<long long, shared_ptr<DomainAbstractedState>>(abstractStateIndex, successorState));
                } else if (newGValue < alreadyFound.at(abstractStateIndex)->getGValue()) {
                    // we found a cheaper operator to reach the same successor!
                    alreadyFound.at(abstractStateIndex)->setGValue(newGValue);
                    alreadyFound.at(abstractStateIndex)->set_operator_id(operatorIndex);
                }
            }
        }
        // extract values from map
        DomainAbstractedStates successorList;
        successorList.reserve(alreadyFound.size());
        for (const auto& pair: alreadyFound) {
            successorList.push_back(pair.second);
        }
        // Sort by Operator cost (needed in case of early goal check in uniform cost search)
        sort(successorList.begin(), successorList.end(), DomainAbstractedState::getComparator());
        reverse(successorList.begin(), successorList.end());

        return successorList;
    }

    shared_ptr<DomainAbstractedState> DomainAbstraction::getInitialAbstractState() {
        /*
         * Uses concrete Initial state and current variable-group mapping to derive abstract state
         * */
        vector<int> variableValues = concrete_initial_state.get_unpacked_values();
        assert(originalTask.get_variables().size() ==
               variableValues.size()); // STATE safes all variable values? even when one not assigned?
        // store group assignment for initial State
        vector<int> abstractStateGroups;
        abstractStateGroups.reserve(variableValues.size());
        // loop over variables in concrete init state == all variables
        for (int varIndex = 0; varIndex < (int) variableValues.size(); varIndex++) {
            // insert group number for variable x in the vector describing abstract
            int domainIndex = getDomainIndexOfVariableValue(varIndex, variableValues[varIndex]);
            abstractStateGroups.push_back(variableGroupVectors[varIndex][domainIndex]);
        }
        return make_shared<DomainAbstractedState>(abstractStateGroups, abstractStateLookupIndex(abstractStateGroups));
    }

}