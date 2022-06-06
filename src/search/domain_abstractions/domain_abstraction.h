#ifndef DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_H
#define DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_H

#include "../task_utils/task_properties.h"
#include "../task_proxy.h"
#include "../utils/logging.h"

#include "data_structures.h"
#include "domainAbstractedState.h"
#include "transition_system.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace domain_abstractions {

    class DomainAbstraction {
        const std::shared_ptr<TransitionSystem> transition_system;
        const State concrete_initial_state;
        const std::vector<FactPair> goal_facts;

        TaskProxy originalTask;
        utils::LogProxy &log;

        VariableGroupVectors variableGroupVectors; // group mapping
        std::vector<long long> nValuesForHash; // NValues need to compute perfect hash function for h-value lookup
        std::vector<int> operatorCosts; // for faster operator cost access
        std::vector<int> domainSizes; // store num groups/domain size per variable. Used to get rid of max_element-ops
        long long numAbstractStates{0};
        int max_states;

        // precalced for every reload for performance reasons
        std::vector<std::vector<std::vector<FactPair>>> variableGroupFacts; // for each variable, for each group the vector of FactPairs is stored
        std::vector<std::vector<int>> operatorsApplicablePerAbstractState;

        // Operators based on abstraction(group var)
        std::vector<std::vector<std::pair<int, int>>> abstractOperatorPreconditions;
        std::vector<std::vector<std::pair<int, int>>> abstractOperatorPostconditions;

    public:
        explicit DomainAbstraction(VariableGroupVectors domains, utils::LogProxy &log, TaskProxy originalTask,
                                   std::shared_ptr<TransitionSystem>, int max_states);

        int reload(VariableGroupVectors newAbstraction);

        bool isGoal(const std::shared_ptr<DomainAbstractedState> &candidate);

        DomainAbstractedStates getSuccessors(const std::shared_ptr<DomainAbstractedState> &state);

        std::shared_ptr<DomainAbstractedState> getInitialAbstractState();

        bool
        groupAssignmentFulfillsFacts(std::vector<int> abstractCandidate, const std::vector<FactPair> &factsToFulfill);

        VariableGroupVector
        getGroupAssignmentsForConcreteState(std::vector<int> &stateValues); // State -> abstract state
        long long abstractStateLookupIndex(
                std::vector<int> &abstractStateRepresentation);// for given Assignments of a State, a position of Abstract state in hvalue map is returned

        VariableGroupVectors getAbstractDomains();

        std::vector<FactPair> getVariableGroupFacts(int varIndex, int groupNumber);

        std::vector<std::shared_ptr<DomainAbstractedState>> getAbstractGoalStates();

        DomainAbstractedStates getPredecessors(const std::shared_ptr<DomainAbstractedState> &state);

        int getGroupForFact(FactPair fact);

        void generateAbstractTransitionSystem();

        long long getNumberOfAbstractStates() const;

        int getDomainSize(int var);

    private:

        void preCalculateVariableGroupFacts();

        std::vector<long long> computeNValues(VariableGroupVectors newAbstraction, std::vector<int> &newDomainSizes);

        static bool
        abstractStateFulfillsAbstractFacts(std::vector<int> abstractState,
                                           const std::vector<std::pair<int, int>> &abstractFacts);
    };
}


#endif
