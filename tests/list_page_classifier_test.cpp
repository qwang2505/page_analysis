#include "gtest/gtest.h"

#include "list_page_classifier.h"
#include "dom_tree.h"
#include "utils.h"

template <class T>
void compare_vector(const vector<T>& first, const vector<T>& second)
{
    EXPECT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
    {
        EXPECT_EQ(first[i], second[i]);
    }
}

template <class T>
void print_vector(const char* prefix, const vector<T>& v)
{
    cout << prefix << " ";
    for (size_t i = 0; i < v.size(); ++i)
    {
        cout << v[i] << " ";
    }
}

class ListPageClassifierTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        bool success = this->m_classifier.init("list_page_classifier_test.ini");
        EXPECT_TRUE(success);
        EXPECT_EQ(20, this->m_classifier.m_non_link_text_length_threshold);
        EXPECT_EQ(2, this->m_classifier.m_large_text_count_threshold);
        EXPECT_EQ(8, this->m_classifier.m_large_text_threshold);
        string blacklist[] = {"forum.", "list", "default.", "index."};
        vector<string> blacklist_result(blacklist, blacklist + sizeof(blacklist) / sizeof(string));
        compare_vector(blacklist_result, this->m_classifier.m_filename_blacklist);

        default_html = 
            "html\n"
            " body hello world\n"
            "  a my space\n"
            "  b hello\tworld\n"
            " body1\n"
            "  x abc\n"
            "   h1\n"
            "    a hello\n";

        default_url = "http://www.google.com/w/a/b/c?q=1";

        printed_html = 
            "html,NULL,,2\n"
            "body,html,hello world,2\n"
            "a,body,my space,0\n"
            "b,body,hello\tworld,0\n"
            "body1,html,,1\n"
            "x,body1,abc,1\n"
            "h1,x,,1\n"
            "a,h1,hello,0\n";
    }

    /*
    fake html format as below:
    html
        a hello world
         b xyz
         c abc
        d
        e x
         mm xyz
    */

    size_t prefix_space_count(const string& line)
    {
        for (size_t j = 0; j < line.size(); ++j)
        {
            if (line[j] != ' ')
            {
                return j;
            }
        }

        return line.size();
    }

    void print_dom(DomNode* node, stringstream& text) const
    {
        const char* parent_tag = node->get_parent() != NULL ? node->get_parent()->get_tag() : "NULL";
        text << node->get_tag() << "," << parent_tag << "," << node->get_text() << "," << (int)node->get_children()->size() << endl;
        for (size_t i = 0; i < node->get_children()->size(); ++i)
        {
            this->print_dom((*node->get_children())[i], text);
        }
    }

    DomNode* create_dom_tree(const char* html)
    {
        vector<string> lines;
        split(html, "\n", lines);

        vector<DomNode*> curr_nodes;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const string& line = lines[i];
            size_t space_count = prefix_space_count(line);

            size_t layer = space_count;

            const string& line_data = line.substr(space_count);
            size_t pos = line_data.find(' ');
            string tag, text;
            if (pos == string::npos)
            {
                tag = line_data;
            }
            else
            {
                tag = line_data.substr(0, pos);
                text = line_data.substr(pos + 1);
            }

            if (tag.size() == 0)
            {
                if (curr_nodes.size() > 0)
                {
                    delete curr_nodes[0];
                }

                return NULL;
            }

            DomNode* node = new DomNode(tag, text);

            if (layer <= curr_nodes.size())
            {
                if (layer == 0)
                {
                    if (curr_nodes.size() != 0)
                    {
                        delete curr_nodes[0];
                        return NULL;//multiple roots
                    }
                }
                else
                {
                    curr_nodes[layer - 1]->append_child(node);
                }

                if (layer == curr_nodes.size())
                {
                    curr_nodes.push_back(node);
                }
                else
                {
                    curr_nodes[layer] = node;
                    for (size_t k = curr_nodes.size() - 1; k > layer; --k)
                    {
                        curr_nodes.pop_back();
                    }
                }
            }
            else
            {
                if (curr_nodes.size() > 0)
                {
                    delete curr_nodes[0];
                }

                return NULL;
            }
        }

        if (curr_nodes.size() == 0)
        {
            return NULL;
        }
        else
        {
            return curr_nodes[0];
        }
    }


