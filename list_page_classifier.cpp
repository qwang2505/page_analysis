#include <assert.h>
#include <iostream>
#include <cstring>

#include "list_page_classifier.h"
#include "utils.h"
#include "config.h"

using namespace std;

const char* c_anchor_tag = "a";
size_t c_anchor_tag_len = strlen(c_anchor_tag);
const char* c_section_name = "listPageClassifier";
int c_large_text_length_threshold = 80;
int c_non_link_text_length_threshold = 600;
int c_large_text_count_threshold = 2;

bool ListPageClassifier::init(const char* config_file_path)
{
    assert(config_file_path != NULL);
    assert(!this->m_initialized);

    Config config;
    bool ret = config.Init(config_file_path);
    if (!ret)
    {
        std::cout << "init config failed";
        return false;
    }

    this->m_non_link_text_length_threshold = config.GetIntValue(c_section_name, "non_link_text_length_threshold", c_non_link_text_length_threshold);
    this->m_large_text_count_threshold = config.GetIntValue(c_section_name, "large_text_count_threshold", c_large_text_count_threshold);
    this->m_large_text_threshold = config.GetIntValue(c_section_name, "large_text_length_threshold", c_large_text_length_threshold);
    config.GetStringList(c_section_name, "url_filename_blacklist", this->m_filename_blacklist, "");

    const char* model_file_path = config.GetValue(c_section_name, "model_file_path").c_str();
    bool success = this->m_classifier.init(model_file_path);
    if (!success)
    {
        std::cout << "init classifier failed";
        return false;
    }

    this->m_initialized = true;
    return true;
}

bool ListPageClassifier::classify(DomNode* dom, const char* url) const
{
    assert(this->m_initialized);
    assert(dom != NULL);
    assert(url != NULL);

    std::vector<double> features(FN_TOTAL_FEATURE_COUNT, 0);
    this->extract_features(dom, url, features);
    double label = this->m_classifier.classify(features);
    if (label == 1.0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ListPageClassifier::extract_features(DomNode* dom, const char* url, std::vector<double>& features) const
{
    std::vector<int> internal_features(IFN_TOTAL_FEATURE_COUNT, 0);
    this->traverse(dom, internal_features);
    this->calculate_features(url, internal_features, features);
}

void ListPageClassifier::calculate_features(const char* url, const std::vector<int>& internal_features, std::vector<double>& features) const
{
    features[FN_LINK_TEXT_RATIO] = internal_features[IFN_TEXT_LENGTH] > 0 ? internal_features[IFN_LINK_TEXT_LENGTH] * 1.0 / internal_features[IFN_TEXT_LENGTH] : 0;

    features[FN_URL_IS_FILENAME] = this->is_url_filename(url);

    features[FN_NON_LINK_TEXT_LENGTH_HIGH] = internal_features[IFN_TEXT_LENGTH] - internal_features[IFN_LINK_TEXT_LENGTH] >= this->m_non_link_text_length_threshold;

    features[FN_LARGE_TEXT_COUNT_HIGH] = internal_features[IFN_LARGE_TEXT_COUNT] >= this->m_large_text_count_threshold;
}

void ListPageClassifier::traverse(DomNode* node, std::vector<int>& features) const
{
    const std::vector<DomNode*>* children = node->get_children();
    assert(children != NULL);

    this->process_node(node, features);

    for (std::vector<DomNode*>::const_iterator i = children->begin(); i != children->end(); ++i)
    {
        DomNode* child = *i;
        assert(child != NULL);
        this->traverse(child, features);
    }
}

void ListPageClassifier::process_node(DomNode* node, std::vector<int>& features) const
{
    const char* text = node->get_text();
    int text_length = count_without_spaces(text); 
    features[IFN_TEXT_LENGTH] += text_length;

    const char* tag = node->get_tag();
    assert(tag != NULL);
    if (strncmp(tag, c_anchor_tag, c_anchor_tag_len) == 0)
    {
        features[IFN_LINK_TEXT_LENGTH] += text_length;
    }
    else
    {
        if (text_length >= this->m_large_text_threshold)
        {
            features[IFN_LARGE_TEXT_COUNT] += 1;
        }
    }
}

bool ListPageClassifier::is_url_filename(const char* url) const
{
    ParseResult result = urlparse(url);
    string filename = get_basename(result.path);
    if (filename.length() > 0)
    {
        bool matched = match_list(filename.c_str(), this->m_filename_blacklist) != -1;
        if (matched)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}
