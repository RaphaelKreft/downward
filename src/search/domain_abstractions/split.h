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

    class DomainSplitter {
        SplitMethod currentMethod;
        utils::LogProxy &log;

    public:
        DomainSplitter(const std::string &method, utils::LogProxy &log);

        VariableGroupVectors split(const std::shared_ptr<Flaw> &flaw,
                                   const std::shared_ptr<DomainAbstraction> &currentAbstraction);

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

        static VariableGroupVectors performSingleValueSplit(const std::shared_ptr<Flaw> &flaw,
                                                            const std::shared_ptr<DomainAbstraction> &currentAbstraction);

    private:

        static VariableGroupVectors performRandomUniformSplit(const std::shared_ptr<Flaw> &flaw,
                                                       const std::shared_ptr<DomainAbstraction> &currentAbstraction);
    };

}


#endif //DOMAIN_ABSTRACTIONS_SPLIT_H
