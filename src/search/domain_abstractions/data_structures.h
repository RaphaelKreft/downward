#ifndef DOMAIN_ABSTRACTIONS_DATA_STRUCTURES_H
#define DOMAIN_ABSTRACTIONS_DATA_STRUCTURES_H

#include <iostream>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cassert>
#include <limits>

// make state from task_proxy class visible
class State;

struct FactPair;

namespace domain_abstractions {

    struct Transition;

    const int INF = std::numeric_limits<int>::max();

    using Trace = std::deque<Transition>;
    //using DomainAbstractedStates = std::vector<DomainAbstractedState *>;
    // same as in domain_abstracted_task.cc
    // Group mapping vectors
    using VariableGroupVector = std::vector<int>;
    using VariableGroupVectors = std::vector<VariableGroupVector>;

    struct Transition {
        int op_id;
        // Id of state that is the target (Can be real or abstract state)
        long long target_id;

        Transition(int op_id, long long target_id)
                : op_id(op_id),
                  target_id(target_id) {
        }

        bool operator==(const Transition &other) const {
            return op_id == other.op_id && target_id == other.target_id;
        }

        friend std::ostream &operator<<(std::ostream &os, const Transition &t) {
            return os << "[" << t.op_id << "," << t.target_id << "]";
        }
    };

    struct Flaw {
        std::shared_ptr<std::vector<int>> stateWhereFlawHappens; // from that we can later derive the abstract state
        std::shared_ptr<std::vector<FactPair>> missedFacts; // vector storing the facts for that the operation/goal flag would actually be applicable/true

        Flaw(const std::vector<int>& flawBaseState, const std::shared_ptr<std::vector<FactPair>>& missedF) {
            assert(!missedF->empty());
            stateWhereFlawHappens = std::make_shared<std::vector<int>>(flawBaseState);
            missedFacts = missedF;
        }

        std::vector<int> getStateWhereFlawHappensCopy() {
            std::vector<int> copyVec;
            for (int & i : *stateWhereFlawHappens) {
                copyVec.push_back(i);
            }
            return copyVec;
        }
    };
}

#endif
