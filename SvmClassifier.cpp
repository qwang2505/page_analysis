#include "SvmClassifier.h"

#include "svm.h"
#include <assert.h>
#include <vector>

SvmClassifier::SvmClassifier() :
    m_model(NULL),
    m_initialized(false)
{
}

SvmClassifier::~SvmClassifier()
{
    if (this->m_model != NULL)
    {
        svm_free_and_destroy_model(&this->m_model);
    }
}

bool SvmClassifier::init(const char* model_file_path)
{
    assert(!this->m_initialized);
    this->m_model = svm_load_model(model_file_path);
    if (this->m_model == NULL)
    {
        return false;
    }

    this->m_initialized = true;
    return true;
}

double SvmClassifier::classify(const std::vector<double>& features) const
{
    assert(this->m_initialized);
    struct svm_node* nodes = new struct svm_node[features.size() + 1];
    for (size_t i = 0; i < features.size(); ++i)
    {
        nodes[i].index = i + 1;
        nodes[i].value = features[i];
    }

    nodes[features.size()].index = -1;
    double label = svm_predict(this->m_model, nodes);
    delete[] nodes;
    return label;
}
