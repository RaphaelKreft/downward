#ifndef DOMAIN_ABSTRACTIONS_SPLIT_H
#define DOMAIN_ABSTRACTIONS_SPLIT_H

#include "data_structures.h"
#include "domain_abstraction.h"
#include "transition_system.h"

namespace domain_abstractions {

    // Enum, representing the options that exist on how to split Abstract Domains of DomainAbstraction
    enum class SplitMethod {
        HARDSPLIT // new groups -> States where transition would have worked + Others
    };

    class DomainSplitter {
        SplitMethod currentMethod;

    public:
        DomainSplitter(SplitMethod method);

        VariableGroupVectors split(std::shared_ptr<Flaw> flaw,
                                   std::unique_ptr<DomainAbstraction> currentAbstraction);

        // map string to ENUM
        static SplitMethod getEnumForString(std::string splitMethodSuggestion) {
            if (splitMethodSuggestion == "HardSplit") {
                return SplitMethod::HARDSPLIT;
            } else {
                // by default always use Hardsplit
                return SplitMethod::HARDSPLIT;
            }
        }

    private:
        VariableGroupVectors performHardSplit(std::shared_ptr<Flaw> flaw,
                                              std::unique_ptr<DomainAbstraction> currentAbstraction);
    };

}


#endif //DOMAIN_ABSTRACTIONS_SPLIT_H
