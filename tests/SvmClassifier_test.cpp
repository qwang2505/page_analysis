#include "gtest/gtest.h"
#include "SvmClassifier.h"

#include <vector>

using namespace std;

TEST(SvmClassifier, main)
{
    const double features[][4] = 
    {
        0.4, 0, 0, 0,
        0,   1, 0, 0,
        0.4, 1, 1, 0,
        0,   1, 0, 0,
        0.8, 0, 0, 1,
    };

    const double labels[] =
    {
        1, 0, 1, 0, 1,
    };

    SvmClassifier classifier;
    bool success = classifier.init("../list_page_classifier.svm");
    EXPECT_TRUE(success);

    for (size_t i = 0; i < sizeof(features) / sizeof(double) / 4; ++i)
    {
        vector<double> feature(features[i], features[i] + 4);
        double label = classifier.classify(feature);
        EXPECT_EQ(labels[i], label);
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
