#ifndef _SVM_CLASSIFIER_H_
#define _SVM_CLASSIFIER_H_

#include <cstddef>
#include <vector>
#include "svm.h"
//struct svm_model;
//

class SvmClassifier
{
public:
    SvmClassifier();
    ~SvmClassifier();
    bool init(const char* model_file_path);
    double classify(const std::vector<double>& features) const;

    struct svm_model* m_model;
    bool m_initialized;
};
#endif
