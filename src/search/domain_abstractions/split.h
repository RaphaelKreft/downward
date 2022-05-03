#ifndef DOMAIN_ABSTRACTIONS_SPLIT_H
#define DOMAIN_ABSTRACTIONS_SPLIT_H

#include "../utils/logging.h"

#include "data_structures.h"
#include "domain_abstraction.h"

namespace domain_abstractions {

    // Enum, representing the options that exist on how to split Abstract Domains of DomainAbstraction
    enum class SplitMethod {
        HARDSPLIT // new groups -> States where transition would have worked + Others
    };

    class DomainSplitter {
        SplitMethod currentMethod;
        utils::LogProxy &log;

    public:
        DomainSplitter(SplitMethod method, utils::LogProxy &log);

        VariableGroupVectors split(const std::shared_ptr<Flaw>& flaw,
                                   const std::shared_ptr<DomainAbstraction>& currentAbstraction);

        // map string to ENUM
        static SplitMethod getEnumForString(const std::string& splitMethodSuggestion) {
            if (splitMethodSuggestion == "HardSplit") {
                return SplitMethod::HARDSPLIT;
            } else {
                // by default always use Hardsplit
                return SplitMethod::HARDSPLIT;
            }
        }

    private:
        VariableGroupVectors performHardSplit(const std::shared_ptr<Flaw>& flaw,
                                              const std::shared_ptr<DomainAbstraction>& currentAbstraction);
    };

}


#endif //DOMAIN_ABSTRACTIONS_SPLIT_H
