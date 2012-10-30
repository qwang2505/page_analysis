#include "gtest/gtest.h"
#include "utils.h"
#include <cstdio>
#include <string>
#include <vector>


using namespace std;

void compare_parse_result(const ParseResult& first, const ParseResult& second)
{
    printf("%s, %s, %s, %s\n", first.protocol.c_str(), first.host.c_str(), first.path.c_str(), first.query.c_str());
    EXPECT_EQ(first.protocol, second.protocol);
    EXPECT_EQ(first.host, second.host);
    EXPECT_EQ(first.path, second.path);
    EXPECT_EQ(first.query, second.query);
}

template <class T>
void compare_vector(const vector<T>& first, const vector<T>& second)
{
    EXPECT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
    {
        EXPECT_EQ(first[i], second[i]);
    }
}

TEST(urlparse, positive)
{
    const string urls[] = {"http://www.baidu.com", "http://news.sina.com.cn/a/b/c", "http://abc.com/x?a=1", "http://xyz.com/a/b/c/?x=1",
        "http://www.baidu.com?x=1", "http:/www.baidu.com"};
    ParseResult  results[] = {ParseResult("http", "www.baidu.com", "", ""), ParseResult("http", "news.sina.com.cn", "/a/b/c", ""), ParseResult("http", "abc.com", "/x", "a=1"), ParseResult("http", "xyz.com", "/a/b/c/", "x=1"),
        ParseResult("http", "www.baidu.com?x=1", "", ""), ParseResult("http:/www.baidu.com", "", "", "")};
    for (size_t i = 0; i < sizeof(urls) / sizeof(string); ++i)
    {
        compare_parse_result(results[i], urlparse(urls[i]));
    }
}

TEST(urlparse, negative)
{
    const string urls[] = {"", "http://www.baidu.com?x=1", "http:/www.baidu.com"};
    ParseResult  results[] = {ParseResult("", "", "", ""), ParseResult("http", "www.baidu.com?x=1", "", ""), ParseResult("http:/www.baidu.com", "", "", "")};
    for (size_t i = 0; i < sizeof(urls) / sizeof(string); ++i)
    {
        compare_parse_result(results[i], urlparse(urls[i]));
    }
}

TEST(get_basename, main)
{
    const string urls[][2] = {
        "/a/b/c", "c",
        "a", "a",
        "/a/b/c/", "",
        "", "",
        "a/b//c", "c",
        "a/b//c/d//", "",
        "/", "",
    };

    for (size_t i = 0; i < sizeof(urls) / sizeof(string) / 2; ++i)
    {
        string basename = get_basename(urls[i][0]);
        EXPECT_EQ(urls[i][1], basename);
    }
}

TEST(split, main)
{
    const char* strs[][2] = {
        " \t\nabc\rdef  xyz  a ", " \t\r\n",
        "abc xyzadf.aeo", "",
        "", "",
        "", "",
        "xyz", "",
    };

    const string result0[] = {string("abc"), string("def"), string("xyz"), string("a")};
    const string result1[] = {string("abc xyz"), string("adf.aeo")};
    const string result2[] = {};
    const string result3[] = {};
    const string result4[] = {string("xyz")};

    vector<string> results[] = {
        vector<string>(result0, result0 + sizeof(result0) / sizeof(string)),
        vector<string>(result1, result1 + sizeof(result1) / sizeof(string)),
        vector<string>(result2, result2 + sizeof(result2) / sizeof(string)),
        vector<string>(result3, result3 + sizeof(result3) / sizeof(string)),
        vector<string>(result4, result4 + sizeof(result4) / sizeof(string)),
    };

    for (size_t i = 0; i < sizeof(strs) / sizeof(string) / 2; ++i)
    {
        vector<string> segments;
        split(string(strs[i][0]), strs[i][1], segments);
        compare_vector(results[i], segments);
    }
}

TEST(match_list, main)
{
    const char* strs[] = {"ab", "abc", "", "bc", "Ab", "abcabc"};
    bool matches[] = {false, true, false, false, false, true};
    string str_list[] = {string("abc"), string("abd"), string("xab")};
    const vector<string> list(str_list, str_list + sizeof(str_list) / sizeof(string));

    for (size_t i = 0; i < sizeof(strs) / sizeof(const char*); ++i)
    {
        bool matched = match_list(strs[i], list) >= 0;
        EXPECT_EQ(matches[i], matched) << i;
    }

    EXPECT_EQ(0, match_list("abc", list, 1));
    EXPECT_EQ(-1, match_list("ab", list, 1));
    EXPECT_EQ(-1, match_list("abcd", list, 1));
    EXPECT_EQ(1, match_list("abd", list, 2));
    EXPECT_EQ(2, match_list("xaby", list, 2));
    EXPECT_EQ(-1, match_list("xa2by", list, 3));
    EXPECT_EQ(2, match_list("helloxab", list, 3));

}

TEST(count_without_spaces, main)
{
    const char* strs[] = {"abc\xf2\xe3""abc \r \ta", " \r\t\n\x0c\x0d", "", "abc", " a\rb\n\xff\x0c\x0d", "导航"};
    int counts[] = {9, 0, 0, 3, 3, 6};
    for (size_t i = 0; i < sizeof(strs) / sizeof(const char*); ++i)
    {
        int count = count_without_spaces(strs[i]);
        EXPECT_EQ(counts[i], count);
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
