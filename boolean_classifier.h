#ifndef _BOOLEAN_CLASSIFIER_H_
#define _BOOLEAN_CLASSIFIER_H_

#include <vector>
#include <string>
#include <map>

class BooleanClassifier
{
public:
    BooleanClassifier() :
        _initialized(false)
    {
    }

    ~BooleanClassifier()
    {
    }

    bool init(const char* expression_str, const std::vector<std::string>& feature_names);
    bool classify(const std::map<int, double>& features) const;

private:
    bool _initialized;

    struct Atom
    {
    public:
        Atom(int feature_id, std::string feature_name, bool with_not, int comparer_op_id, double right_value, int group_id) :
            feature_id(feature_id), feature_name(feature_name), with_not(with_not), comparer_op_id(comparer_op_id), right_value(right_value), group_id(group_id)
        {
        }

        int feature_id;
        std::string feature_name;
        bool with_not;
        int comparer_op_id;
        double right_value;
        int group_id;
    };

    std::vector<Atom> _expression;
};
#endif
