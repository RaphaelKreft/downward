#include "heuristic_basis.h"


namespace domain_abstractions {
    HeuristicBasis::HeuristicBasis(int max_time, utils::LogProxy &log) : max_time(max_time), log(log){

    }

    void HeuristicBasis::construct(std::shared_ptr<AbstractTask> task) {
        // TaskProxy, storing the concrete original Task
        TaskProxy taskProxy(*task);
        //new timer to limit construction_time
        utils::CountdownTimer timer(max_time);


    }

    int HeuristicBasis::getValue(State state) {
        return 0;
    }
}
