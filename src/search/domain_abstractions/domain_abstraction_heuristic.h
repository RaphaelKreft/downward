#ifndef DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H
#define DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

#include "heuristic_basis.h"

namespace domain_abstractions {
    class HeuristicBasis;

    class DomainAbstractionHeuristic : public Heuristic {
        // Heuristic instance that is stored and used to get h-values and construct it
        HeuristicBasis *heuristic_function;
    protected:
        virtual int compute_heuristic(const State &ancestor_state) override;

    public:
        HeuristicBasis *generate_heuristic(const options::Options &opts, utils::LogProxy &log);

        explicit DomainAbstractionHeuristic(const options::Options &opts);
    };
}

#endif //DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H