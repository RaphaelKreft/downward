#include "da_utils.h"

#include <random>
#include <vector>
#include <cassert>
#include <algorithm>

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

    int randIntFromRange(int min, int max) {
        // returns a randint from  range min-max including min/max
        std::random_device rd;     // only used once to initialise (seed) engine
        std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
        std::uniform_int_distribution<int> uni(min,max);

        return uni(rng);
    }

    /*std::vector<int> iRange(int start, int end) {
        std::vector<int> range;
        for (int i = start; i <= end; i++) {
            range.push_back(i);
        }
        return range;
    } */
    /*
     * std::unordered_set<int> choseSet(toChooseFrom.begin(), toChooseFrom.end());
        std::vector<int> toShuffle(choseSet.begin(), choseSet.end());
     * */

    std::vector<int> randomPickFromList(int numElements, std::vector<int> toChooseFrom) {
        // create set of unique nums of input vector
        std::shuffle(toChooseFrom.begin(), toChooseFrom.end(), std::mt19937(std::random_device()()));

        std::vector<int> elementSelection(toChooseFrom.begin(), toChooseFrom.begin()+numElements);
        assert( (int) elementSelection.size() == numElements);
        return elementSelection;
    }
}