#include "heuristic_basis.h"

#include <vector>
#include <memory>

using namespace std;

namespace domain_abstractions {
    static const int memory_padding_in_mb = 75;

    HeuristicBasis::HeuristicBasis(double max_time, utils::LogProxy &log, TaskProxy originalTask,
                                   string splitMethodSuggestion) :
            max_time(max_time), log(log),
            transitionSystem(make_shared<TransitionSystem>(originalTask.get_operators(), originalTask)),
            domainSplitter(DomainSplitter(DomainSplitter::getEnumForString(splitMethodSuggestion))), timer(max_time) {
        // reserve memory padding, at this time timer is also already started!
        utils::reserve_extra_memory_padding(memory_padding_in_mb);
    }

    void HeuristicBasis::construct(TaskProxy originalTask) {
        /*
         Constructs the heuristic in two steps:
            1. generate abstraction using CEGAR for DomainAbstractions
            2. calculate the heuristic values for that Abstraction
        */

        // CONSTRUCT ABSTRACTION
        if (log.is_at_least_normal()) {
            log << "Now construct abstraction..." << endl;
        }
        abstraction = createAbstraction(originalTask);

        if (log.is_at_least_normal()) {
            log << "Abstraction Construction finished!" << endl;
            log << "Final Abstraction: " << abstraction->getAbstractDomains() << endl;
        }
        // PRECOMPUTE HEURISTIC VALUES TODO: For now disabled as we first test with on the fly calculation
        //heuristicValues = calculateHeuristicValues();
    }

    int HeuristicBasis::getValue(State state) {
        /*
         Returns the heuristic value for a given State s. For that it is using the Stored heuristic values as well
         as the Domain Abstraction for mapping the state to an abstract one.
        */
        vector<int> stateValues = state.get_unpacked_values();
        vector<int> correspondingAbstractState = abstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        int hMapIndex = abstraction->abstractStateLookupIndex(correspondingAbstractState);
        //return heuristicValues.at(hMapIndex); TODO also disabled due to on the fly computation test
        return calculateHValueOnTheFly(correspondingAbstractState, hMapIndex);
    }

    unique_ptr <DomainAbstraction> HeuristicBasis::createAbstraction(TaskProxy originalTask) {
        /*
         * This Method contains the CEGAR Algorithm for Domain Abstractions that will create the
         * Abstraction for a given originalTask
         */
        // first create a trivial Abstraction
        if (log.is_at_least_normal()) {
            log << "CEGAR: create initial trivial abstraction.." << endl;
        }
        unique_ptr<DomainAbstraction> currentAbstraction(cegarTrivialAbstraction(originalTask));
        if (log.is_at_least_normal()) {
            log << "CEGAR: initial abstraction created, now run refinement loop..." << endl;
        }
        while (not cegarShouldTerminate()) {
            log << "CEGAR: Current Abstraction is: " << currentAbstraction->getAbstractDomains() << endl;
            shared_ptr<Trace> t = cegarFindOptimalTrace(move(currentAbstraction));
            if (!t) {
                // When no trace in Abstract space was found -> Task Unsolvable -> log and break
                if (log.is_at_least_normal()) {
                    log << "CEGAR: Abstract task is unsolvable. -> Normal task is unsolvable" << endl;
                }
                break;
            }
            shared_ptr<Flaw> f = cegarFindFlaw(t, originalTask);
            if (!f) {
                if (log.is_at_least_normal()) {
                    log << "CEGAR: Found concrete solution during refinement!!!" << endl;
                    // maybe show solution??
                    //log << cegarExtractPath(t, originalTask);
                }
                break;
            }
            // If a Flaw has been found we need to refine the Abstraction
            cegarRefine(f, move(currentAbstraction));
        }
        return currentAbstraction;
    }

    bool HeuristicBasis::cegarShouldTerminate() {
        if (timer.is_expired()) {
            if (log.is_at_least_normal()) {
                log << "CEGAR: Time expired!" << endl;
            }
            return true;
        } else if (!utils::extra_memory_padding_is_reserved()) {
            if (log.is_at_least_normal()) {
                log << "CEGAR: Reached memory limit." << endl;
            }
            return true;
        }
        if (log.is_at_least_normal()) {
            log << "CEGAR: Start another round of refinement! Time elapsed until now: " << timer.get_elapsed_time() << endl;
        }
        return false;
    }


