#include "gtest/gtest.h"

#include "boolean_classifier.h"

#include <vector>
#include <string>
#include <algorithm>
#include <map>

using namespace std;

TEST(BooleanClassifier, main)
{
    const char* feature_names[] = {"first", "second", "third", "fourth"};
    vector<string> feature_name_vector(feature_names, feature_names + sizeof(feature_names) / sizeof(feature_names[0]));
    const char* expression_str[] = 
    {
        "first == 1.0",
        "first == 0 || second >= 2 && fourth < 1 && third == 0.0 || third == 1.0 || first == 1 && second == 2",
        "first == 1.0 || second >= 2",
        "first == 1.0 && second >= 2",
        "! third <= 5 && ! third == 0.0 && second == 1.0 || fourth == 5",
        "first == 0 && first == 0 && first == 0 || second == 1.0 && second == 1.0",
    };

    double features[][4] = 
    {
        1, 2, 0, 3,
        0, 1, 1, 2,
        1, 0, 2, 1,
        0, 2, 3, 0,
        5, 2, 0, 0,
        1, 3, 2, 4,
        2, 2, 6, 5,
        1, 1, 6, 5,
    };

    bool results[] = 
    {
        1, 0, 1, 0, 0, 2, 0, 1,
        1, 1, 0, 1, 1, 0, 0, 0,
        1, 0, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 1,
        0, 1, 0, 1, 0, 0, 0, 1,
    };

    for (size_t i = 0; i < sizeof(expression_str) / sizeof(expression_str[0]); ++i)
    {
        BooleanClassifier classifier;
        EXPECT_TRUE(classifier.init(expression_str[i], feature_name_vector));
        
        for (size_t j = 0; j < sizeof(features) / sizeof(features[0][0]) / 4; ++j)
        {
            vector<double> feature_vector(features[j], features[j] + 4);
            map<int, double> feature_map;
            for (size_t k = 0; k < feature_vector.size(); ++k)
            {
                feature_map[k] = feature_vector[k];
            }

            bool result = classifier.classify(feature_map);
            EXPECT_EQ(results[i * sizeof(features) / sizeof(features[0][0]) / 4 + j], result);
        }
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
