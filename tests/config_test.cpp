#include "gtest/gtest.h"

#include "config.h"
#include <vector>
#include <cstdio>

using namespace std;

template <class T>
void compare_vector(const vector<T>& first, const vector<T>& second)
{
    EXPECT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
    {
        EXPECT_EQ(first[i], second[i]);
    }
}

TEST(Config, list_page_classifier)
{
    Config config;
    bool success = config.Init("../list_page_classifier.ini");
    EXPECT_TRUE(success);

    EXPECT_EQ(80, config.GetIntValue("listPageClassifier", "large_text_length_threshold"));
    EXPECT_EQ(10, config.GetIntValue("listPageClassifier", "not_exist", 10));
    vector<string> filename_blacklist;
    config.GetStringList("listPageClassifier", "url_filename_blacklist", filename_blacklist, "");
    string str_list[] = {string("forum."), string("list"), string("default."), string("index.")};
    vector<string> results(str_list, str_list + sizeof(str_list) / sizeof(string));
    compare_vector(results, filename_blacklist);
    EXPECT_EQ(string("abc"), config.GetValue("listPageClassifier", "name", "abc"));
}

TEST(Config, body_extractor)
{
    Config config;
    bool success = config.Init("../body_extractor.ini");
    EXPECT_TRUE(success);
    const char* c_section_name = "bodyExtractor";

    double weights[] = {0.01,1,0,0,0,0,0,0,0,1,0,25,-25,0};
    vector<double> weights_vec(weights, weights + sizeof(weights) / sizeof(weights[0]));

    const vector<double>& weights_result = config.GetDoubleList(c_section_name, "classifier_weights");
    compare_vector(weights_vec, weights_result);

    const char* factor_tag_names[] = {"div", "blockquote", "form", "th"};
    vector<string> factor_tag_names_vec(factor_tag_names, factor_tag_names + sizeof(factor_tag_names) / sizeof(factor_tag_names[0]));
    compare_vector(factor_tag_names_vec, config.GetStringList(c_section_name, "factor_tag_names"));
    EXPECT_EQ(true, config.GetBoolValue(c_section_name, "negative_tags_enabled"));
    EXPECT_EQ(25, config.GetIntValue(c_section_name, "min_text_length"));
    string sanitize_expr("FN_IS_HEADER_TAG == 1 && FN_BASIC_WEIGHT < 0 || FN_IS_HEADER_TAG == 1 && FN_LINK_DENSITY > 0.33 || FN_IS_INTERACTIVE_TAG == 1 || FN_IS_STRUCT_TAG == 1 && FN_BASIC_WEIGHT < 0");
    EXPECT_EQ(sanitize_expr, config.GetStringValue(c_section_name, "sanitize_expression"));
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
