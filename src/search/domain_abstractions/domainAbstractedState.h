#ifndef DOMAIN_ABSTRACTIONS_DOMAINABSTRACTEDSTATE_H
#define DOMAIN_ABSTRACTIONS_DOMAINABSTRACTEDSTATE_H

#include "data_structures.h"

#include <utility>
#include <vector>
#include <memory>
#include <functional>

namespace domain_abstractions {

    class DomainAbstractedState;

    using DomainAbstractedStates = std::vector<std::shared_ptr<DomainAbstractedState>>;

    class DomainAbstractedState {
        std::vector<int> groupAssignments; // for every var -> In which group inside dom(var) split are u?

        long long abstract_state_id;
        // search-info
        int incomingOperator_ID{};
        int g_value{};
        std::shared_ptr<DomainAbstractedState> parentAbstractState;
    public:
        DomainAbstractedState(std::vector<int> groupAssignments, long long ID);

        std::vector<int> getGroupsAssignment();

        long long get_id() const;

        int get_operator_id() const;

        void set_operator_id(int op_id);

        // searchInfo Part -> These Tasks can also be Nodes
        int getGValue() const;

        void setGValue(int new_value);

        void setParent(std::shared_ptr<DomainAbstractedState> parent);

        std::shared_ptr<DomainAbstractedState> getParent();

        static std::shared_ptr<Trace> extractSolution(std::shared_ptr<DomainAbstractedState> goal) {
            /*
             * When used as search nodes, this function can extract a trace out of DomainAbstractedStates
             * DO NOT INSERT INIT STATE! -> trace length = pathlength -1
             * */
            std::shared_ptr<Trace> abstractSolutionTrace = std::make_shared<Trace>();
            std::shared_ptr<DomainAbstractedState> currentState = std::move(goal);
            // now loop over search nodes and construct
            while (currentState->getParent() != nullptr) {
                abstractSolutionTrace->emplace_front(
                        Transition(currentState->get_operator_id(), currentState->get_id()));
                currentState = currentState->getParent();
                      // TODO: Here an error was located
            }
            return abstractSolutionTrace;
        }

        static std::function<bool(std::shared_ptr<DomainAbstractedState>, std::shared_ptr<DomainAbstractedState>)>
        getComparator() {
            std::function<bool(std::shared_ptr<DomainAbstractedState>,
                               std::shared_ptr<DomainAbstractedState>)> comparator = [](
                    const std::shared_ptr<DomainAbstractedState>& left, const std::shared_ptr<DomainAbstractedState>& right) {
                return (left->getGValue()) > (right->getGValue()); // TODO > < changed because weird c++ comparator logic
            };
            return comparator;
        }
    };
}

#endif
