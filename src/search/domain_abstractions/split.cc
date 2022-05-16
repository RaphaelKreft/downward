#include "split.h"
#include "data_structures.h"

#include <vector>
#include <cassert>

using namespace std;

namespace domain_abstractions {

    DomainSplitter::DomainSplitter(SplitMethod method, utils::LogProxy &log) : currentMethod(method), log(log) {
    }

    VariableGroupVectors
    DomainSplitter::split(const shared_ptr<Flaw>& flaw, const shared_ptr<DomainAbstraction>& currentAbstraction) {
        // Select method on how to split and call Submethod. We return the new Abstraction
        if (currentMethod == SplitMethod::HARDSPLIT) {
            log << "CEGAR -- Split: using HARD-split!" << endl;
            return performHardSplit(flaw, currentAbstraction);
        } else if (currentMethod == SplitMethod::EVENSPLIT) {
            log << "CEGAR -- Split: using EVEN-split!" << endl;
            return performEvenSplit(flaw, currentAbstraction);
        }else {
            // default fallback if unknown
            log << "CEGAR -- Split: using fallback Method(Hard-Split) since given method not specified!" << endl;
            return performHardSplit(flaw, currentAbstraction);
        }
    }

    VariableGroupVectors
    DomainSplitter::performHardSplit(const shared_ptr<Flaw>& flaw, const shared_ptr<DomainAbstraction>& currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every variable (that has missed facts in their domain)
         * */
        VariableGroupVectors oldAbstraction = currentAbstraction->getAbstractDomains();
        VariableGroupVectors newAbstraction = oldAbstraction;

        vector<int> stateValues = flaw->getStateWhereFlawHappensCopy();
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);
        // TODO: we assume that there is max one missed fact for every variable!? -> YES in the formalism used in downward yes!
        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        if (log.is_at_least_debug()) {
            log << "--Missed facts: " << *missedFacts << endl;
            log << "Real State where flaw happened: " << stateValues << endl;
            log << "--Abstract State Where Flaw happened: " << abstractStateWhereFlawHappened << endl;
            for (const auto& factPair: *missedFacts) {
                log << "Group Facts for missed var(in real space)" << factPair.var << ": "<< currentAbstraction->getVariableGroupFacts(factPair.var, abstractStateWhereFlawHappened.at(factPair.var)) << endl;
            }
        }
        assert(!missedFacts->empty());
        // loop over variables and their domains
        for (auto factPair : *missedFacts) {
            VariableGroupVector oldVariableAbstractDomain = oldAbstraction.at(factPair.var);
            VariableGroupVector newVariableAbstractDomain = oldVariableAbstractDomain;
            int maxGroupNumber = *max_element(oldVariableAbstractDomain.begin(), oldVariableAbstractDomain.end());
            assert(maxGroupNumber <= (int)oldVariableAbstractDomain.size());
            // just give missed-fact a new group TODO: if check is already in separate group flaw should not have been happened!
            //int domainIndex = currentAbstraction->getDomainIndexOfVariableValue(factPair.var, factPair.value);
            //assert(domainIndex == factPair.value);
            newVariableAbstractDomain[factPair.value] = maxGroupNumber + 1;
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        assert(newAbstraction.size() == oldAbstraction.size());
        return newAbstraction;
    }

    VariableGroupVectors DomainSplitter::performEvenSplit(const shared_ptr <Flaw> &flaw,
                                                          const shared_ptr <DomainAbstraction> &currentAbstraction) {
        /*
         * Even-Split will put the missed fact into a new group, together with half of its old group (randomly).
         * This is done for every variable that has indeed a missed fact in its domain.
         * */
        VariableGroupVectors oldAbstraction = currentAbstraction->getAbstractDomains();
        VariableGroupVectors newAbstraction = oldAbstraction;

        vector<int> stateValues = flaw->getStateWhereFlawHappensCopy();
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(
                stateValues);

        shared_ptr<vector<FactPair>> missedFacts = flaw->missedFacts;
        if (log.is_at_least_debug()) {
            log << "--Missed facts: " << *missedFacts << endl;
            log << "Real State where flaw happened: " << stateValues << endl;
            log << "--Abstract State Where Flaw happened: " << abstractStateWhereFlawHappened << endl;
            for (const auto& factPair: *missedFacts) {
                log << "Group Facts for missed var(in real space)" << factPair.var << ": "<< currentAbstraction->getVariableGroupFacts(factPair.var, abstractStateWhereFlawHappened.at(factPair.var)) << endl;
            }
        }
        assert(!missedFacts->empty());
        // loop over Missed Facts
        for (auto factPair : *missedFacts) {
            // Get and Copy domain abstraction of affected variable
            VariableGroupVector oldVariableAbstractDomain = oldAbstraction.at(factPair.var);
            VariableGroupVector newVariableAbstractDomain = oldVariableAbstractDomain;
            // determine groupNr for new group
            int maxGroupNumber = *max_element(oldVariableAbstractDomain.begin(), oldVariableAbstractDomain.end());
            // position of missed fact inside variable domain, also change it immediately
            //int domainIndex = currentAbstraction->getDomainIndexOfVariableValue(factPair.var, factPair.value);
            newVariableAbstractDomain[factPair.value] = maxGroupNumber + 1;

            // change half of old group (-1)
            int oldGroupNr = oldVariableAbstractDomain.at(factPair.value);
            vector<FactPair> oldGroupFacts = currentAbstraction->getVariableGroupFacts(factPair.var, oldGroupNr);
            int oldGroupSize = (int) oldGroupFacts.size();
            int changed = 1;
            int toChange = oldGroupSize / 2;
            for (auto oldGroupFact: oldGroupFacts) {
                if (oldGroupFact == factPair) {
                    continue;
                }
                if (changed >= toChange) {
                    break;
                }
                newVariableAbstractDomain[oldGroupFact.value] = maxGroupNumber + 1;
                changed++;
            }
            // Write new domain abstraction for variable to result (Due to downward-formalism just one missed fact per variable can exist)
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        assert(newAbstraction.size() == oldAbstraction.size());
        return newAbstraction;
    }
}