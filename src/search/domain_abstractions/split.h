#ifndef DOMAIN_ABSTRACTIONS_SPLIT_H
#define DOMAIN_ABSTRACTIONS_SPLIT_H

#include "../utils/logging.h"

#include "data_structures.h"
#include "domain_abstraction.h"


namespace domain_abstractions {

    // Enum, representing the options that exist on how to split Abstract Domains of DomainAbstraction
    enum class SplitMethod {
        SINGLEVALUESPLIT,  // new groups -> For every variable: fact that was part of flaw becomes a new group
        RANDOMUNIFORMSPLIT   // new groups -> For every variable: fact that was part of flaw + fill up new group so that
        //               half of old group where fact was located is in new group and rest in old
    };

    // Enum representing the options that exist when using Single Value Split where we need to choose the variable/Fact we want to split along
    enum class SplitSelector {
        MIN_NEWSTATES, // choose variable whose split would introduce least new states in abstraction
        LEAST_REFINED, // choose variable that has been least refined until now
        RANDOM // choose variable random
    };

    class DomainSplitter {
        SplitMethod currentSplitMethod;
        SplitSelector currentSplitSelector;
        utils::LogProxy &log;

    public:
        DomainSplitter(const std::string &splitMethodString, const std::string &splitSelectorString ,utils::LogProxy &log);

        VariableGroupVectors split(const std::shared_ptr<Flaw> &flaw,
                                   const std::shared_ptr<DomainAbstraction> &currentAbstraction, bool splitRandomValue);

        // map string to ENUM
        static SplitMethod getEnumForString(const std::string &splitMethodSuggestion) {
            if (splitMethodSuggestion == "SingleValueSplit") {
                return SplitMethod::SINGLEVALUESPLIT;
            } else if (splitMethodSuggestion == "RandomUniformSplit") {
                return SplitMethod::RANDOMUNIFORMSPLIT;
            } else {
                // by default always use best known option (Now: hardsplit)
                return SplitMethod::RANDOMUNIFORMSPLIT;
            }
        }

        static SplitSelector getSplitSelectorForString(const std::string &splitSelectString) {
            if (splitSelectString == "random") {
                return SplitSelector::RANDOM;
            } else if (splitSelectString == "min_states_gain") {
                return SplitSelector::MIN_NEWSTATES;
            } else if (splitSelectString == "least_refined") {
                // by default always use best known option (Now: hardsplit)
                return SplitSelector::LEAST_REFINED;
            } else {
                // fallback
                return SplitSelector::RANDOM;
            }
        }

    private:

        static VariableGroupVectors performSingleValueSplit(const std::shared_ptr<Flaw> &flaw,
                                                            const std::shared_ptr<DomainAbstraction> &currentAbstraction);

        VariableGroupVectors
        performSingleValueSplitOneFact(const std::shared_ptr<Flaw> &flaw,
                                       const std::shared_ptr<DomainAbstraction> &currentAbstraction);

        static VariableGroupVectors performRandomUniformSplit(const std::shared_ptr<Flaw> &flaw,
                                                              const std::shared_ptr<DomainAbstraction> &currentAbstraction);

        VariableGroupVectors performRandomUniformSplitOneFact(const std::shared_ptr<Flaw> &flaw,
                                                                     const std::shared_ptr<DomainAbstraction> &currentAbstraction);

        static int
        selectSplitFact(const std::shared_ptr<Flaw> &flaw, const std::shared_ptr<DomainAbstraction> &currentAbstraction,
                        SplitSelector); // given an abstraction and a list of variables and a split selector, the index of the fact
                        // in the missedFacts is returned that is best to split along according to split selector criterion
    };

}


#endif //DOMAIN_ABSTRACTIONS_SPLIT_H
