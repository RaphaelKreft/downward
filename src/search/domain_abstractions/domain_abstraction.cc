#include "domain_abstraction.h"
#include "da_utils.h"

#include <utility>
#include <cassert>

using namespace std;

namespace domain_abstractions {

    int DomainAbstraction::reload(VariableGroupVectors newAbstraction) {
        /*
         * Calculate new nvalues for hash. Check if product of NValues will be an Overflow(which can lead to bad indexes).
         * If this happens the reload is aborted and -1 is returned. Else the new values are set, the new abstraction is loaded
         * and 0 is returned.
         * */
        vector<int> newDomainSizes;
        // calculate new domain sizes
        for (auto &i: newAbstraction) {
            int max_var_group_num = *max_element(i.begin(), i.end());
            newDomainSizes.push_back((max_var_group_num + 1));
        }

        vector<long long> newValues = computeNValues(newAbstraction, newDomainSizes);

        // calculate max possible index
        long long maxIndex = 0;
        for (int i = 0; i < (int) newAbstraction.size(); i++) {
            maxIndex += (newValues.at(i) * (newDomainSizes.at(i) - 1));
        }
        if (maxIndex < 0 || maxIndex > max_states) {
            //keep old Abstraction
            return -1;
        }
        variableGroupVectors = std::move(newAbstraction);
        nValuesForHash = newValues;
        numAbstractStates = maxIndex + 1;
        domainSizes = newDomainSizes;

        preCalculateVariableGroupFacts();
        //generateAbstractTransitionSystem();
        return 0;
    }

    VariableGroupVectors DomainAbstraction::getAbstractDomains() {
        return variableGroupVectors;
    }

