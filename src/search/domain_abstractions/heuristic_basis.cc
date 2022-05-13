#include "heuristic_basis.h"

#include <vector>
#include <memory>

using namespace std;

namespace domain_abstractions {
    static const int memory_padding_in_mb = 75;

    HeuristicBasis::HeuristicBasis(double max_time, utils::LogProxy &log, TaskProxy originalTask,
                                   const string& splitMethodSuggestion) :
            max_time(max_time), log(log),
            transitionSystem(make_shared<TransitionSystem>(originalTask.get_operators(), originalTask, log)),
            domainSplitter(DomainSplitter(DomainSplitter::getEnumForString(splitMethodSuggestion), log)), timer(max_time) {
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

    int HeuristicBasis::getValue(const State& state) {
        /*
         Returns the heuristic value for a given State s. For that it is using the Stored heuristic values as well
         as the Domain Abstraction for mapping the state to an abstract one.
        */
        vector<int> stateValues = state.get_unpacked_values();
        vector<int> correspondingAbstractState = abstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        long long hMapIndex = abstraction->abstractStateLookupIndex(correspondingAbstractState);
        if (heuristicValues.find(hMapIndex) == heuristicValues.end()) {
            // if we have not calculated before, calculate and store
            int h_val = calculateHValueOnTheFly(correspondingAbstractState, hMapIndex);
            heuristicValues.insert(pair<long long , int>(hMapIndex, h_val));
            return h_val;
        }
        log << "Access already stored hvals" << endl;
        return heuristicValues.at(hMapIndex);
    }

    shared_ptr <DomainAbstraction> HeuristicBasis::createAbstraction(TaskProxy originalTask) {
        /*
         * This Method contains the CEGAR Algorithm for Domain Abstractions that will create the
         * Abstraction for a given originalTask
         */
        // first create a trivial Abstraction
        if (log.is_at_least_normal()) {
            log << "CEGAR: create initial trivial abstraction.." << endl;
        }
        shared_ptr<DomainAbstraction> currentAbstraction = cegarTrivialAbstraction(originalTask);
        if (log.is_at_least_normal()) {
            log << "CEGAR: initial abstraction created, now run refinement loop..." << endl;
        }
        int rounds = 0;
        while (not cegarShouldTerminate()) {
            rounds++;
            //log << "CEGAR round " << rounds << ": --Current Abstraction--> " << currentAbstraction->getAbstractDomains() << endl;
            shared_ptr<Trace> t = cegarFindOptimalTrace(currentAbstraction);
            if (!t) {
                // When no trace in Abstract space was found -> Task Unsolvable -> log and break
                if (log.is_at_least_normal()) {
                    log << "CEGAR round " << rounds << ": Abstract task is unsolvable. -> Normal task is unsolvable" << endl;
                }
                break;
            }
            shared_ptr<Flaw> f = cegarFindFlaw(t, originalTask);
            if (!f) {
                if (log.is_at_least_normal()) {
                    log << "CEGAR round " << rounds << ": Found concrete solution during refinement!!!" << endl;
                    // maybe show solution??
                    for (auto &transition : *t) {
                        log << transition.op_id << ",";
                    }
                    log << endl;
                }
                break;
            }
            // If a Flaw has been found we need to refine the Abstraction
            cegarRefine(f, currentAbstraction);
        }
        return currentAbstraction;
    }

    bool HeuristicBasis::cegarShouldTerminate() {
        if (timer.is_expired()) {
            if (log.is_at_least_normal()) {
                log << "CEGAR Termination Check: Time expired!" << endl;
            }
            return true;
        } else if (!utils::extra_memory_padding_is_reserved()) {
            if (log.is_at_least_normal()) {
                log << "CEGAR Termination Check: Reached memory limit." << endl;
            }
            return true;
        }
        if (log.is_at_least_debug()) {
            log << "CEGAR Termination Check: Start another round of refinement! Time elapsed until now: " << timer.get_elapsed_time() << endl;
        }
        return false;
    }


    shared_ptr <DomainAbstraction> HeuristicBasis::cegarTrivialAbstraction(TaskProxy originalTask) {
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
        return make_shared<DomainAbstraction>(domains, log, originalTask, transitionSystem);
    }

    shared_ptr <Trace> HeuristicBasis::cegarFindOptimalTrace(const shared_ptr <DomainAbstraction>& currentAbstraction) {
        /*
         * Uses UniformCostSearch to find the best way through abstract space to goal. Return trace if found.
         * Else return nullptr -> Task is not solvable
         * */
        if (log.is_at_least_debug()) {
            log << "CEGAR Find Optimal Trace: try to find optimal trace in abstract state space..." << endl;
            log << currentAbstraction->getAbstractDomains() << endl;
        }
        shared_ptr<DomainAbstractedState> initialState = currentAbstraction->getInitialAbstractState();
        initialState->setGValue(0); // g-values must be set manually
        // PQ returns node with smallest g value -> Shortest Path until now
        priority_queue<shared_ptr<DomainAbstractedState>, DomainAbstractedStates, decltype(DomainAbstractedState::getComparator())> openList(
                DomainAbstractedState::getComparator());
        openList.push(initialState);
        unordered_set<long long> closedList;

        while (!openList.empty()) {
            shared_ptr<DomainAbstractedState> nextState = openList.top();
            openList.pop();
            long long nextState_ID = nextState->get_id();
            if (closedList.find(nextState_ID) == closedList.end()) {
                closedList.insert(nextState_ID);
                DomainAbstractedStates successorVector = currentAbstraction->getSuccessors(nextState);

                for (const auto& successorNode : successorVector) {
                    successorNode->setParent(nextState);
                    if (currentAbstraction->isGoal(successorNode)) {
                        if (log.is_at_least_debug()) {
                            log << "CEGAR Find Optimal Trace: Found Goal!! No extract solution trace..." << endl;
                        }
                        return DomainAbstractedState::extractSolution(successorNode);
                    }

                    openList.push(successorNode);
                }
            }
        }
        if (log.is_at_least_normal()) {
            log << "CEGAR Find Optimal Trace: NO SOLUTION found in Abstract space..." << endl;
        }
        return nullptr;
    }

    shared_ptr <Flaw> HeuristicBasis::cegarFindFlaw(const shared_ptr <Trace>& trace, TaskProxy originalTask) {
        /*
         * Tries to apply trace in original task and returns a flaw if we cannot reach goal via this trace.
         * -> Precondition flaw and goal flaws
         * */
        if (log.is_at_least_debug()) {
            log << "CEGAR Find Flaw: now try to find a flaw based on following trace (op-id, target-id).." << endl;
            for (auto & it : *trace)
                log << ' ' << it;
            log << endl;
        }
        vector<int> currState = originalTask.get_initial_state().get_unpacked_values();
        // follow trace
        //log << "Check if precondition flaw..." << endl;
        while (!(trace->empty())) {
            // get next transition candidate
            Transition nextTransition = trace->front();
            trace->pop_front();
            vector<FactPair> missedPreconditionFacts = transitionSystem->transitionApplicable(currState,
                                                                                              nextTransition);
            // when missed-facts is not empty we have a precondition flaw -> preconditions not fulfilled in curr  state
            if (!missedPreconditionFacts.empty()) {
                shared_ptr<vector<FactPair>> missedF(new vector<FactPair>(missedPreconditionFacts));
                //log << "--> Needed Fact Pairs would have been: " << transitionSystem->get_precondition_assignments_for_operator(nextTransition.op_id) << endl;
                //log << " --> Precondition Flaw at transition " << nextTransition << endl;
                return make_shared<Flaw>(currState, missedF);
            }
            // if we have no missed facts we can apply operator and continue to follow the trace
            currState = transitionSystem->applyOperator(currState, nextTransition.op_id);
        }
        // we get here when we followed trace without precondition flaw
        // now check for goal flaw
        //log << "check if goal violation..." << endl;
        vector<FactPair> missedGoalFacts = transitionSystem->isGoal(currState);
        //log << "received missed goal facts..." << endl;
        if (!missedGoalFacts.empty()) {
            //log << "--> Goal Fact violation Flaw!" << endl;
            shared_ptr<vector<FactPair>> missedGF(new vector<FactPair>(missedGoalFacts));
            return make_shared<Flaw>(currState, missedGF);
        }
        //log << "--> No Flaw!" << endl;
        return nullptr;
    }

    void HeuristicBasis::cegarRefine(const shared_ptr <Flaw>& flaw,
                                     const shared_ptr <DomainAbstraction>& currentDomainAbstraction) {
        /*
         * Uses the splitter as well as the retrieved flaw to refine the abstraction
         * */
        if (log.is_at_least_debug()) {
            log << "CEGAR Refine: refine abstraction on basis of found flaw.." << endl;
        }
        VariableGroupVectors refinedAbstraction = domainSplitter.split(flaw, currentDomainAbstraction);
        // Update DomainAbstractionObject
        //log << "CEGAR Refine: reload abstraction instance using refinement result Abstraction: " << refinedAbstraction << endl;
        currentDomainAbstraction->reload(refinedAbstraction);
    }

    int HeuristicBasis::calculateHValueOnTheFly(const VariableGroupVector& startStateValues, long long abstractStateIndex) {
        /*
         * Creates Abstract State Instance and returns distance to goal State (In Abstracted State Space) by using uniform cost search!
         * Intended to be used when no heuristic values are precomputed after Abstraction construction.
         *
         * Returns INFINITY(INF) when solution cannot be found!
         * */
        shared_ptr<DomainAbstractedState> startAState = make_shared<DomainAbstractedState>(startStateValues,
                                                                                           abstractStateIndex);
        startAState->setGValue(0);
        priority_queue<shared_ptr<DomainAbstractedState>, DomainAbstractedStates, decltype(DomainAbstractedState::getComparator())> openList(
                DomainAbstractedState::getComparator());
        openList.push(startAState);
        unordered_set<long long> closedList;

        while (!openList.empty()) {
            shared_ptr<DomainAbstractedState> nextState = openList.top();
            openList.pop();
            long long nextState_ID = nextState->get_id();
            if (closedList.find(nextState_ID) == closedList.end()) {
                closedList.insert(nextState_ID);
                DomainAbstractedStates successorVector = abstraction->getSuccessors(nextState);
                for (const auto& successorNode : successorVector) {
                    successorNode->setParent(nextState);
                    if (abstraction->isGoal(successorNode)) {
                        return successorNode->getGValue();
                    }
                    openList.push(successorNode);
                }
            }
        }
        // If no Solution found return INF
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
