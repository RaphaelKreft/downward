#include "domainAbstractedState.h"

#include <cassert>
#include <utility>
#include <vector>
#include <memory>

using namespace std;

namespace domain_abstractions {
    DomainAbstractedState::DomainAbstractedState(vector<int> groupAssignments, int ID) :
            groupAssignments(std::move(groupAssignments)), abstract_state_id(ID) {
    }

    vector<int> DomainAbstractedState::getGroupsAssignment() {
        return groupAssignments;
    }

    int DomainAbstractedState::get_id() const {
        return abstract_state_id;
    }

    int DomainAbstractedState::get_operator_id() const {
        return incomingOperator_ID;
    }

    int DomainAbstractedState::getGValue() const {
        assert(g_value >= 0);
        return g_value;
    }

    void DomainAbstractedState::setGValue(int new_value) {
        assert(new_value >= 0);
        g_value = new_value;
    }

    void DomainAbstractedState::setParent(shared_ptr <DomainAbstractedState> parent) {
        parentAbstractState = std::move(parent);
    }

    void DomainAbstractedState::set_operator_id(int op_id) {
        incomingOperator_ID = op_id;
    }

    shared_ptr <DomainAbstractedState> DomainAbstractedState::getParent() {
        return parentAbstractState;
    }

    shared_ptr <Trace> DomainAbstractedState::extractSolution(shared_ptr <DomainAbstractedState> goal) {
        /*
         * When used as search nodes, this function can extract a trace out of DomainAbstractedStates
         * DO NOT INSERT INIT STATE! -> trace length = pathlength -1
         * */
        shared_ptr<Trace> abstractSolutionTrace = make_shared<Trace>();
        shared_ptr<DomainAbstractedState> currentState = move(goal);
        // now loop over search nodes and construct
        while (currentState->getParent() != nullptr) {
            abstractSolutionTrace->emplace_front(
                    Transition(currentState->get_operator_id(), currentState->get_id()));
            currentState = currentState->getParent();
        }
        return abstractSolutionTrace;
    }

    function<bool(shared_ptr < DomainAbstractedState > , shared_ptr < DomainAbstractedState > )>
    DomainAbstractedState::getComparator() {
        function<bool(shared_ptr<DomainAbstractedState>,
                      shared_ptr<DomainAbstractedState>)> comparator = [](
                const shared_ptr<DomainAbstractedState> &left,
                const shared_ptr<DomainAbstractedState> &right) {
            return (left->getGValue()) >
                   (right->getGValue()); // TODO > < changed because weird c++ comparator logic
        };
        return comparator;
    }
}