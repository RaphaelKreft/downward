#include "split.h"

namespace domain_abstractions {

    DomainSplitter::DomainSplitter(SplitMethod method) : currentMethod(method) {
    }

    VariableGroupVectors DomainSplitter::split(shared_ptr<Flaw> flaw, unique_ptr<DomainAbstraction> currentAbstraction) {
        // Select method on how to split and call Submethod. We return the new Abstraction
        if (currentMethod == SplitMethod::HARDSPLIT) {
            return performHardSplit( flaw, move(currentAbstraction));
        } else {
            raise(EXIT_FAILURE);
        }
    }

    VariableGroupVectors DomainSplitter::performHardSplit(shared_ptr<Flaw> flaw, unique_ptr<DomainAbstraction> currentAbstraction) {
        /*
         * Split missed fact from rest of facts in same group for every missed fact
         * */
        VariableGroupVectors oldAbstraction = currentAbstraction->getAbstractDomains();
        VariableGroupVectors newAbstraction = oldAbstraction;
        vector<int> stateValues = flaw->stateWhereFlawHappens;
        vector<int> abstractStateWhereFlawHappened = currentAbstraction->getGroupAssignmentsForConcreteState(stateValues);
        // TODO: we assume that there is max one missed fact for every variable!?
        vector<FactPair> missedFacts = flaw->missedFacts;
        // loop over variables and their domains
        for (FactPair &factPair: missedFacts) {
            VariableGroupVector oldVariableAbstractDomain = oldAbstraction.at(factPair.var);
            // TODO does copy work?
            VariableGroupVector newVariableAbstractDomain = oldVariableAbstractDomain;
            int currentNumGroups = *max_element(oldVariableAbstractDomain.begin(), oldVariableAbstractDomain.end());
            // just give missed-fact a new group
            int domainIndex = currentAbstraction->getDomainIndexOfVariableValue(factPair.var, factPair.value);
            newVariableAbstractDomain[domainIndex] = currentNumGroups + 1;
            newAbstraction[factPair.var] = newVariableAbstractDomain;
        }
        return newAbstraction;
    }
}