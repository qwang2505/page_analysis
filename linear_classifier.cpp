#include "linear_classifier.h"

#include <assert.h>

using namespace std;

bool LinearClassifier::init(const vector<double>& weights, double threshold)
{
    assert(!_initialized);

    this->_weights = weights;
    this->_threshold = threshold;

    _initialized = true;
    return true;
}

bool LinearClassifier::classify(const vector<double>& features, double& score) const
{
    assert(features.size() >= this->_weights.size());
    score = 0.0;
    for (size_t i = 0; i < this->_weights.size(); ++i)
    {
        score += this->_weights[i] * features[i];
    }

    return score >= this->_threshold;
}

bool LinearClassifier::classify(const vector<double>& features) const
{
    double score;
    return this->classify(features, score);
}
