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
        long long numAbstractStates{};

        // Operators based on abstraction(group var)
        std::vector<std::vector<std::pair<int, int>>> abstractOperatorPreconditions;
        std::vector<std::vector<std::pair<int, int>>> abstractOperatorPostconditions;

    public:
        explicit DomainAbstraction(VariableGroupVectors domains, utils::LogProxy &log, TaskProxy originalTask, std::shared_ptr<TransitionSystem>);

        int reload(VariableGroupVectors newAbstraction);

        bool isGoal(const std::shared_ptr<DomainAbstractedState>& candidate);

        DomainAbstractedStates getSuccessors(const std::shared_ptr<DomainAbstractedState>& state);

        std::shared_ptr<DomainAbstractedState> getInitialAbstractState();

        bool
        groupAssignmentFulfillsFacts(std::vector<int> abstractCandidate, const std::vector<FactPair> &factsToFulfill);

        VariableGroupVector getGroupAssignmentsForConcreteState(std::vector<int> &stateValues); // State -> abstract state
        long long abstractStateLookupIndex(
                std::vector<int> &abstractStateRepresentation);// for given Assignments of a State, a position of Abstract state in hvalue map is returned

        VariableGroupVectors getAbstractDomains();

        int getDomainIndexOfVariableValue(int variable, int value);

        std::vector<FactPair> getVariableGroupFacts(int varIndex, int groupNumber);

        long long int getNumAbstractStates() const;

        std::vector<std::shared_ptr<DomainAbstractedState>> getAbstractGoalStates();

        DomainAbstractedStates getPredecessors(const std::shared_ptr<DomainAbstractedState> &state);

        int getGroupForFact(FactPair fact);

        void generateAbstractTransitionSystem();

    private:

        std::vector<long long> computeNValues(VariableGroupVectors newAbstraction);

        std::vector<std::vector<int>> groupCombinations(std::vector<std::vector<int>> groupNumbersPerVar) {
            // Init
            int numResultingStates = 1;
            std::vector<std::vector<int>::iterator> iterators;
            assert(groupNumbersPerVar.size() == originalTask.get_variables().size());

            int howManyVectors = (int) groupNumbersPerVar.size();
            for (auto & i : groupNumbersPerVar) {
                numResultingStates *= (int) i.size();
                iterators.push_back(i.begin());
            }

            assert(iterators.size() == groupNumbersPerVar.size());
            assert(howManyVectors == (int) groupNumbersPerVar.size());
            std::vector<std::vector<int>> resultingStates;
            resultingStates.reserve(numResultingStates);

            // Odometer to generate combinations
            while (iterators.at(0) != groupNumbersPerVar.at(0).end()) {
                // process the pointed-to elements
                std::vector<int> toInsert;
                for (int i = 0; i < howManyVectors; i++) {
                    toInsert.push_back(*(iterators[i]));
                }
                assert(toInsert.size() == groupNumbersPerVar.size());
                resultingStates.push_back(toInsert);
                // the following increments the "odometer" by 1
                ++iterators[howManyVectors - 1];
                for (int i = howManyVectors - 1; (i > 0) && (iterators[i] == groupNumbersPerVar[i].end()); --i) {
                    iterators[i] = groupNumbersPerVar[i].begin();
                    ++iterators[i - 1];
                }
            }
            return resultingStates;
        };

        static bool
        abstractStateFulfillsAbstractFacts(std::vector<int> abstractState, const std::vector<std::pair<int, int>> &abstractFacts);
    };
}


#endif
