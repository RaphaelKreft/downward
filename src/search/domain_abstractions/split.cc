#include "split.h"

#include <vector>
#include <cassert>
#include "da_utils.h"

using namespace std;

namespace domain_abstractions {

    DomainSplitter::DomainSplitter(const string &splitMethodString, const string &splitSelectorString,
                                   utils::LogProxy &log) : currentSplitMethod(
            getEnumForString(splitMethodString)), currentSplitSelector(getSplitSelectorForString(splitSelectorString)),
                                                           log(log) {
    }

    VariableGroupVectors
    DomainSplitter::split(const shared_ptr <Flaw> &flaw, const shared_ptr <DomainAbstraction> &currentAbstraction,
                          bool splitSingleRandomValue) {
        // Select method on how to split and call Sub-method. Can split all variables or choose one random! If we choose one fact to split, there are multiple methods on how to do that.
        if (currentSplitMethod == SplitMethod::SINGLEVALUESPLIT) {
            if (splitSingleRandomValue) {
                return performSingleValueSplitOneFact(flaw, currentAbstraction);
            }
            return performSingleValueSplit(flaw, currentAbstraction);
        } else if (currentSplitMethod == SplitMethod::RANDOMUNIFORMSPLIT) {
            if (splitSingleRandomValue) {
                return performRandomUniformSplitOneFact(flaw, currentAbstraction);
            }
            return performRandomUniformSplit(flaw, currentAbstraction);
        } else {
            // default fallback if unknown
            if (splitSingleRandomValue) {
                return performSingleValueSplitOneFact(flaw, currentAbstraction);
            }
            return performSingleValueSplit(flaw, currentAbstraction);
        }
    }

    VariableGroupVectors
    DomainSplitter::performSingleValueSplit(const shared_ptr <Flaw> &flaw,
                                            const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every variable (that has missed facts in their domain)
         * */
        VariableGroupVectors newAbstraction = currentAbstraction->getAbstractDomains();

        vector<int> stateValues = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        // we assume that there is max one missed fact for every variable -> in the formalism used in downward yes!
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        assert(!missedFacts->empty());
        // loop over variables and their domains
        for (auto factPair: *missedFacts) {
            int domainSize = currentAbstraction->getDomainSize(factPair.var);
            // just give missed-fact a new group
            newAbstraction[factPair.var][factPair.value] = domainSize;
        }
        return newAbstraction;
    }

    VariableGroupVectors
    DomainSplitter::performSingleValueSplitOneFact(const shared_ptr <Flaw> &flaw,
                                                   const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every variable (that has missed facts in their domain)
         * */
        VariableGroupVectors newAbstraction = currentAbstraction->getAbstractDomains();

        vector<int> stateValues = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        // choose one random missed fact
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        int splitFactIndex = selectSplitFact(flaw, currentAbstraction, currentSplitSelector);
        FactPair factPair = missedFacts->at(splitFactIndex);
        int domainSize = currentAbstraction->getDomainSize(factPair.var);
        // just give missed-fact a new group
        newAbstraction[factPair.var][factPair.value] = domainSize;

        return newAbstraction;
    }

    VariableGroupVectors DomainSplitter::performRandomUniformSplit(const shared_ptr <Flaw> &flaw,
                                                                   const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Even-Split will put the missed fact into a new group, together with half of its old group (randomly).
         * This is done for every variable that has indeed a missed fact in its domain.
         * */
        VariableGroupVectors newAbstraction = currentAbstraction->getAbstractDomains();

        vector<int> flawState = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                flawState);

        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;

