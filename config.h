#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <map>
#include <string>
#include <vector>

class Config
{
public:
    ~Config();
    bool Init(const char* conf_path, bool write = false);
    bool HasKey(const std::string& section, const std::string& key) const;
    std::string GetValue(const std::string& section, const std::string& key) const;
    std::string GetValue(const std::string& section, const std::string& key, const std::string& default_value) const;
    int GetIntValue(const std::string& section, const std::string& key) const;
    int GetIntValue(const std::string& section, const std::string& key, int default_value) const;
    double GetDoubleValue(const std::string& section, const std::string& key) const;
    double GetDoubleValue(const std::string& section, const std::string& key, double default_value) const;
    bool GetBoolValue(const std::string& section, const std::string& key) const;
    bool GetBoolValue(const std::string& section, const std::string& key, bool default_value) const;
    std::string GetStringValue(const std::string& section, const std::string& key) const;
    bool GetStringList(const std::string& section, const std::string& key, std::vector<std::string>& list, const char* delimeter = "") const;
    const std::vector<std::string>& GetStringList(const std::string& section, const std::string& key, const char* delimeter = "") const;
    const std::vector<double>& GetDoubleList(const std::string& section, const std::string& key, const char* delimeter = "") const;
    void Set(const std::string& section, const std::string& key, const std::string& value);
private:
    static std::string GenUniqueKey(const std::string& section, const std::string& key);

    bool mWrite;
    std::string mPath;
    std::map<std::string, std::map<std::string, std::string> > mConfig;

    mutable std::map<std::string, int> m_int_values;
    mutable std::map<std::string, double> m_double_values;
    mutable std::map<std::string, bool> m_bool_values;
    mutable std::map<std::string, std::string> m_string_values;
    mutable std::map<std::string, std::vector<std::string> > m_string_lists;
    mutable std::map<std::string, std::vector<double> > m_double_lists;
};

#endif
