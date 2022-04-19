#include "heuristic_basis.h"

#include <unordered_set>
#include "queue"

using namespace std;

namespace domain_abstractions {

    HeuristicBasis::HeuristicBasis(int max_time, utils::LogProxy &log, TaskProxy originalTask,
                                   string splitMethodSuggestion) :
            max_time(max_time), timer(max_time), log(log),
            transitionSystem(make_shared<TransitionSystem>(originalTask.get_operators(), originalTask)),
            domainSplitter(DomainSplitter(DomainSplitter::getEnumForString(splitMethodSuggestion))) {
    }

    void HeuristicBasis::construct(TaskProxy originalTask) {
        /*
         Constructs the heuristic in two steps:
            1. generate abstraction using CEGAR for DomainAbstractions
            2. calculate the heuristic values for that Abstraction
        */

        // CONSTRUCT ABSTRACTION
        abstraction = createAbstraction(originalTask);

        // CALCULATE HEURISTIC VALUES
        heuristicValues = calculateHeuristicValues();
    }

    int HeuristicBasis::getValue(State state) {
        /*
         Returns the heuristic value for a given State s. For that it is using the Stored heuristic values as well
         as the Domain Abstraction for mapping the state to an abstract one.
        */
        vector<int> correspondingAbstractState = abstraction->getGroupAssignmentsForConcreteState(
                state.get_unpacked_values());
        int hMapIndex = abstraction->abstractStateLookupIndex(correspondingAbstractState);
        return heuristicValues.at(hMapIndex);
    }

