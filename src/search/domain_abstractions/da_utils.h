#ifndef FAST_DOWNWARD_DA_UTILS_H
#define FAST_DOWNWARD_DA_UTILS_H

#include <random>
#include <vector>
#include <cassert>
#include <random>

namespace domain_abstractions {
    std::vector<std::vector<int>> groupCombinations(std::vector<std::vector<int>> groupNumbersPerVar) {
        // Init
        int numResultingStates = 1;
        std::vector<std::vector<int>::iterator> iterators;

        int howManyVectors = (int) groupNumbersPerVar.size();
        for (auto &i: groupNumbersPerVar) {
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
    }

    std::vector<int> iRange(int start, int end) {
        /*
         * Return a range of integers including start and end
         * */
        std::vector<int> range;
        for (int i = start; i <= end; i++) {
            range.push_back(i);
        }
        return range;
    }

    std::vector<int> uniqueRandomPickFromList(int numElements, std::vector<int> toChooseFrom) {
        // create set of unique nums of input vector
        std::unordered_set<int> choseSet(toChooseFrom.begin(), toChooseFrom.end());
        std::vector<int> toShuffle(choseSet.begin(), choseSet.end());
        std::shuffle(toShuffle.begin(), toShuffle.end(), std::mt19937(std::random_device()()));

        std::vector<int> elementSelection(toShuffle.begin(), toShuffle.begin()+numElements);
        assert( (int) elementSelection.size() == numElements);
        return elementSelection;
    }

}


#endif //FAST_DOWNWARD_DA_UTILS_H
