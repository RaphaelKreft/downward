#ifndef DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_H
#define DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_H

#include "../task_utils/task_properties.h"
#include "../task_proxy.h"

#include "data_structures.h"
#include "domainAbstractedState.h"
#include "transition_system.h"

#include <unordered_map>


namespace domain_abstractions {

    class DomainAbstraction {
        const std::shared_ptr<TransitionSystem> transition_system;
        const State concrete_initial_state;
        const std::vector<FactPair> goal_facts;
        TaskProxy originalTask;

        VariableGroupVectors variableGroupVectors; // group mapping
        std::vector<int> nValuesForHash; // NValues need to compute perfect hash function for h-value lookup

    public:
        explicit DomainAbstraction(VariableGroupVectors domains, TaskProxy originalTask, std::shared_ptr<TransitionSystem>);

        void reload(VariableGroupVectors newAbstraction);

        bool isGoal(DomainAbstractedState *candidate);

        DomainAbstractedStates getSuccessors(DomainAbstractedState *state);

        DomainAbstractedState *getInitialAbstractState();

        bool
        groupAssignmentFulfillsFacts(const std::vector<int> &abstractCandidate, const std::vector<FactPair> &existingFactPairs);

        VariableGroupVector getGroupAssignmentsForConcreteState(std::vector<int> stateValues); // State -> abstract state
        int abstractStateLookupIndex(
                std::vector<int> abstractStateRepresentation);// for given Assignments of a State, a position of Abstract state in hvalue map is returned
        const std::vector<FactPair> getGoalFacts();

        VariableGroupVectors getAbstractDomains();

        int getDomainIndexOfVariableValue(int variable, int value);

        TaskProxy getOriginalTask() {
            return originalTask;
        };
    private:
        std::vector<FactPair> getVariableGroupFacts(int varIndex, int groupNumber);

        int getGroupForFact(FactPair fact);

        std::vector<int> computeNValues();
    };
}


#endif
