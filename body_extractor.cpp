#include "body_extractor.h"

#include "utils.h"
#include <cmath>
#include <assert.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdlib>

using namespace std;

// define section name of configs.
static const char* c_section_name = "bodyExtractor";

// define header tags.
static const char* _header_tags[] = {"h1", "h2", "h3", "h4", "h5", "h6"};
// interactive: 
static const char* _interactive_tags[] = {"form", "iframe"};
// struct: 
static const char* _struct_tags[] = {"table", "ul", "div"};

// vectors of tags, why?
static const vector<string> c_header_tags(_header_tags, _header_tags + sizeof(_header_tags) / sizeof(_header_tags[0]));
static const vector<string> c_interactive_tags(_interactive_tags, _interactive_tags + sizeof(_interactive_tags) / sizeof(_interactive_tags[0]));
static const vector<string> c_struct_tags(_struct_tags, _struct_tags + sizeof(_struct_tags) / sizeof(_struct_tags[0]));

// # in #define: converte param to char.
// so all features defined in body_extractor_features.h is just strings?
static const char* c_feature_names[] = 
{
#define BODY_EXTRACTOR_FEATURE(f) #f,
#include "body_extractor_features.h"
#undef BODY_EXTRACTOR_FEATURE
};

//select the best one
bool comparer(const DomNode* first, const DomNode* second)
{
    // compare by basic weight/score.
    return first->get_extra(BodyExtractor::FN_BASIC_WEIGHT) < second->get_extra(BodyExtractor::FN_BASIC_WEIGHT);
}

/*
common aggregation features: FN_TEXT_LENGTH, FN_LINK_LENGTH, FN_COMMA_COUNT, FN_LINK_DENSITY
features used by basic    classifier: FN_COMMA_COUNT, FN_LINK_DENSITY, MATCHED_GOOD/BAD_CLASS_IDS, TAG_FACTOR, TEXT_LENGTH, CANDIDATE_SOURCE 
features used by sanitize classifier: FN_IS_HEADER_TAG, FN_BASIC_WEIGHT, FN_LINK_DENSITY, FN_IS_INTERACTIVE_TAG, FN_IS_STRUCT_TAG
features used by sibling classifier : FN_IS_P_TAG, FN_TEXT_CURRENT_LENGTH, FN_LINK_DENSITY, FN_FOUND_BREAK_PUNC
*/

// use this class to visit dom tree.
// in preprocess, drop negative node.
class BodyExtractorVisitor : public DomTreeVisitor
{
public:
    BodyExtractorVisitor(const BodyExtractor* extractor, vector<DomNode*>& candidates) :
        _extractor(extractor),
        _candidates(candidates)
    {
    }

    // called in DomNode::postorder_traverse
    virtual bool preprocess(DomNode* node)
    {
        // drop negative node. if dropped, return false, else return true.
        return !this->_extractor->drop_negative_node(node);
    }

    // called in DomNode::postorder_traverse
    // extract features, and add into candidate if valid.
    virtual bool visit(DomNode* node)
    {
        // extract features?
        this->_extractor->extract_features(node);
        node->print_node();
        // validate
        if (this->_extractor->valid_node(node))
        {
            this->_extractor->add_candidate(node, 0, _candidates);
        }

        return true;
    }

private:
    const BodyExtractor* _extractor;
    vector<DomNode*>& _candidates;
};

// for dom node, visit return bool value, should or should not drop node.
class SanitizeVisitor : public DomTreeVisitor
{
public:
    // pass extractor object while constructing.
    SanitizeVisitor(const BodyExtractor* extractor) :
        _extractor(extractor)
    {
    }

    virtual bool visit(DomNode* node)
    {
        // if do not have basic weight, calculate and set first.
        if (!node->has_extra(BodyExtractor::FN_BASIC_WEIGHT))
        {
            double weight;
            this->_extractor->calculate_basic_score(node, weight);
            node->set_extra(BodyExtractor::FN_BASIC_WEIGHT, weight);
        }

        // get all features values
        const map<int, double>& features = node->get_extras();

        bool result = this->_extractor->_sanitize_classifier.classify(features);
        if (result)
        {
            DomNode::drop_node(node);
            return false;
        }

        return true;
    }

private:
    const BodyExtractor* _extractor;
};

