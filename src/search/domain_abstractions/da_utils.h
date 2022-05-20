#ifndef FAST_DOWNWARD_DA_UTILS_H
#define FAST_DOWNWARD_DA_UTILS_H

#include <vector>
#include <cassert>

namespace domain_abstractions {
    std::vector<std::vector<int>> groupCombinations(std::vector<std::vector<int>> groupNumbersPerVar) {
        // Init
        int numResultingStates = 1;
        std::vector<std::vector<int>::iterator> iterators;

        int howManyVectors = (int) groupNumbersPerVar.size();
        for (auto & i : groupNumbersPerVar) {
            numResultingStates *= (int) i.size();
            iterators.push_back(i.begin());
        }

        assert(iterators.size() == groupNumbersPerVar.size());
        assert(howManyVectors == (int) groupNumbersPerVar.size());
        std::vector<std::vector<int>> resultingStates;
        resultingStates.reserve(numResultingStates);

        // Odometer to generate combinations
        while (iterators.at(0) != groupNumbersPerVar.at(0).end()) {
            // process the pointed-to elements
            std::vector<int> toInsert;
            for (int i = 0; i < howManyVectors; i++) {
                toInsert.push_back(*(iterators[i]));
            }
            assert(toInsert.size() == groupNumbersPerVar.size());
            resultingStates.push_back(toInsert);
            // the following increments the "odometer" by 1
            ++iterators[howManyVectors - 1];
            for (int i = howManyVectors - 1; (i > 0) && (iterators[i] == groupNumbersPerVar[i].end()); --i) {
                iterators[i] = groupNumbersPerVar[i].begin();
                ++iterators[i - 1];
            }
        }
        return resultingStates;
    };

};


#endif //FAST_DOWNWARD_DA_UTILS_H
