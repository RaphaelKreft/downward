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
            if (log.is_at_least_debug()) {
                log << "CEGAR -- Split: using hard-split!" << endl;
            }
            return performHardSplit(flaw, currentAbstraction);
        } else {
            // default fallback if unknown
            if (log.is_at_least_debug()) {
                log << "CEGAR -- Split: using fallback Method(Hard-Split) since given method not specified!" << endl;
            }
            return performHardSplit(flaw, currentAbstraction);
        }
    }

    VariableGroupVectors
    DomainSplitter::performHardSplit(const shared_ptr<Flaw>& flaw, const shared_ptr<DomainAbstraction>& currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every missed fact
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
            int domainIndex = currentAbstraction->getDomainIndexOfVariableValue(factPair.var, factPair.value);
            assert(domainIndex <= (int)oldVariableAbstractDomain.size());
            newVariableAbstractDomain[domainIndex] = maxGroupNumber + 1;
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        assert(newAbstraction.size() == oldAbstraction.size());
        return newAbstraction;
    }
}