#ifndef FAST_DOWNWARD_DOMAIN_ABSTRACTION_H
#define FAST_DOWNWARD_DOMAIN_ABSTRACTION_H


#include "../task_proxy.h"

namespace domain_abstractions {
    class DomainAbstraction {
    public:
        explicit DomainAbstraction();
        int mapStateToAbstractState(State state);
    };
}



#endif //FAST_DOWNWARD_DOMAIN_ABSTRACTION_H