    unique_ptr<DomainAbstraction> HeuristicBasis::createAbstraction(TaskProxy originalTask) {
        /*
         * This Method contains the CEGAR Algorithm for Domain Abstractions that will create the
         * Abstraction for a given originalTask
         */
        // first create a trivial Abstraction
        unique_ptr<DomainAbstraction> currentAbstraction(cegarTrivialAbstraction(originalTask));
        while (not cegarShouldTerminate()) {
            shared_ptr<Trace> t = cegarFindOptimalTrace(move(currentAbstraction));
            if (!t) { // TODO unterscheidung empty = leerer plan und keine lösung gefunden
                // When no trace in Abstract space was found -> Task Unsolvable -> log and break
                if (log.is_at_least_normal()) {
                    log << "Abstract task is unsolvable. -> Normal task is unsolvable" << std::endl;
                }
                break;
            }
            shared_ptr<Flaw> f = cegarFindFlaw(t, originalTask);
            if (!f) {
                if (log.is_at_least_normal()) {
                    log << "Found concrete solution during refinement." << std::endl;
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
                log << "Time expired!" << std::endl;
            }
            return true;
        } else if (!utils::extra_memory_padding_is_reserved()) {
            if (log.is_at_least_normal()) {
                log << "Reached memory limit." << std::endl;
            }
            return true;
        }
        return false;
    }


    unique_ptr<DomainAbstraction> HeuristicBasis::cegarTrivialAbstraction(TaskProxy originalTask) {
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
        return unique_ptr<DomainAbstraction>(new DomainAbstraction(domains, originalTask, transitionSystem));
    }

    shared_ptr<Trace> HeuristicBasis::cegarFindOptimalTrace(unique_ptr<DomainAbstraction> currentAbstraction) {
        /*
         * Uses UniformCostSearch to find the best way through abstract space to goal. Return trace if found.
         * Else return nullptr -> Task is not solvable
         * */
        DomainAbstractedState *initialState = currentAbstraction->getInitialAbstractState();
        // comparable lambda expression to support priority queue
        auto cmp = [](DomainAbstractedState *left, DomainAbstractedState *right) {
            return (left->getGValue()) < (right->getGValue());
        };
        shared_ptr<priority_queue<DomainAbstractedState *, DomainAbstractedStates, decltype(cmp)>> openList =
                make_shared<priority_queue<DomainAbstractedState *, DomainAbstractedStates, decltype(cmp)>>(cmp);
        openList->push(initialState);
        unordered_set<int> closedList;

        while (!openList->empty()) {
            DomainAbstractedState *nextState = openList->top();
            openList->pop();
            int nextState_ID = nextState->get_id();
            if (closedList.find(nextState_ID) == closedList.end()) {
                closedList.insert(nextState_ID);
                for (DomainAbstractedState *successorNode: abstraction->getSuccessors(nextState)) {
                    successorNode->setParent(nextState);
                    if (abstraction->isGoal(successorNode)) {
                        return extractSolution(successorNode);
                    }
                    openList->push(successorNode);
                }
            }
        }
        return NULL;
    }

    shared_ptr<Flaw> HeuristicBasis::cegarFindFlaw(shared_ptr<Trace> trace, TaskProxy originalTask) {
        /*
         * Tries to apply trace in original task and returns a flaw if we cannot reach goal via this trace.
         * -> Precondition flaw and goal flaws
         * */
        vector<int> currState = originalTask.get_initial_state().get_unpacked_values();
        // follow trace
        while (!trace->empty()) {
            // get next transition candidate
            Transition nextTransition = trace->front();
            trace->pop_front();
            vector<FactPair> missedPreconditionFacts = transitionSystem->transitionApplicable(currState, nextTransition);
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

    void HeuristicBasis::cegarRefine(std::shared_ptr<Flaw> flaw,
                                     std::unique_ptr<DomainAbstraction> currentDomainAbstraction) {
        /*
         * Uses the splitter as well as the retrieved flaw to refine the abstraction
         * */
        VariableGroupVectors refinedAbstraction = domainSplitter.split(flaw, move(currentDomainAbstraction));
        // Update DomainAbstractionObject
        currentDomainAbstraction->reload(refinedAbstraction);
    }

    vector<int> HeuristicBasis::calculateHeuristicValues() {
        /*
         * Calculates the heuristic values by using Dijkstra Algorithm to calculate Distances from goal states
         * in the Abstract State Space induced by the created DomainAbstraction "abstraction". Therefor use real statespace
         * and convert to abstract one on fly(abstractions keep transitions). We can assume that there is only one goal state
         */
        // 1. create goal state and set corresponding abstract state h-value to 0
        vector<FactPair> goalFacts = abstraction->getGoalFacts();
        vector<int> abstractEquivalent = abstraction->getGroupAssignmentsForConcreteState(goalFacts);
        int index = abstraction->abstractStateLookupIndex(abstractEquivalent);
        heuristicValues.insert(heuristicValues.begin() + index, 0);
        // 2. create closed and open list
        DomainAbstractedState *initialState = new DomainAbstractedState(abstractEquivalent, index);
        initialState->setGValue(0);
        // comparable lambda expression to support priority queue
        auto cmp = [](DomainAbstractedState *left, DomainAbstractedState *right) {
            return (left->getGValue()) < (right->getGValue());
        };
        priority_queue<DomainAbstractedState *, DomainAbstractedStates, decltype(cmp)> openList(cmp);
        openList.push(initialState);
        unordered_set<int> closedList;
        // loop until all reachable states has been visited
        while (!openList.empty()) {
            DomainAbstractedState *nextState = openList.top();
            openList.pop();
            int nextState_ID = nextState->get_id();
            if (closedList.find(nextState_ID) == closedList.end()) {
                heuristicValues.insert(heuristicValues.begin() + nextState_ID, nextState->getGValue());
                closedList.insert(nextState_ID);
                // compute/get successors -> need incoming transitions therefor
                // TODO: is it enough to generate predecessors in real space? or do we need those in Abstract space -> Guess: no! we expand whole state space
                for (DomainAbstractedState *successorNode: abstraction->getSuccessors(nextState)) {
                    successorNode->setParent(nextState);
                    openList.push(successorNode);
                }
            }
        }

    }
}