    unique_ptr <DomainAbstraction> HeuristicBasis::cegarTrivialAbstraction(TaskProxy originalTask) {
        /*
         * This Method takes the original task and Creates a Domain Abstraction Object where the domains are built on
         * basis of the goal variable values of first goal state. group num goal-fact-group  = 1, others = 0
         * */
        VariableGroupVectors domains;
        // loop over every variable and create domain split
        for (VariableProxy var: originalTask.get_variables()) {
            VariableGroupVector varVec;
            varVec.clear();
            // loop over variable values and check for every one of it if it is contained in goal state
            for (int i = 0; i < var.get_domain_size(); i++) {
                FactProxy fact = var.get_fact(i);
                bool isGoal = false;
                // Check whether variable value is in goal facts
                for (FactProxy goalFact: originalTask.get_goals()) {
                    if (goalFact == fact) {
                        varVec.push_back(1);
                        isGoal = true;
                        break;
                    }
                }
                if (!isGoal) {
                    varVec.push_back(0);
                }
            }
            // Add domain group mapping for single variable to list
            domains.push_back(varVec);
        }
        return unique_ptr<DomainAbstraction>(new DomainAbstraction(domains, log, originalTask, transitionSystem));
    }

    shared_ptr <Trace> HeuristicBasis::cegarFindOptimalTrace(unique_ptr <DomainAbstraction> currentAbstraction) {
        /*
         * Uses UniformCostSearch to find the best way through abstract space to goal. Return trace if found.
         * Else return nullptr -> Task is not solvable
         * */
        if (log.is_at_least_normal()) {
            log << "CEGAR: try to find optimal trace in abstract state space..." << endl;
        }
        if (log.is_at_least_normal()) {
            log << "CEGAR: -- get initial abstract state and init priority queue..." << endl;
        }
        shared_ptr<DomainAbstractedState> initialState = currentAbstraction->getInitialAbstractState();
        priority_queue<shared_ptr<DomainAbstractedState>, DomainAbstractedStates, decltype(DomainAbstractedState::getComparator())> openList(
                DomainAbstractedState::getComparator());
        if (log.is_at_least_normal()) {
            log << "CEGAR: -- push initial state in openList and create closed list..." << endl;
        }
        openList.push(initialState);
        unordered_set<int> closedList;
        if (log.is_at_least_normal()) {
            log << "CEGAR: -- enter search loop..." << endl;
        }
        while (!openList.empty()) {
            shared_ptr<DomainAbstractedState> nextState = openList.top();
            openList.pop();
            int nextState_ID = nextState->get_id();
            if (log.is_at_least_normal()) {
                log << "CEGAR: -- check if in closed list already..." << endl;
            }
            if (closedList.find(nextState_ID) == closedList.end()) {
                if (log.is_at_least_normal()) {
                    log << "CEGAR: -- not in closed list, insert in closed list.." << endl;
                }
                closedList.insert(nextState_ID);
                DomainAbstractedStates successorVector = currentAbstraction->getSuccessors(nextState);
                if (log.is_at_least_normal()) {
                    log << "CEGAR: -- now loop over " << successorVector.size() << " successors..." << endl;
                }
                for (int successorIndex = 0; successorIndex < (int) successorVector.size(); successorIndex++) {
                    shared_ptr<DomainAbstractedState> successorNode = successorVector.at(successorIndex);
                    if (log.is_at_least_normal()) {
                        log << "CEGAR: ---succ: set parent and perform goal check..." << endl;
                    }
                    successorNode->setParent(nextState);
                    if (currentAbstraction->isGoal(successorNode)) {
                        if (log.is_at_least_normal()) {
                            log << "CEGAR: ---succ: Found Goal!! No extract solution trace..." << endl;
                        }
                        return DomainAbstractedState::extractSolution(successorNode);
                    }
                    if (log.is_at_least_normal()) {
                        log << "CEGAR: ---succ: successor is no goal, add to open List" << endl;
                    }
                    openList.push(successorNode);
                }
            }
        }
        if (log.is_at_least_normal()) {
            log << "CEGAR: -- No solution found in Abstract space..." << endl;
        }
        return nullptr;
    }

