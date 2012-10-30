#include <fstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include "config.h"

#include "utils.h"

using namespace std;

Config::~Config()
{
    if (mWrite)
    {
        ofstream ofs(mPath.c_str());
        for (map<string, map<string, string> >::const_iterator iter = mConfig.begin();
                iter != mConfig.end(); ++iter)
        {
            const string& section = iter->first;
            ofs << '[' << section << ']' << endl;

            const map<string, string>& kvs = iter->second;
            for (map<string, string>::const_iterator i_iter = kvs.begin();
                    i_iter != kvs.end(); ++i_iter)
            {
                ofs << i_iter->first << '=' << i_iter->second << endl;
            }
        }
        ofs.close();
    }
}

bool Config::Init(const char* conf_path, bool write)
{
    mWrite = write;
    mPath = conf_path;
    mConfig.clear();
    ifstream ifs(mPath.c_str());
    if (!ifs)
    {
        return false;
    }
    string line;
    string section;
    while (getline(ifs, line))
    {
        if (line[0] == '[' && line[line.size() - 1] == ']')
        {
            section = line.substr(1, line.size() - 2);
        }
        else
        {
            size_t pos = line.find('=');
            if (pos == string::npos)
            {
                continue;
            }
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            mConfig[section][key] = value;
        }
    }
    ifs.close();
    return true;
}

bool Config::HasKey(const string& section, const string& key) const
{
    map<string, map<string, string> >::const_iterator iter = mConfig.find(section);
    if (iter == mConfig.end())
    {
        return false;
    }

    map<string, string>::const_iterator i_iter = iter->second.find(key);
    if (i_iter == iter->second.end())
    {
        return false;
    }
    return true;
}

string Config::GetValue(const string& section, const string& key) const
{
    map<string, map<string, string> >::const_iterator iter = mConfig.find(section);
    if (iter == mConfig.end())
    {
        return "";
    }

    map<string, string>::const_iterator i_iter = iter->second.find(key);
    if (i_iter == iter->second.end())
    {
        return "";
    }
    string value = i_iter->second;

    size_t start = value.find("$(");

    while (start != string::npos)
    {
        size_t end = value.find(")", start + 2);
        if (end == string::npos)
        {
            return "";
        }

        string key = value.substr(start + 2, end - start - 2);
        string sub = GetValue("global", key);
        
        string head = value.substr(0, start);
        string tail = value.substr(end + 1);
        value = head + sub + tail;

        start = value.find("$(");
    }
    return value;
}

string Config::GetValue(const string& section, const string& key, const string& default_value) const
{
    if (this->HasKey(section, key))
    {
        return this->GetValue(section, key);
    }
    else
    {
        return default_value;
    }
}

string Config::GenUniqueKey(const string& section, const string& key)
{
    return section + "##" + key;
}

int Config::GetIntValue(const string& section, const string& key) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, int>::const_iterator iter = this->m_int_values.find(unique_key);
    if (iter != this->m_int_values.end())
    {
        return iter->second;
    }
    else
    {
        string value = this->GetValue(section, key);
        int int_value = atoi(value.c_str());
        this->m_int_values[key] = int_value;
        return int_value;
    }
}

string Config::GetStringValue(const string& section, const string& key) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, string>::const_iterator iter = this->m_string_values.find(unique_key);
    if (iter != this->m_string_values.end())
    {
        return iter->second;
    }
    else
    {
        string value = this->GetValue(section, key);
        this->m_string_values[key] = value;
        return value;
    }
}

int Config::GetIntValue(const string& section, const string& key, int default_value) const
{
    if (this->HasKey(section, key))
    {
        return this->GetIntValue(section, key);
    }
    else
    {
        return default_value;
    }
}

double Config::GetDoubleValue(const string& section, const string& key) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, double>::const_iterator iter = this->m_double_values.find(unique_key);
    if (iter != this->m_double_values.end())
    {
        return iter->second;
    }
    else
    {
        string value = this->GetValue(section, key);
        double double_value = atof(value.c_str());
        this->m_double_values[key] = double_value;
        return double_value;
    }
}

double Config::GetDoubleValue(const string& section, const string& key, double default_value) const
{
    if (this->HasKey(section, key))
    {
        return this->GetDoubleValue(section, key);
    }
    else
    {
        return default_value;
    }
}

bool Config::GetBoolValue(const string& section, const string& key) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, bool>::const_iterator iter = this->m_bool_values.find(unique_key);
    if (iter != this->m_bool_values.end())
    {
        return iter->second;
    }
    else
    {
        string value = this->GetValue(section, key);
        bool bool_value = strncmp(value.c_str(), "1", 1) == 0;
        this->m_bool_values[key] = bool_value;
        return bool_value;
    }
}

bool Config::GetBoolValue(const string& section, const string& key, bool default_value) const
{
    if (this->HasKey(section, key))
    {
        return this->GetBoolValue(section, key);
    }
    else
    {
        return default_value;
    }
}

bool Config::GetStringList(const string& section, const string& key, vector<string>& list, const char* delimeter) const
{
    if (this->HasKey(section, key))
    {
        string value = this->GetValue(section, key, delimeter);
        split(value, delimeter, list);
        return true;
    }
    else
    {
        return false;
    }
}

const vector<string>& Config::GetStringList(const string& section, const string& key, const char* delimeter) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, vector<string> >::const_iterator iter = this->m_string_lists.find(unique_key);
    if (iter != this->m_string_lists.end())
    {
        return iter->second;
    }
    else
    {
        string value;
        if (this->HasKey(section, key))
        {
            value = this->GetValue(section, key, delimeter);
        }
        else
        {
            value = "";
        }

        vector<string> list;
        split(value, delimeter, list);
        this->m_string_lists[unique_key] = list;
        return this->m_string_lists[unique_key];
    }
}

static double converter(string str)
{
    return atof(str.c_str());
}

const vector<double>& Config::GetDoubleList(const string& section, const string& key, const char* delimeter) const
{
    const string unique_key = Config::GenUniqueKey(section, key);
    map<string, vector<double> >::const_iterator iter = this->m_double_lists.find(unique_key);
    if (iter != this->m_double_lists.end())
    {
        return iter->second;
    }
    else
    {
        string value;
        if (this->HasKey(section, key))
        {
            value = this->GetValue(section, key, delimeter);
        }
        else
        {
            value = "";
        }

        vector<string> list;
        split(value, delimeter, list);
        vector<double> double_list;

        transform(list.begin(), list.end(), back_inserter(double_list), converter);

        this->m_double_lists[unique_key] = double_list;
        return this->m_double_lists[unique_key];
    }
}

void Config::Set(const string& section, const string& key, const string& value)
{
    mConfig[section][key] = value;
}
