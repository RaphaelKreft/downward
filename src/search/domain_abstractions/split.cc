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
    DomainSplitter::split(const shared_ptr <Flaw> &flaw, const shared_ptr <DomainAbstraction> &currentAbstraction) {
        // Select method on how to split and call Submethod. We return the new Abstraction
        if (currentMethod == SplitMethod::SINGLEVALUESPLIT) {
            return performSingleValueSplit(flaw, currentAbstraction);
        } else if (currentMethod == SplitMethod::RANDOMUNIFORMSPLIT) {
            return performRandomUniformSplit(flaw, currentAbstraction);
        } else {
            // default fallback if unknown
            //log << "CEGAR -- Split: using fallback Method(Hard-Split) since given method not specified!" << endl;
            return performSingleValueSplit(flaw, currentAbstraction);
        }
    }

    VariableGroupVectors
    DomainSplitter::performSingleValueSplit(const shared_ptr <Flaw> &flaw,
                                            const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every variable (that has missed facts in their domain)
         * */
        VariableGroupVectors oldAbstraction = currentAbstraction->getAbstractDomains();
        VariableGroupVectors newAbstraction = oldAbstraction;

        //vector<int> stateValues = flaw->getStateWhereFlawHappensCopy();
        vector<int> stateValues = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        // we assume that there is max one missed fact for every variable -> in the formalism used in downward yes!
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        assert(!missedFacts->empty());
        // loop over variables and their domains
        for (auto factPair: *missedFacts) {
            VariableGroupVector oldVariableAbstractDomain = oldAbstraction.at(factPair.var);
            VariableGroupVector newVariableAbstractDomain = oldVariableAbstractDomain;
            int domainSize = currentAbstraction->getDomainSize(factPair.var);
            // just give missed-fact a new group
            newVariableAbstractDomain[factPair.value] = domainSize;
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        assert(newAbstraction.size() == oldAbstraction.size());
        return newAbstraction;
    }

    VariableGroupVectors DomainSplitter::performRandomUniformSplit(const shared_ptr <Flaw> &flaw,
                                                                   const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Even-Split will put the missed fact into a new group, together with half of its old group (randomly).
         * This is done for every variable that has indeed a missed fact in its domain.
         * */
        VariableGroupVectors oldAbstraction = currentAbstraction->getAbstractDomains();
        VariableGroupVectors newAbstraction = oldAbstraction;

        vector<int> flawState = *(flaw->stateWhereFlawHappens);
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                flawState);

        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;

        assert(!missedFacts->empty());
        // loop over Missed Facts
        for (auto factPair: *missedFacts) {
            // Get and Copy domain abstraction of affected variable
            VariableGroupVector oldVariableAbstractDomain = oldAbstraction[factPair.var];
            VariableGroupVector newVariableAbstractDomain = oldVariableAbstractDomain;
            // determine groupNr for new group == domainSize(== max group num + 1)
            int domainSize = currentAbstraction->getDomainSize(factPair.var);

            // position of missed fact inside variable domain, also change it immediately
            newVariableAbstractDomain[factPair.value] = domainSize;

            // change half of old group (-1)
            int oldGroupNr = oldVariableAbstractDomain[factPair.value];
            vector<FactPair> oldGroupFacts = currentAbstraction->getVariableGroupFacts(factPair.var,
                                                                                                oldGroupNr);
            // candidates for random change in group
            vector<int> choice_positions;
            int oldGroupSize = (int) oldGroupFacts.size();
            int toChange = oldGroupSize / 2;
            for (auto oldGroupFact: oldGroupFacts) {
                if (oldGroupFact == factPair) {
                    continue;
                }
                choice_positions.push_back(oldGroupFact.value);
            }
            // now get random selection and change
            for (int idx: randomPickFromList(toChange, choice_positions)) {
                newVariableAbstractDomain[idx] = domainSize;
            }
            // Write new domain abstraction for variable to result (Due to downward-formalism just one missed fact per variable can exist)
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        assert(newAbstraction.size() == oldAbstraction.size());
        return newAbstraction;
    }
}