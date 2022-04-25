#ifndef DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H
#define DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "heuristic_basis.h"

namespace domain_abstractions {
    class HeuristicBasis;

    class DomainAbstractionHeuristic : public Heuristic {
        //Heuristic instance that is stored and used to get h-values and construct it
        std::shared_ptr<HeuristicBasis> heuristic_function;
    protected:
        virtual int compute_heuristic(const State &ancestor_state) override;

    public:
        std::shared_ptr<HeuristicBasis> generate_heuristic(const options::Options &opts, utils::LogProxy &log);
        DomainAbstractionHeuristic(const options::Options &opts);
        ~DomainAbstractionHeuristic();
    };
}

#endif //DOMAIN_ABSTRACTIONS_DOMAIN_ABSTRACTION_HEURISTIC_H