BodyExtractor::BodyExtractor() :
    _initialized(false)
{
}

BodyExtractor::~BodyExtractor()
{
}

bool BodyExtractor::init(const char* config_file_path)
{
    assert(config_file_path != NULL);
    assert(!this->_initialized);

    // init config.
    bool success = this->_config.Init(config_file_path);
    if (!success)
    {
        cout << "init config failed" << endl;
        return false;
    }

    // init base classifier, read weights and threshold value from config.
    const vector<double>& weights = this->_config.GetDoubleList(c_section_name, "classifier_weights");
    double threshold = this->_config.GetDoubleValue(c_section_name, "classifier_threshold", 0.0);
    success = this->_basic_classifier.init(weights, threshold);
    if (!success)
    {
        cout << "init basic classifier failed" << endl;
        return false;
    }

    // init sanitize classifier.
    vector<string> feature_names(c_feature_names, c_feature_names + sizeof(c_feature_names) / sizeof(c_feature_names[0]));

    string sanitize_expression = this->_config.GetStringValue(c_section_name, "sanitize_expression");
    success = this->_sanitize_classifier.init(sanitize_expression.c_str(), feature_names);
    if (!success)
    {
        cout << "init santize classifier failed" << endl;
        return false;
    }

    // init sibling classifier.
    string sibling_expression = this->_config.GetStringValue(c_section_name, "sibling_expression");
    success = this->_sibling_classifier.init(sibling_expression.c_str(), feature_names);
    if (!success)
    {
        cout << "init sibling classifier failed" << endl;
        return false;
    }

    const vector<string>& factor_tag_names = this->_config.GetStringList(c_section_name, "factor_tag_names");
    const vector<double>& factor_tag_values = this->_config.GetDoubleList(c_section_name, "factor_tag_values");
    if (factor_tag_names.size() != factor_tag_values.size())
    {
        cout << "factor tag name/values should be in pairs" << endl;
        return false;
    }

    this->_initialized = true;
    return true;
}

DomNode* BodyExtractor::extract(DomNode* dom) const
{
    assert(dom != NULL);
    assert(_initialized);

    // traverse dom tree to drop invalid nodes, select candidate nodes, extract features
    vector<DomNode*> candidates;
    BodyExtractorVisitor visitor(this, candidates);
    // call preprocess, visit, postprocess in visitor.
    dom->postorder_traverse(visitor);

    // select ancestor nodes of candidates.
    vector<DomNode*>::const_iterator end = candidates.end();
    for (vector<DomNode*>::const_iterator iter = candidates.begin(); iter != end; ++iter)
    {
        this->select_ancestor_nodes(*iter, candidates);
    }

    // calculate scores, filter by threshold
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        DomNode* node = candidates[i];
        double score;
        // calculate basic score
        bool success = this->calculate_basic_score(node, score);
        // set score
        node->set_extra(FN_BASIC_WEIGHT, score);
        // set is candidate
        node->set_extra(FN_IS_CANDIDATE, success);
        cout << "candidate " << node->get_tag() << " " << score << node->get_attribute("class") << " " << node->get_attribute("id") << endl;
    }

    if (candidates.size() == 0)
    {
        return NULL; //TODO: try again without invalid node drop;
    }

    DomNode* best_candidate = *(max_element(candidates.begin(), candidates.end(), comparer));
    cout << "best candidate" << best_candidate->get_tag() << endl;

    // get the main body node
    DomNode* body = this->get_body(best_candidate);

    if (body == NULL)
    {
        return NULL;
    }

    // sanitize
    this->sanitize(body);

    // post validation
    if (this->post_validate(body))
    {
        return body;
    }
    else
    {
        return NULL;
    }
}