    shared_ptr <Flaw> HeuristicBasis::cegarFindFlaw(shared_ptr <Trace> trace, TaskProxy originalTask) {
        /*
         * Tries to apply trace in original task and returns a flaw if we cannot reach goal via this trace.
         * -> Precondition flaw and goal flaws
         * */
        if (log.is_at_least_normal()) {
            log << "CEGAR: now try to find a flaw based on following trace (op-id, target-id).." << endl;
            for (auto it = trace->begin(); it != trace->end(); ++it)
                log << ' ' << *it;
            log << endl;
        }
        vector<int> currState = originalTask.get_initial_state().get_unpacked_values();
        // follow trace
        while (!trace->empty()) {
            // get next transition candidate
            Transition nextTransition = trace->front();
            trace->pop_front();
            vector<FactPair> missedPreconditionFacts = transitionSystem->transitionApplicable(currState,
                                                                                              nextTransition);
            // when missed-facts is not empty we have a precondition flaw -> preconditions not fulfilled in curr  state
            if (!missedPreconditionFacts.empty()) {
                return make_shared<Flaw>(currState, missedPreconditionFacts);
            }
            // if we have no missed facts we can apply operator and continue to follow the trace
            currState = transitionSystem->applyOperator(currState, nextTransition.op_id);
        }
        // we get here when we followed trace without precondition flaw
        // now check for goal flaw
        vector<FactPair> missedGoalFacts = transitionSystem->isGoal(currState);
        if (!missedGoalFacts.empty()) {
            return make_shared<Flaw>(currState, missedGoalFacts);
        }
        return nullptr;
    }

    void HeuristicBasis::cegarRefine(shared_ptr <Flaw> flaw,
                                     unique_ptr <DomainAbstraction> currentDomainAbstraction) {
        /*
         * Uses the splitter as well as the retrieved flaw to refine the abstraction
         * */
        if (log.is_at_least_normal()) {
            log << "CEGAR: refine abstraction on basis of found flaw.." << endl;
        }
        VariableGroupVectors refinedAbstraction = domainSplitter.split(flaw, move(currentDomainAbstraction));
        // Update DomainAbstractionObject
        currentDomainAbstraction->reload(refinedAbstraction);
    }

    int HeuristicBasis::calculateHValueOnTheFly(VariableGroupVector startStateValues, int abstractStateIndex) {
        /*
         * Creates Abstract State Instance and returns distance to goal State (In Abstracted State Space) by using uniform cost search!
         * Intended to be used when no heuristic values are precomputed after Abstraction construction.
         *
         * Returns INFINITY(INF) when solution cannot be found!
         * */
        shared_ptr<DomainAbstractedState> startAState = make_shared<DomainAbstractedState>(startStateValues,
                                                                                           abstractStateIndex);

        priority_queue<shared_ptr<DomainAbstractedState>, DomainAbstractedStates, decltype(DomainAbstractedState::getComparator())> openList(
                DomainAbstractedState::getComparator());
        openList.push(startAState);
        unordered_set<int> closedList;
        while (!openList.empty()) {
            shared_ptr<DomainAbstractedState> nextState = openList.top();
            openList.pop();
            int nextState_ID = nextState->get_id();
            if (closedList.find(nextState_ID) == closedList.end()) {
                closedList.insert(nextState_ID);
                for (shared_ptr<DomainAbstractedState> successorNode: abstraction->getSuccessors(nextState)) {
                    successorNode->setParent(nextState);
                    if (abstraction->isGoal(successorNode)) {
                        return successorNode->getGValue();
                    }
                    openList.push(successorNode);
                }
            }
        }
        return INF;
    }

    vector<int> HeuristicBasis::calculateHeuristicValues() {
        /*
         * Calculates the heuristic values by using Dijkstra Algorithm to calculate Distances from goal states
         * in the Abstract State Space induced by the created DomainAbstraction "abstraction". Therefor use real statespace
         * and convert to abstract one on fly(abstractions keep transitions). We can assume that there is only one goal state
         */
        vector<int> newHeuristicValues;
        return newHeuristicValues;
    }
}
