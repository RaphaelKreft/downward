#ifndef DOMAIN_ABSTRACTIONS_DATA_STRUCTURES_H
#define DOMAIN_ABSTRACTIONS_DATA_STRUCTURES_H

#include <iostream>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

// make state from task_proxy class visible
class State;

struct FactPair;

namespace domain_abstractions {

    struct Transition;

    const int INF = std::numeric_limits<int>::max();

    using Transitions = std::vector<Transition>;
    using Trace = std::deque<Transition>;
    using Solution = std::deque<State>;
    //using DomainAbstractedStates = std::vector<DomainAbstractedState *>;
    // same as in domain_abstracted_task.cc
    using ValueGroup = std::vector<int>;
    using ValueGroups = std::vector<ValueGroup>;
    using VarToGroups = std::unordered_map<int, ValueGroups>;
    // Group mapping vectors
    using VariableGroupVector = std::vector<int>;
    using VariableGroupVectors = std::vector<VariableGroupVector>;

    struct Transition {
        int op_id;
        // Id of state that is the target (Can be rel or abstract state
        int target_id;

        Transition(int op_id, int target_id)
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
        // TODO Members must have constant size or Flaw must be class
        std::vector<int> stateWhereFlawHappens; // from that we can later derive the abstract state
        std::vector<FactPair> missedFacts; // vector storing the facts for that the operation/goal flag would actually be applicable/true

        Flaw(std::vector<int> flawBaseState, std::vector<FactPair> missedFacts) : stateWhereFlawHappens(flawBaseState),
                                                                        missedFacts(missedFacts) {
        }
    };
}

#endif
