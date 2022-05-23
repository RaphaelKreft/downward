#include "domainAbstractedState.h"

#include <cassert>
#include <utility>
#include <vector>
#include <memory>

using namespace std;

namespace domain_abstractions {
    DomainAbstractedState::DomainAbstractedState(vector<int> groupAssignments, long long ID) :
            groupAssignments(std::move(groupAssignments)), abstract_state_id(ID) {
    }

    vector<int> DomainAbstractedState::getGroupsAssignment() {
        return groupAssignments;
    }

    long long DomainAbstractedState::get_id() const {
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
}