// called in DomNode::postorder_traverse, than called by visitor's preprocess.
// drop negative node: by node tag, class and id to drop node. if should by dropped,
// drop it and return true, else return false.
bool BodyExtractor::drop_negative_node(DomNode* node) const
{
    bool dropped = false;
    // if match negative_tags list, drop it.
    if (this->_config.GetBoolValue(c_section_name, "negative_tags_enabled") && match_list(node->get_tag(), this->_config.GetStringList(c_section_name, "negative_tags"), 1) >= 0)
    {
        dropped = true;
    }
    // if negative class ids enabled and match, drop it.
    // first use class, if can not drop by class, use id.
    else if (this->_config.GetBoolValue(c_section_name, "negative_class_ids_enabled"))
    {
        const char* class_attrib = node->get_attribute("class");
        if (class_attrib != NULL &&
            match_list(class_attrib, this->_config.GetStringList(c_section_name, "negative_class_ids"), 2) >= 0 &&
            !match_list(class_attrib, this->_config.GetStringList(c_section_name, "positive_class_ids"), 2) >= 0)
        {
            dropped = true;
        }
        else
        {
            const char* id_attrib = node->get_attribute("id");
            if (id_attrib != NULL &&
                match_list(id_attrib, this->_config.GetStringList(c_section_name, "negative_class_ids"), 2) >= 0 &&
                !match_list(id_attrib, this->_config.GetStringList(c_section_name, "positive_class_ids"), 2) >= 0)
            {
                dropped = true;
            }
        }
    }

    if (dropped)
    {
        cout << "dropped " << node->get_tag() << endl;
        DomNode::drop_node(node);
        return true;
    }
    else
    {
        return false;
    }
}

bool BodyExtractor::rename_div(DomNode* node) const
{
    return true;
}