protected:
    ListPageClassifier m_classifier;
    const char* default_html;
    const char* default_url;
    const char* printed_html;

    void test_dom_tree()
    {
        DomNode* dom = create_dom_tree(default_html);
        stringstream text;
        this->print_dom(dom, text);
        ASSERT_STREQ(printed_html, text.str().c_str());
        //cout << text.str();
        delete dom;
    }

    void test_is_url_filename()
    {
        const char* urls[] = {"http://www.baidu.com", "http://google.com/a/b/c/", "http://www.com/x?a=1", "http://www.a.com/index.html", "http://www.a/index.", "http://www.a/forum"};
        bool results[] = {false, false, true, false, false, true};
        for (size_t i = 0; i < sizeof(urls) / sizeof(const char*); ++i)
        {
            bool success = m_classifier.is_url_filename(urls[i]);
            EXPECT_EQ(results[i], success) << i;
        }
    }

    void test_process_node()
    {
        DomNode* nodes[] = {new DomNode("h1", "abc def\r\nx"), new DomNode("h2", "aaaabbbbccccaaaadddds"), new DomNode("a", "ab cde")};
        int results[][3] = {0, 7, 0, 0, 21, 1, 5, 5, 0};
        for (size_t i = 0; i < sizeof(nodes) / sizeof(DomNode*); ++i)
        {
            vector<int> features(ListPageClassifier::IFN_TOTAL_FEATURE_COUNT, 0);
            m_classifier.process_node(nodes[i], features);
            vector<int> result(results[i], results[i] + 3);
            //cout << "process_node" << i;
            compare_vector(result, features);
            delete nodes[i];
        }

    }

    void test_traverse()
    {
        vector<int> features(ListPageClassifier::IFN_TOTAL_FEATURE_COUNT, 0);
        DomNode* dom = create_dom_tree(default_html);

        m_classifier.traverse(dom, features);
        int results[] = {12, 35, 2};
        vector<int> results_vector(results, results + sizeof(results) / sizeof(int));
        compare_vector(results_vector, features);
        //print_vector("traverse_features", features);
        delete dom;
    }

    void test_extract_features()
    {
        DomNode* dom = create_dom_tree(default_html);
        vector<double> features(ListPageClassifier::FN_TOTAL_FEATURE_COUNT, 0);
        this->m_classifier.extract_features(dom, default_url, features);
        double results[] = {0.34285714285714285714285714285714, 1, 1, 1};
        vector<double> results_vector(results, results + sizeof(results) / sizeof(double));
        compare_vector(results_vector, features);
        //print_vector("extract_features", features);

        delete dom;
    }

    void test_main()
    {
        DomNode* dom= create_dom_tree(default_html);

        bool label = this->m_classifier.classify(dom, default_url);

        EXPECT_EQ(false, label);
        //printf("label:%d", label);
    }

    void test_calculate_features()
    {
        const char* urls[] = {"http://a/x", "http://a/b/c", "http://www.b/x/c/", "http://www.x/a/index.html"};
        int input_features[][3] = {9, 12, 1,  9, 100, 2,  9, 12, 3,  9, 15, 0,};
        double result_features[][4] = {0.75, 1, 0, 0,  0.09, 1, 1, 1,  0.75, 0, 0, 1,  0.6, 0, 0, 0};
        double labels[] = {1, 0, 1, 1};

        for (size_t i = 0; i < sizeof(urls) / sizeof(const char*); ++i)
        {
            vector<int> internal_features(input_features[i], input_features[i] + 3);
            vector<double> output_feature(4, 0);
            this->m_classifier.calculate_features(urls[i], internal_features, output_feature);

            vector<double> result_feature(result_features[i], result_features[i] + 4);
            compare_vector(result_feature, output_feature);
            double label = this->m_classifier.m_classifier.classify(output_feature);
            EXPECT_EQ(labels[i], label);
        }
    }
};

TEST_F(ListPageClassifierTest, dom_tree)
{
    test_dom_tree();
}

TEST_F(ListPageClassifierTest, is_url_filename)
{
    test_is_url_filename();
}

TEST_F(ListPageClassifierTest, process_node)
{
    test_process_node();
}

TEST_F(ListPageClassifierTest, traverse)
{
    test_traverse();
}

TEST_F(ListPageClassifierTest, calculate_features)
{
    test_calculate_features();
}

TEST_F(ListPageClassifierTest, extract_features)
{
    test_extract_features();
}

TEST_F(ListPageClassifierTest, main)
{
    test_main();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
