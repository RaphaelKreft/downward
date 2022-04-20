#include "domainAbstractedState.h"
#include <cassert>

using namespace std;

namespace domain_abstractions {
    DomainAbstractedState::DomainAbstractedState(vector<int> groupAssignments, int ID) :
            groupAssignments(groupAssignments), abstract_state_id(ID) {
    }

    vector<int> DomainAbstractedState::getGroupsAssignment() {
        return groupAssignments;
    }

    int DomainAbstractedState::get_id() {
        return abstract_state_id;
    }

    int DomainAbstractedState::get_operator_id() {
        return incomingOperator_ID;
    }

    int DomainAbstractedState::getGValue() {
        assert(g_value);
        return g_value;
    }

    void DomainAbstractedState::setGValue(int new_value) {
        g_value = new_value;
    }

    void DomainAbstractedState::setParent(DomainAbstractedState *parent) {
        parentAbstractState = parent;
    }

    void DomainAbstractedState::set_operator_id(int op_id) {
        incomingOperator_ID = op_id;
    }

    DomainAbstractedState *DomainAbstractedState::getParent() {
        return parentAbstractState;
    }
}