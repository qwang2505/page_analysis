#ifndef _BODY_EXTRACTOR_H_
#define _BODY_EXTRACTOR_H_

#include "config.h"
#include "linear_classifier.h"
#include "boolean_classifier.h"
#include "dom_tree.h"

#include <vector>
#include <string>

class BodyExtractor
{
public:
    BodyExtractor();

    ~BodyExtractor();

    bool init(const char* config_file_path);

    // returned dom node is a sub tree in the root dom tree, don't release this node since it shares the memory with root dom node
    DomNode* extract(DomNode* dom) const;

private:

    // features defined here
    enum FeatureNames
    {
        #define BODY_EXTRACTOR_FEATURE(f) f,
        #include "body_extractor_features.h"
        #undef BODY_EXTRACTOR_FEATURE
            
        FN_TOTAL_FEATURE_COUNT,
    };

    class Candidate
    {
    public:
        Candidate(DomNode* node, const std::vector<double>& features, double score) :
            _node(node),
            //_features(FN_TOTAL_FEATURE_COUNT, 0),
            _features(features),
            _score(score)
        {
        }

        const DomNode* get_node() const
        {
            return this->_node;
        }

        void set_score(double score)
        {
            this->_score = score;
        }

        double get_score() const
        {
            return this->_score;
        }

        void set_feature(int feature_name, double feature_value)
        {
            this->_features[feature_name] = feature_value;
        }

        const std::vector<double>* get_features() const
        {
            return &this->_features;
        }

        DomNode* _node;
        std::vector<double> _features;
        double _score;
    };

    friend class BodyExtractorVisitor;
    friend class SanitizeVisitor;
    friend bool comparer(const DomNode*, const DomNode*);

    bool drop_negative_node(DomNode* node) const;
    bool rename_div(DomNode* node) const;
    bool valid_node(DomNode* node) const;
    void extract_features(DomNode* node) const;
    void select_ancestor_nodes(DomNode* node, std::vector<DomNode*>& candidates) const;
    void add_candidate(DomNode* node, int source, std::vector<DomNode*>& candidates) const;
    void sort_candidates(std::vector<DomNode*>& candidates) const;
    DomNode* get_body(DomNode* best_candidate) const;
    bool valid_paragraph_sibling(DomNode* sibling) const;
    void sanitize(DomNode* body) const;
    bool post_validate(const DomNode* body) const;
    bool calculate_basic_score(const DomNode* node, double& score) const;

    bool _initialized;

    Config _config;
    LinearClassifier _basic_classifier;
    BooleanClassifier _sanitize_classifier;
    BooleanClassifier _sibling_classifier;
};

#endif

