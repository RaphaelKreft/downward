#ifndef DOMAIN_ABSTRACTIONS_UTILS_H
#define DOMAIN_ABSTRACTIONS_UTILS_H

#include <vector>

namespace domain_abstractions {
    static std::vector<std::vector<int>> groupCombinations(std::vector<std::vector<int>> groupNumbersPerVar) {
        // Init
        int numResultingStates = 1;
        std::vector<std::vector<int>::iterator> iterators;
        int howManyVectors = (int) groupNumbersPerVar.size();
        for (int i = 0; i < (int) groupNumbersPerVar.size(); i++) {
            numResultingStates *= (int) groupNumbersPerVar.at(i).size();
            iterators.at(i) = groupNumbersPerVar.at(i).begin();
        }
        std::vector<std::vector<int>> resultingStates;
        resultingStates.reserve(numResultingStates);

        // Odometer to generate combinations
        while (iterators[0] != groupNumbersPerVar.at(0).end()) {
            // process the pointed-to elements
            std::vector<int> toInsert;
            for (int i = 0; i < howManyVectors; i++) {
                toInsert.push_back(*iterators[i]);
            }
            resultingStates.push_back(toInsert);
            // the following increments the "odometer" by 1
            ++iterators[howManyVectors - 1];
            for (int i = howManyVectors - 1; (i > 0) && (iterators[i] == groupNumbersPerVar[i].end()); --i) {
                iterators[i] = groupNumbersPerVar[i].begin();
                ++iterators[i - 1];
            }
        }
        return resultingStates;
    }
}


#endif
