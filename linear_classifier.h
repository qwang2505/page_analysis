#ifndef _LINEAR_CLASSIFIER_H_
#define _LINEAR_CLASSIFIER_H_

#include <vector>

class LinearClassifier
{
public:
    LinearClassifier() :
        _initialized(false)
    {
    }

    bool init(const std::vector<double>& weights, double threshold);

    bool classify(const std::vector<double>& features, double& score) const;

    bool classify(const std::vector<double>& features) const;

private:
    bool _initialized;
    std::vector<double> _weights;
    double _threshold;
};

#endif
