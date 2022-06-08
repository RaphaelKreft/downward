#include "split.h"

#include <vector>
#include <cassert>
#include "da_utils.h"

using namespace std;

namespace domain_abstractions {

    DomainSplitter::DomainSplitter(const string &method, utils::LogProxy &log) : currentMethod(
            getEnumForString(method)), log(log) {
    }

    VariableGroupVectors
    DomainSplitter::split(const shared_ptr <Flaw> &flaw, const shared_ptr <DomainAbstraction> &currentAbstraction, bool splitSingleRandomValue) {
        // Select method on how to split and call Submethod. We return the new Abstraction
        //log << "Missed Facts: " << flaw->missedFacts->size() << endl;
        if (currentMethod == SplitMethod::SINGLEVALUESPLIT) {
            if (splitSingleRandomValue) {
                return performSingleValueSplitOneFact(flaw, currentAbstraction);
            }
            return performSingleValueSplit(flaw, currentAbstraction);
        } else if (currentMethod == SplitMethod::RANDOMUNIFORMSPLIT) {
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

        //vector<int> stateValues = flaw->getStateWhereFlawHappensCopy();
        vector<int> stateValues = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        // choose one random missed fact
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        int random_idx = randIntFromRange(0, (int) missedFacts->size() - 1);
        assert(random_idx < (int) missedFacts->size());
        assert(random_idx >= 0);
        // loop over variables and their domains
        FactPair factPair = missedFacts->at(random_idx);
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
        int random_idx = randIntFromRange(0, (int) missedFacts->size() - 1);
        assert(random_idx < (int) missedFacts->size());
        assert(random_idx >= 0);
        // loop over variables and their domains
        FactPair factPair = missedFacts->at(random_idx);
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
}