#ifndef _LIST_PAGE_CLASSIFIER_H_
#define _LIST_PAGE_CLASSIFIER_H_

#include "dom_tree.h"
#include "SvmClassifier.h"

class ListPageClassifier
{
    friend class ListPageClassifierTest;

public:
    ListPageClassifier() :
        m_initialized(false)
    {
    }

    ~ListPageClassifier()
    {
    }

    bool init(const char* config_file_path);

    // assume preprocess is done
    bool classify(DomNode* dom, const char* url) const;
private:
    enum FeatureNames
    {
        FN_LINK_TEXT_RATIO,
        FN_URL_IS_FILENAME,
        FN_NON_LINK_TEXT_LENGTH_HIGH,
        FN_LARGE_TEXT_COUNT_HIGH,
        FN_TOTAL_FEATURE_COUNT,
    };

    enum Internal_FeatureNames
    {
        IFN_LINK_TEXT_LENGTH,
        IFN_TEXT_LENGTH,
        IFN_LARGE_TEXT_COUNT,
        IFN_TOTAL_FEATURE_COUNT,
    };

    void extract_features(DomNode* dom, const char* url, std::vector<double>& features) const;
    void calculate_features(const char* url, const std::vector<int>& internal_features, std::vector<double>& features) const;
    void traverse(DomNode* node, std::vector<int>& features) const;
    void process_node(DomNode* node, std::vector<int>& features) const;
    bool is_url_filename(const char* url) const;

    int m_non_link_text_length_threshold;
    int m_large_text_count_threshold;
    int m_large_text_threshold;
    std::vector<std::string> m_filename_blacklist;

    SvmClassifier m_classifier;

    bool m_initialized;
};

#endif
