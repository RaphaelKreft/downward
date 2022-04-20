#ifndef DOMAIN_ABSTRACTIONS_DOMAINABSTRACTEDSTATE_H
#define DOMAIN_ABSTRACTIONS_DOMAINABSTRACTEDSTATE_H

#include "data_structures.h"
#include "split.h"

#include <vector>

namespace domain_abstractions {
    class DomainAbstractedState {
        std::vector<int> groupAssignments; // for every var -> In which group inside dom(var) split are u?

        int abstract_state_id;
        // search-info
        int incomingOperator_ID;
        int g_value;
        DomainAbstractedState *parentAbstractState;
    public:
        DomainAbstractedState(std::vector<int> groupAssignments, int ID);

        std::vector<int> getGroupsAssignment();

        int get_id();

        int get_operator_id();

        void set_operator_id(int op_id);

        // searchInfo Part -> These Tasks can also be Nodes
        int getGValue();

        void setGValue(int new_value);

        void setParent(DomainAbstractedState *parent);

        DomainAbstractedState *getParent();
    };

    static std::shared_ptr<Trace> extractSolution(DomainAbstractedState *goal) {
        /*
         * When used as search nodes, this function can extract a trace out of DomainAbstractedStates
         * */
        std::shared_ptr<Trace> abstractSolutionTrace = std::make_shared<Trace>();
        DomainAbstractedState *currentState = goal;
        abstractSolutionTrace->emplace_front(Transition(currentState->get_operator_id(), currentState->get_id()));
        while (currentState->getParent() != nullptr) {
            DomainAbstractedState *parent = currentState->getParent();
            abstractSolutionTrace->emplace_front(Transition(currentState->get_operator_id(), currentState->get_id()));
            currentState = parent;
        }
        return abstractSolutionTrace;
    }
}

#endif