        assert(!missedFacts->empty());
        // loop over Missed Facts
        for (auto factPair: *missedFacts) {
            // determine groupNr for new group == domainSize(== max group num + 1)
            int domainSize = currentAbstraction->getDomainSize(factPair.var);

            // position of missed fact inside variable domain, also change it immediately
            int oldGroupNr = newAbstraction[factPair.var][factPair.value];
            newAbstraction[factPair.var][factPair.value] = domainSize;
            assert(newAbstraction[factPair.var][factPair.value] != oldGroupNr);

            // change half of old group (-1)
            vector<FactPair> oldGroupFacts = currentAbstraction->getVariableGroupFacts(factPair.var,
                                                                                       oldGroupNr);
            // candidates for random change in group
            vector<int> choice_positions;
            int oldGroupSize = (int) oldGroupFacts.size();
            int toChange = (oldGroupSize - 1) / 2;
            for (auto oldGroupFact: oldGroupFacts) {
                if (oldGroupFact == factPair) {
                    continue;
                }
                choice_positions.push_back(oldGroupFact.value);
            }
            // now get random selection and change
            for (int idx: randomPickFromList(toChange, choice_positions)) {
                newAbstraction[factPair.var][idx] = domainSize;
            }
        }
        return newAbstraction;
    }

    VariableGroupVectors DomainSplitter::performRandomUniformSplitOneFact(const shared_ptr <Flaw> &flaw,
                                                                          const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Even-Split will put the missed fact into a new group, together with half of its old group (randomly).
         * This is done for every variable that has indeed a missed fact in its domain.
         * */
        VariableGroupVectors newAbstraction = currentAbstraction->getAbstractDomains();

        vector<int> flawState = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                flawState);

        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        int splitFactIndex = selectSplitFact(flaw, currentAbstraction, currentSplitSelector);
        // loop over variables and their domains
        FactPair factPair = missedFacts->at(splitFactIndex);
        // determine groupNr for new group == domainSize(== max group num + 1)
        int domainSize = currentAbstraction->getDomainSize(factPair.var);

        // position of missed fact inside variable domain, also change it immediately
        int oldGroupNr = newAbstraction[factPair.var][factPair.value];
        newAbstraction[factPair.var][factPair.value] = domainSize;
        assert(newAbstraction[factPair.var][factPair.value] != oldGroupNr);

        // change half of old group (-1)
        vector<FactPair> oldGroupFacts = currentAbstraction->getVariableGroupFacts(factPair.var,
                                                                                   oldGroupNr);
        // candidates for random change in group
        vector<int> choice_positions;
        int oldGroupSize = (int) oldGroupFacts.size();
        int toChange = (oldGroupSize - 1) / 2;
        for (auto oldGroupFact: oldGroupFacts) {
            if (oldGroupFact == factPair) {
                continue;
            }
            choice_positions.push_back(oldGroupFact.value);
        }
        // now get random selection and change
        for (int idx: randomPickFromList(toChange, choice_positions)) {
            newAbstraction[factPair.var][idx] = domainSize;
        }
        return newAbstraction;
    }

    int DomainSplitter::selectSplitFact(const shared_ptr <Flaw> &flaw,
                                        const shared_ptr <DomainAbstraction> &currentAbstraction,
                                        SplitSelector split_select) {
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;;
        // choose random fact
        if (split_select == SplitSelector::RANDOM) {
            int random_idx = randIntFromRange(0, (int) missedFacts->size() - 1);
            return random_idx;
        }
        // choose according to other criteria
        int currBestPairIndex = 0;
        if (split_select == SplitSelector::MIN_NEWSTATES) {
            int currBestNumNewStates = INF;
            for (int i = 0; i < (int) missedFacts->size(); i++) {
                int curr_domainSize = currentAbstraction->getDomainSize(missedFacts->at(i).var);
                int newNumAbstractStates = (curr_domainSize + 1) / curr_domainSize;
                if (newNumAbstractStates < currBestNumNewStates) {
                    currBestPairIndex = i;
                    currBestNumNewStates = newNumAbstractStates;
                }
            }
        } else if (split_select ==
                   SplitSelector::LEAST_REFINED) { // least refined mean we have the minimal num of groups in the domain and thus a higher gain of new#states/old#states ratio
            int currBestNumNewStates = 0;
            for (int i = 0; i < (int) missedFacts->size(); i++) {
                int curr_domainSize = currentAbstraction->getDomainSize(missedFacts->at(i).var);
                int newNumAbstractStates = (curr_domainSize + 1) / curr_domainSize;
                if (newNumAbstractStates > currBestNumNewStates) {
                    currBestPairIndex = i;
                    currBestNumNewStates = newNumAbstractStates;
                }
            }
        }

        return currBestPairIndex;
    }

}