// is candidate?
bool BodyExtractor::valid_node(DomNode* node) const
{
    // if match candidate tag names, and text length is enough
    if (match_list(node->get_tag(), this->_config.GetStringList(c_section_name, "candidate_tag_names"), 1) >= 0)
    {
        if (node->get_extra(FN_TEXT_LENGTH) <= this->_config.GetIntValue(c_section_name, "min_text_length"))
        {
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

// extract features, and set info node's extras.
void BodyExtractor::extract_features(DomNode* node) const
{
    // get class and id
    const char* class_attrib = node->get_attribute("class");
    const char* id_attrib = node->get_attribute("id");

    // use enum as int, as count of enum members.
    vector<double> features(FN_TOTAL_FEATURE_COUNT, 0);

    // good class and ids.
    if (match_list(class_attrib, this->_config.GetStringList(c_section_name, "good_class_ids"), 2) != -1)
    {
        ++features[FN_MATCHED_GOOD_CLASS_IDS];
    }

    if (match_list(id_attrib, this->_config.GetStringList(c_section_name, "good_class_ids"), 2) != -1)
    {
        ++features[FN_MATCHED_GOOD_CLASS_IDS];
    }

    // bad class and ids.
    if (match_list(class_attrib, this->_config.GetStringList(c_section_name, "bad_class_ids"), 2) != -1)
    {
        ++features[FN_MATCHED_BAD_CLASS_IDS];
    }

    if (match_list(id_attrib, this->_config.GetStringList(c_section_name, "bad_class_ids"), 2) != -1)
    {
        ++features[FN_MATCHED_BAD_CLASS_IDS];
    }

    // factor tag names. TODO why called factor?
    int pos = match_list(node->get_tag(), this->_config.GetStringList(c_section_name, "factor_tag_names"), 1);
    if (pos != -1)
    {
        features[FN_TAG_FACTOR] = this->_config.GetDoubleList(c_section_name, "factor_tag_values")[pos];
    }

    // set features. why children? calculate from direct children
    for (size_t i = 0; i < node->get_children()->size(); ++i)
    {
        // get extra information from children, TODO but where extra in children been set?
        DomNode* child = (*node->get_children())[i];
        // text length
        features[FN_TEXT_LENGTH] += child->get_extra(FN_TEXT_LENGTH);
        // comma count
        features[FN_COMMA_COUNT] += child->get_extra(FN_COMMA_COUNT);
        // link length
        features[FN_LINK_LENGTH] += child->get_extra(FN_LINK_LENGTH);
        // node count
        features[FN_NODE_COUNT] += child->get_extra(FN_NODE_COUNT);
        // link count
        features[FN_LINK_COUNT] += child->get_extra(FN_LINK_COUNT);
    }

    // TODO understand this
    if (node->get_children()->size() == 0)
    {
        features[FN_NODE_COUNT] ++;
    }

    // get text string of node
    const string& text = node->get_text_string();
    // count with space.
    features[FN_CURRENT_TEXT_LENGTH] = static_cast<int>(text.size());//count_without_spaces(text.c_str());
    // same as current text length
    features[FN_TEXT_LENGTH] += features[FN_CURRENT_TEXT_LENGTH];
    // add comma count.
    features[FN_COMMA_COUNT] += static_cast<double>(count(text.begin(), text.end(), ',')); // TODO: need to consider chinese punctuations

    // get link count and length
    if (strncmp(node->get_tag(), "a", 1) == 0) //TODO: case insensesive
    {
        features[FN_LINK_LENGTH] += features[FN_CURRENT_TEXT_LENGTH];
        features[FN_LINK_COUNT] ++;
    }

    // calculate link density
    features[FN_LINK_DENSITY] = features[FN_TEXT_LENGTH] != 0 ? features[FN_LINK_LENGTH] / features[FN_TEXT_LENGTH] : 0.0;
    // calculate link node density
    features[FN_LINK_NODE_DENSITY] = features[FN_NODE_COUNT] != 0 ? features[FN_LINK_COUNT] / features[FN_NODE_COUNT] : 0.0;

    // is header?
    if (match_list(node->get_tag(),  c_header_tags, 1) >= 0)
    {
        features[FN_IS_HEADER_TAG] = 1;
    }

    // is interactive node?
    if (match_list(node->get_tag(),  c_interactive_tags, 1) >= 0)
    {
        features[FN_IS_INTERACTIVE_TAG] = 1;
    }

    // is struct node?
    if (match_list(node->get_tag(),  c_struct_tags, 1) >= 0)
    {
        features[FN_IS_STRUCT_TAG] = 1;
    }

    features[FN_IS_CANDIDATE] = 0;

    // set extra into node.
    for (int i = 0; i < static_cast<int>(features.size()); ++i)
    {
        node->set_extra(i, features[i]);
    }

    for (int j = 0; j < FN_TOTAL_FEATURE_COUNT; ++j)
    {
        cout << "fe " << node->get_tag() << " "  << node->get_attribute("class") << " " << node->get_attribute("id") << " " << node->get_extra(j) << " " << endl;
    }
}

void BodyExtractor::select_ancestor_nodes(DomNode* node, vector<DomNode*>& candidates) const
{
    DomNode* parent = node->get_parent();
    if (this->_config.GetBoolValue(c_section_name, "include_parent_node_enabled") && parent != NULL && find(candidates.begin(), candidates.end(), parent) == candidates.end())
    {
        // TODO understand source=1
        this->add_candidate(parent, 1, candidates);
    }

    if (this->_config.GetBoolValue(c_section_name, "include_grand_parent_node_enabled") && parent != NULL)
    {
        DomNode* grand_parent = parent->get_parent();

        if (grand_parent != NULL && find(candidates.begin(), candidates.end(), parent) == candidates.end())
        {
            // TODO understand source=2
            this->add_candidate(grand_parent, 2, candidates);
        }
    }
}

void BodyExtractor::add_candidate(DomNode* node, int source, vector<DomNode*>& candidates) const
{
    // if add into candidates as self, source=0.
    // if add into candidates as parent node, source=1.
    // if add into candidates as grand parent node, source=1.
    // use source to normalize score of candidates.
    node->set_extra(FN_CANDIDATE_SOURCE, source);
    candidates.push_back(node);
}

void BodyExtractor::sort_candidates(vector<DomNode*>& candidates) const
{
    sort(candidates.begin(), candidates.end(), comparer);
}

DomNode* BodyExtractor::get_body(DomNode* best_candidate) const
{
    DomNode* parent = best_candidate->get_parent();
    // TODO understand why
    if (parent == NULL)
    {
        return best_candidate;
    }

    vector<DomNode*> dropping_siblings;

    for (size_t i = 0; i < parent->get_children()->size(); ++i)
    {
        DomNode* sibling = (*parent->get_children())[i];

        if (!(sibling == best_candidate ||
            (sibling->get_extra_default(FN_IS_CANDIDATE, false) == 1 && sibling->get_extra(FN_BASIC_WEIGHT) >= max(10.0, sibling->get_extra(FN_BASIC_WEIGHT) * 0.2)) ||
            this->valid_paragraph_sibling(sibling)))
        {
            dropping_siblings.push_back(sibling);
        }
    }

    // drop unlikely sibling nodes.
    for (size_t i = 0; i < dropping_siblings.size(); ++i)
    {
        DomNode::drop_node(dropping_siblings[i]);
    }

    if (parent->get_children()->size() == 1)
    {
        return (*parent->get_children())[0];
    }
    else
    {
        return parent;
    }
}

/*
valid sibling classifier: FN_IS_P_TAG == 1 && FN_TEXT_CURRENT_LENGTH > 80 && FN_LINK_DENSITY < 0.25 || FN_IS_P_TAG && FN_CURRENT_TEXT_LENGTH < 80 && FN_LINK_DENSITY == 0.0 && FN_FOUND_BREAK_PUNC >= 0
*/
bool BodyExtractor::valid_paragraph_sibling(DomNode* sibling) const
{
    // TODO has break func?
    bool found_break_func =
        match_list(sibling->get_text(), this->_config.GetStringList(c_section_name, "paragraph_break_punctuations"), 2) >= 0 ||
        match_list(sibling->get_text(), this->_config.GetStringList(c_section_name, "paragraph_end_punctuations"), 3) >= 0;

    sibling->set_extra(FN_IS_P_TAG, strncmp(sibling->get_tag(), "p", 1) == 0);
    sibling->set_extra(FN_HAS_BREAK_PUNC, found_break_func);

    // understand sibling classifier, as in boolean classifier
    // boolean classifier is just judge by an expression, if satisfy it, return true, else return false.
    return this->_sibling_classifier.classify(sibling->get_extras());
}
/*
current boolean expression for sanitize:
FN_IS_HEADER_TAG == 1 && FN_BASIC_WEIGHT < 0 || FN_IS_HEADER_TAG == 1 && FN_LINK_DENSITY > 0.33 || FN_IS_INTERACTIVE_TAG == 1|| FN_IS_STRUCT_TAG == 1 && FN_BASIC_WEIGHT < 0
TODO: some sanitize work has been dropped
*/
void BodyExtractor::sanitize(DomNode* body) const
{
    SanitizeVisitor visitor(this);
    body->preorder_traverse(visitor);
}

// TODO: validate body text size, etc.;
bool BodyExtractor::post_validate(const DomNode* body) const
{
    return true;
}

bool BodyExtractor::calculate_basic_score(const DomNode* node, double& score) const
{
    vector<double> features(FN_TOTAL_FEATURE_COUNT, 0);
    for (int j = 0; j < FN_TOTAL_FEATURE_COUNT; ++j)
    {
        features[j] = node->get_extra(j);
    }

    // call classifier to get score by features.
    this->_basic_classifier.classify(features, score);
    // normalize score.
    score = score * (1 - features[FN_LINK_NODE_DENSITY]) / sqrt(features[FN_CANDIDATE_SOURCE] + 1);
    return score >= this->_config.GetDoubleValue(c_section_name, "classifier_threshold");
}
