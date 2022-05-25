#include "heuristic_basis.h"

#include <vector>
#include <memory>

using namespace std;

namespace domain_abstractions {
    static const int memory_padding_in_mb = 75;

    HeuristicBasis::HeuristicBasis(bool PRECALC, double max_time, int max_states, utils::LogProxy &log, TaskProxy originalTask,
                                   const string &splitMethodSuggestion) :
            OTF(!PRECALC), max_time(max_time), max_states(max_states),log(log),
            transitionSystem(make_shared<TransitionSystem>(originalTask.get_operators(), originalTask, log)),
            domainSplitter(DomainSplitter(splitMethodSuggestion, log)), timer(max_time) {
        // reserve memory padding, at this time timer is also already started!
        utils::reserve_extra_memory_padding(memory_padding_in_mb);
        terminationFlag = false;
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
            log << "#Abstract States: " << abstraction->getNumberOfAbstractStates() << endl;
        }
        // PRECOMPUTE HEURISTIC VALUES
        if (!OTF) {
            calculateHeuristicValues();
        }
    }

    int HeuristicBasis::getValue(const State &state) {
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
            if (OTF) {
                int h_val = calculateHValueOnTheFly(correspondingAbstractState, hMapIndex);
                heuristicValues.insert(pair<long long, int>(hMapIndex, h_val));
                return h_val;
            } else {
                return INF;
            }
        } else {
            return heuristicValues.at(hMapIndex);
        }
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
        log << "Initial Abstraction: " << currentAbstraction->getAbstractDomains() << endl;
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
                    log << "CEGAR round " << rounds << ": Abstract task is unsolvable. -> Normal task is unsolvable"
                        << endl;
                }
                break;
            }
            shared_ptr<Flaw> f = cegarFindFlaw(t);
            if (!f) {
                if (log.is_at_least_normal()) {
                    log << "CEGAR round " << rounds << ": Found concrete solution during refinement!!!" << endl;
                    // maybe show solution??
                    for (auto &transition: *t) {
                        log << transition.op_id << ",";
                    }
                    log << endl;
                }
                break;
            }
            // If a Flaw has been found we need to refine the Abstraction
            cegarRefine(f, currentAbstraction);
        }
        log << "#CEGAR Loop Iterations: " << rounds << endl;
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
        } else if (terminationFlag) { // set ture if abstraction too fine or max-states reached
            if (log.is_at_least_normal()) {
                log << "CEGAR Termination Check: Abstraction getting too fine, hash-index had Overflow." << endl;
            }
            return true;
        }
        if (log.is_at_least_debug()) {
            log << "CEGAR Termination Check: Start another round of refinement! Time elapsed until now: "
                << timer.get_elapsed_time() << endl;
        }
        return false;
    }


    shared_ptr <DomainAbstraction> HeuristicBasis::cegarTrivialAbstraction(TaskProxy originalTask) {
        /*
         * This Method takes the original task and Creates a Domain Abstraction Object where the domains are built on
         * basis of the goal variable values of first goal state. group num goal-fact-group  = 1, others = 0
         * */
        // create null vectors for all domains
        VariableGroupVectors nullInit;
        VariablesProxy vars = originalTask.get_variables();
        for (int varIndex = 0; varIndex < (int) vars.size(); varIndex++) {
            nullInit.push_back(vector<int>(vars[varIndex].get_domain_size(), 0));
        }
        // make artificial Flaw out of goal facts
        shared_ptr<Flaw> tmpFlaw = make_shared<Flaw>(vector<int>(),
                                                     make_shared<vector<FactPair>>(transitionSystem->getGoalFacts()));
        // create abstraction Instance
        shared_ptr<DomainAbstraction> initAbstraction = make_shared<DomainAbstraction>(nullInit, log, originalTask,
                                                                                       transitionSystem, max_states);
        // Split and reload instance
        VariableGroupVectors domains = domainSplitter.split(tmpFlaw, initAbstraction);
        initAbstraction->reload(domains);
        return initAbstraction;
    }

    shared_ptr <Trace> HeuristicBasis::cegarFindOptimalTrace(const shared_ptr <DomainAbstraction> &currentAbstraction) {
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
                if (currentAbstraction->isGoal(nextState)) {
                    if (log.is_at_least_debug()) {
                        log << "CEGAR Find Optimal Trace: Found Goal!! No extract solution trace..." << endl;
                    }
                    return DomainAbstractedState::extractSolution(nextState);
                }
                // generate successors and add them to openList
                DomainAbstractedStates successorVector = currentAbstraction->getSuccessors(nextState);
                for (const auto &successorNode: successorVector) {
                    successorNode->setParent(nextState);
                    openList.push(successorNode);
                }
            }
        }
        if (log.is_at_least_normal()) {
            log << "CEGAR Find Optimal Trace: NO SOLUTION found in Abstract space..." << endl;
        }
        return nullptr;
    }

    shared_ptr <Flaw> HeuristicBasis::cegarFindFlaw(const shared_ptr <Trace> &trace) {
        /*
         * Tries to apply trace in original task and returns a flaw if we cannot reach goal via this trace.
         * -> Precondition flaw and goal flaws
         * */
        if (log.is_at_least_debug()) {
            log << "CEGAR Find Flaw: now try to find a flaw based on following trace (op-id, target-id).." << endl;
            for (auto &it: *trace)
                log << ' ' << it;
            log << endl;
        }
        vector<int> currState = transitionSystem->getInitialState();
        // follow trace
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
            log << "--> Goal Fact violation Flaw!" << endl;
            shared_ptr<vector<FactPair>> missedGF(new vector<FactPair>(missedGoalFacts));
            return make_shared<Flaw>(currState, missedGF);
        }
        log << "--> No Flaw!" << endl;
        return nullptr;
    }

    void HeuristicBasis::cegarRefine(const shared_ptr <Flaw> &flaw,
                                     const shared_ptr <DomainAbstraction> &currentDomainAbstraction) {
        /*
         * Uses the splitter as well as the retrieved flaw to refine the abstraction
         * */
        if (log.is_at_least_debug()) {
            log << "CEGAR Refine: refine abstraction on basis of found flaw.." << endl;
        }
        VariableGroupVectors refinedAbstraction = domainSplitter.split(flaw, currentDomainAbstraction);
        // Update DomainAbstractionObject if internal validity constraints are fulfilled, else keep old and set termination
        if (currentDomainAbstraction->reload(refinedAbstraction) == -1) {
            terminationFlag = true;
        }
    }

    int
    HeuristicBasis::calculateHValueOnTheFly(const VariableGroupVector &startStateValues, long long abstractStateIndex) {
        /*
         * Creates Abstract State Instance and returns distance to goal State (In Abstracted State Space) by using uniform cost search!
         * Intended to be used when no heuristic values are precomputed after Abstraction construction.
         *
         * Returns INFINITY(INF) when solution cannot be found!
         * */
        shared_ptr<DomainAbstractedState> startAState = make_shared<DomainAbstractedState>(startStateValues,
                                                                                           abstractStateIndex);
        startAState->setGValue(0);
        // Define Open and Closed List
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
                if (abstraction->isGoal(nextState)) {
                    return nextState->getGValue();
                }
                // We are using early goal check, thus the successor vector must be sorted!!!
                DomainAbstractedStates successorVector = abstraction->getSuccessors(nextState);
                for (const auto &successorNode: successorVector) {
                    openList.push(successorNode);
                }
            }
        }
        // If no Solution found return INF
        if (log.is_at_least_debug()) {
            log << "OTF-HVAL-Calculation: Cannot find path to goal, return INF.." << endl;
        }
        return INF;
    }

    void HeuristicBasis::calculateHeuristicValues() {
        /*
         * Calculates the heuristic values by using Dijkstra Algorithm to calculate Distances from goal states
         * in the Abstract State Space induced by the created DomainAbstraction "abstraction". Therefor use real statespace
         * and convert to abstract one on fly(abstractions keep transitions). We can assume that there is only one goal state
         */

        // perform backward-Search from Goal using Dijkstras Algorithm
        priority_queue<shared_ptr<DomainAbstractedState>, DomainAbstractedStates, decltype(DomainAbstractedState::getComparator())> openList(
                DomainAbstractedState::getComparator());
        log << "Now generate Abstract Goal States.." << endl;
        // 1. Create All possible goal states (In Abstract State Space) and add them to openList
        for (const auto &goalState: abstraction->getAbstractGoalStates()) {
            openList.push(goalState);
        }
        log << "Now generate operators for abstract space..." << endl;
        abstraction->generateAbstractTransitionSystem();
        // 2. Run Dijkstras Algorithm for backward search from every goal
        log << "Now run backward search..." << endl;
        while (!openList.empty()) {
            shared_ptr<DomainAbstractedState> nextState = openList.top();
            openList.pop();

            heuristicValues.insert(pair<long long, int>(nextState->get_id(), nextState->getGValue()));
            for (const auto &predecessor: abstraction->getPredecessors(nextState)) {
                // if not in H-values already or we have found a shorter way than we have currently in heuristic values add to openList
                if (heuristicValues.find(predecessor->get_id()) == heuristicValues.end() ||
                    predecessor->getGValue() < heuristicValues.at(predecessor->get_id())) {
                    openList.push(predecessor);
                }
            }

        }
    }
}