    long long DomainAbstraction::abstractStateLookupIndex(vector<int> &abstractStateRepresentation) {
        // maps All sates to a number in range {0,..., #Abstractstates-1}
        long long index = 0;
        // loop over all abstraction parts/domains
        for (int i = 0; i < (int) abstractStateRepresentation.size(); i++) {
            index += (nValuesForHash.at(i) * abstractStateRepresentation.at(i));
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

    int DomainAbstraction::getGroupForFact(FactPair fact) {
        // Returns group number inside abstract variable domain given a fact.
        return variableGroupVectors[fact.var][fact.value];
    }

    vector <FactPair> DomainAbstraction::getVariableGroupFacts(int varIndex, int groupNumber) {
        /*
         * Function that returns the facts(variable assignments) for a specific variable and group.
         * */
        // get Variable proxy for specific variable
        return variableGroupFacts[varIndex][groupNumber];
    }

    bool DomainAbstraction::groupAssignmentFulfillsFacts(vector<int> abstractCandidate,
                                                         const vector <FactPair> &factsToFulfill) {
        /*
         * Checks whether given Abstract state(by its group assignment for variables) fulfills a list of fact pairs
         * (variable assignments). Can be used to check preconditions and isGoal.
         * */
        for (auto wantedFact: factsToFulfill) {
            // get group number of abstract state
            int group = abstractCandidate.at(wantedFact.var);
            vector < FactPair > groupFacts = getVariableGroupFacts(wantedFact.var, group);
            if (find(groupFacts.begin(), groupFacts.end(), wantedFact) == groupFacts.end()) {
                // if I cannot find the wanted fact in my group return false
                return false;
            }
        }
        return true;
    }


    DomainAbstraction::DomainAbstraction(VariableGroupVectors domainMap, utils::LogProxy &log, TaskProxy originalTask,
                                         shared_ptr <TransitionSystem> transitionSystem, int max_states) :
            transition_system(std::move(transitionSystem)),
            concrete_initial_state(originalTask.get_initial_state()),
            goal_facts(task_properties::get_fact_pairs(originalTask.get_goals())),
            originalTask(originalTask),
            log(log),
            variableGroupVectors(std::move(domainMap)),
            nValuesForHash(variableGroupVectors.size(), 0),
            operatorCosts(task_properties::get_operator_costs(originalTask)),
            domainSizes(variableGroupVectors.size(), 1),
            max_states(max_states) {
        // precompute N-Values for perfect hash function that is needed for lookup
        reload(variableGroupVectors);
    }

    vector<long long>
    DomainAbstraction::computeNValues(VariableGroupVectors newAbstraction, vector<int> &newDomainSizes) {
        /*
         * uses current AbstractDomains to calculate the N Values that are needed for the perfect hash function.
         * The perfect hash function maps Abstract states to an index which is used for lookup h values and to
         * store them when computed
         * */
        // loop over all variables (we always have all variables considered in comparison to this hash for PDB)
        vector<long long> newNValuesForHash(newAbstraction.size(), 1);
        for (int i = 0; i < (int) newAbstraction.size(); i++) {
            // loop over groups in abstract domain for current variable v_i
            for (int j = 0; j <= i - 1; j++) {
                VariableGroupVector varGroupMapping = newAbstraction[j];
                int numGroups = newDomainSizes.at(j);
                assert(numGroups > 0);
                newNValuesForHash.at(i) *= numGroups;
            }
            assert(newNValuesForHash.at(i) >= 0);
        }
        assert(nValuesForHash.size() == variableGroupVectors.size());
        return newNValuesForHash;
    }

    bool DomainAbstraction::isGoal(const shared_ptr <DomainAbstractedState> &candidate) {
        /*
         * Check if the abstract state id is present in goalAbstractStates List -> if yes it is a goal state
         * */
        vector<int> candidateGroupAssignments = candidate->getGroupsAssignment();
        return groupAssignmentFulfillsFacts(candidateGroupAssignments, goal_facts);
    }


    DomainAbstractedStates DomainAbstraction::getSuccessors(const shared_ptr <DomainAbstractedState> &state) {
        /*
         * Returns a List of Abstract states (DomainAbstractedState) (representation of one state = vec<int>) that are the
         * possible successors of the given Abstract state. It uses a map to store already found successors, so that duplicated are
         * */
        map<long long, shared_ptr<DomainAbstractedState>> alreadyFound; // store Id's of already found successors

        // loop over all operators
        for (int operatorIndex = 0; operatorIndex < transition_system->get_num_operators(); operatorIndex++) {
            vector < FactPair > preconditionsForOperator = transition_system->get_precondition_assignments_for_operator(
                    operatorIndex);

            if (groupAssignmentFulfillsFacts(state->getGroupsAssignment(), preconditionsForOperator)) {
                vector < FactPair >
                postconditionsForOperator = transition_system->get_postcondition_assignments_for_operator(
                        operatorIndex);

                VariableGroupVector successorGroupMapping(state->getGroupsAssignment());
                //successorGroupMapping.reserve(currentState.size());
                for (const FactPair &fact: postconditionsForOperator) {
                    successorGroupMapping.at(fact.var) = getGroupForFact(fact);
                }

                long long abstractStateIndex = abstractStateLookupIndex(successorGroupMapping);
                int newGValue = state->getGValue() + operatorCosts[operatorIndex];
                // If we have not already found this successor -> add it to results
                if ((alreadyFound.find(abstractStateIndex) == alreadyFound.end())) {
                    shared_ptr<DomainAbstractedState> successorState = make_shared<DomainAbstractedState>(
                            successorGroupMapping,
                            abstractStateIndex);
                    // just set operator id, not parent yet, as this is not clear until expanded during search
                    successorState->set_operator_id(operatorIndex);
                    successorState->setGValue(newGValue);
                    alreadyFound.insert(
                            pair<long long, shared_ptr<DomainAbstractedState>>(abstractStateIndex, successorState));
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
        for (const auto &pair: alreadyFound) {
            successorList.push_back(pair.second);
        }

        return successorList;
    }

    DomainAbstractedStates DomainAbstraction::getPredecessors(const shared_ptr <DomainAbstractedState> &state) {
        /*
         * Get the predecessors in Abstract State Space of the given Abstract State. Therefor the Abstract
         * transition system must be calculated.
         *
         * */
        vector <shared_ptr<DomainAbstractedState>> predecessors;
        int num_variables = (int) originalTask.get_variables().size();
        vector<int> stateValues = state->getGroupsAssignment();

        // loop over operators and check if postconditions are fulfilled
        for (int operatorIndex = 0; operatorIndex < transition_system->get_num_operators(); operatorIndex++) {
            vector <pair<int, int>> postFacts = abstractOperatorPostconditions[operatorIndex];
            // If post-facts not fulfilled -> operator could not have lead to this state -> continue with next operator
            if (abstractStateFulfillsAbstractFacts(stateValues, postFacts)) {
                // precond-vars are fixed, overwritten by post-cond can be chosen arbitrarely others as in post state
                vector <vector<int>> consideredGroupsPerVar(num_variables, vector < int > ());
                // precond facts fxed
                for (auto &precondFact: abstractOperatorPreconditions[operatorIndex]) {
                    consideredGroupsPerVar[precondFact.first].push_back(precondFact.second);
                }
                // post but not in pre -> all groups
                for (auto &postFact: postFacts) {
                    if (consideredGroupsPerVar[postFact.first].empty()) {
                        int maxGroupNum = domainSizes[postFact.first] - 1;
                        vector<int> tmpVec;
                        for (int i = 0; i <= maxGroupNum; i++) {
                            consideredGroupsPerVar[postFact.first].push_back(i);
                        }
                    }
                }
                // add state-value for vars that are neither in pre nor post
                for (int varIndex = 0; varIndex < (int) consideredGroupsPerVar.size(); varIndex++) {
                    if (consideredGroupsPerVar[varIndex].empty()) {
                        consideredGroupsPerVar[varIndex].push_back(stateValues[varIndex]);
                    }
                }

                // Make DomainAbstractedState objects from found predecessor states
                for (auto &precondSate: groupCombinations(consideredGroupsPerVar)) {
                    // Ignore if we have a self-loop
                    if (precondSate == stateValues) {
                        continue;
                    }
                    shared_ptr<DomainAbstractedState> predecessorNode = make_shared<DomainAbstractedState>(precondSate,
                                                                                                           abstractStateLookupIndex(
                                                                                                                   precondSate));
                    predecessorNode->setGValue(
                            state->getGValue() + operatorCosts[operatorIndex]);
                    predecessors.push_back(predecessorNode);
                }
            }
        }
        return predecessors;
    }


    bool DomainAbstraction::abstractStateFulfillsAbstractFacts(vector<int> abstractState,
                                                               const vector <pair<int, int>> &abstractFacts) {
        for (auto &f: abstractFacts) {
            if (abstractState.at(f.first) != f.second) {
                return false;
            }
        }
        return true;
    }

    shared_ptr <DomainAbstractedState> DomainAbstraction::getInitialAbstractState() {
        /*
         * Uses concrete Initial state and current variable-group mapping to derive abstract state
         * */
        vector<int> concreteStateValues = concrete_initial_state.get_unpacked_values();
        // store group assignment for initial State
        vector<int> abstractStateGroups;
        abstractStateGroups.reserve(concreteStateValues.size());
        // loop over variables in concrete init state == all variables
        for (int varIndex = 0; varIndex < (int) concreteStateValues.size(); varIndex++) {
            // insert group number for variable x in the vector describing abstract
            abstractStateGroups.push_back(variableGroupVectors[varIndex][concreteStateValues.at(varIndex)]);
        }
        return make_shared<DomainAbstractedState>(abstractStateGroups, abstractStateLookupIndex(abstractStateGroups));
    }

    vector <shared_ptr<DomainAbstractedState>> DomainAbstraction::getAbstractGoalStates() {
        /*
         * Returns All possible Abstract goal states (fix goal-facts but vary group nums for remaining variables)
         * */
        vector <shared_ptr<DomainAbstractedState>> goalStates;
        vector <vector<int>> consideredGroupsPerVar(originalTask.get_variables().size(), vector < int > ());
        // loop through variables and check if goal group exists
        for (int varIndex = 0; varIndex < (int) originalTask.get_initial_state().size(); varIndex++) {
            // check if we have goal group for this variable
            int lookupVal = transition_system->lookup_value(goal_facts, varIndex);
            // if we found var we have a goal group
            if (lookupVal != -1) {
                int goalGroup = variableGroupVectors.at(varIndex).at(lookupVal);
                consideredGroupsPerVar.at(varIndex).push_back(goalGroup);
            } else {
                // else all groups for this variable can be goal states!
                int maxGroupNum = domainSizes.at(varIndex) - 1;
                assert(maxGroupNum >= 0);
                for (int i = 0; i <= maxGroupNum; i++) {
                    consideredGroupsPerVar.at(varIndex).push_back(i);
                }
            }
        }
        assert(consideredGroupsPerVar.size() == originalTask.get_variables().size());
        // Now construct the Goal states
        log << "ABSTR GOAL STATES: gen group combinations..." << endl;
        for (auto &goalState: groupCombinations(consideredGroupsPerVar)) {
            //log << goalState << endl;
            shared_ptr<DomainAbstractedState> goalSearchNode = make_shared<DomainAbstractedState>(goalState,
                                                                                                  abstractStateLookupIndex(
                                                                                                          goalState));
            goalSearchNode->setGValue(0);
            goalStates.push_back(goalSearchNode);
        }
        return goalStates;
    }


    void DomainAbstraction::generateAbstractTransitionSystem() {
        /*
         * Using the current Abstraction, the operators(their pre- and post-conditions) according to the abstract groups
         * are calculated. this makes the predecessor calculation more efficient.
         * */
        log << "generate operator pre/post conds in abstract space..." << endl;
        for (int op_index = 0; op_index < transition_system->get_num_operators(); op_index++) {
            vector < FactPair > preCondsReal = transition_system->get_precondition_assignments_for_operator(op_index);
            vector < FactPair > postCondsReal = transition_system->get_postcondition_assignments_for_operator(op_index);

            vector <pair<int, int>> tmpVecPre;
            vector <pair<int, int>> tmpVecPost;
            // after that FactPairs have value = group number for variable
            for (auto &preFact: preCondsReal) {
                tmpVecPre.emplace_back(preFact.var, getGroupForFact(preFact));
            }
            abstractOperatorPreconditions.push_back(tmpVecPre);

            for (auto &postFact: postCondsReal) {
                tmpVecPost.emplace_back(postFact.var, getGroupForFact(postFact));
            }


            abstractOperatorPostconditions.push_back(tmpVecPost);
        }
    }

    long long DomainAbstraction::getNumberOfAbstractStates() const {
        return numAbstractStates;
    }

    int DomainAbstraction::getDomainSize(int var) {
        return domainSizes[var];
    }

    void DomainAbstraction::preCalculateVariableGroupFacts() {
        /*
         * Is called every time reload() is executed. It calculates the facts for every group of every variable.
         * */
        VariablesProxy varProxy = originalTask.get_variables();
        int numVars = (int) varProxy.size();
        variableGroupFacts.clear();
        // loop over variables
        for (int varIndex = 0; varIndex < numVars; varIndex++) {
            // add group facts vector for variable -> init
            variableGroupFacts.push_back(vector<vector<FactPair>>(getDomainSize(varIndex), vector<FactPair>()));
            int varDomainSize = varProxy[varIndex].get_domain_size();
            for (int inDomainIndex = 0; inDomainIndex < varDomainSize; inDomainIndex++) {
                FactPair pair(varIndex, inDomainIndex);
                int group = variableGroupVectors[varIndex][inDomainIndex];
                variableGroupFacts[varIndex][group].push_back(pair);
            }
        }
        assert((int) variableGroupFacts.size() == numVars);
    }
}