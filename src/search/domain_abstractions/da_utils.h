#ifndef FAST_DOWNWARD_DA_UTILS_H
#define FAST_DOWNWARD_DA_UTILS_H

#include <random>
#include <vector>
#include <cassert>
#include <random>

namespace domain_abstractions {
    std::vector<std::vector<int>> groupCombinations(std::vector<std::vector<int>> groupNumbersPerVar);

    std::vector<int> randomPickFromList(int numElements, std::vector<int> toChooseFrom);

}


#endif //FAST_DOWNWARD_DA_UTILS_H
