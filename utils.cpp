#include "utils.h"

#include <cstring>
#include <algorithm>
#include <cctype>
#include <functional>
#include <vector>

using namespace std;

ParseResult urlparse(const string& url)
{
    ParseResult result;
    const string prot_end("://");
    string::const_iterator prot_i = search(url.begin(), url.end(), prot_end.begin(), prot_end.end());
    result.protocol.reserve(distance(url.begin(), prot_i));
    transform(url.begin(), prot_i, back_inserter(result.protocol), ptr_fun<int,int>(tolower));
    if(prot_i == url.end())
        return result;

    advance(prot_i, prot_end.length());
    string::const_iterator path_i = find(prot_i, url.end(), '/');
    result.host.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i, back_inserter(result.host), ptr_fun<int,int>(tolower));

    string::const_iterator query_i = find(path_i, url.end(), '?');
    result.path.assign(path_i, query_i);
    if(query_i != url.end())
        ++query_i;
    result.query.assign(query_i, url.end());
    return result;
}

string get_basename(const string& file_path)
{
    size_t pos = file_path.find_last_of('/');
    if (pos != string::npos)
    {
        return file_path.substr(pos + 1);
    }
    else
    {
        return file_path;
    }
}

void split(const string& str, const char* delimeter, vector<string>& segments)
{
    char* buffer = new char[str.length() + 1];
    strncpy(buffer, str.c_str(), str.length());
    buffer[str.length()] = '\0';
    char* ptr = buffer;
    char* saveptr;
    char* token;
    while ((token = strtok_r(ptr, delimeter, &saveptr)) != NULL)
    {
        segments.push_back(string(token));
        ptr = saveptr;
    }

    delete[] buffer;
}

int match_list(const char* str, const vector<string>& string_list, int pattern)
{
    size_t len = strlen(str);
    for (int i = 0; i < static_cast<int>(string_list.size()); ++i)
    {
        switch (pattern)
        {
        case 0: 
            if (strncmp(str, string_list[i].c_str(), string_list[i].length()) == 0)
            {
                return i;
            }

            break;
        case 1:
            if (len == string_list[i].length() && strncmp(str, string_list[i].c_str(), string_list[i].length()) == 0)
            {
                return i;
            }

            break;
        case 2:
            if (string(str).find(string_list[i]) != string::npos)
            {
                return i;
            }

            break;
        case 3:
            if (len >= string_list[i].length() && strncmp(str + len - string_list[i].length(), string_list[i].c_str(), string_list[i].length()) == 0)
            {
                return i;
            }

            break;
        default:
            return -1;
        }
    }

    return -1;
}

static const string g_spaces = std::string(" \r\t\n\x0c\x0d");

int count_without_spaces(const char* str)
{
    int count = 0;
    while (*str != '\0')
    {
        if ((unsigned char)(*str) <= 0xFF)
        {
            if (g_spaces.find_first_of(*str) == std::string::npos)
            {
                count++;
            }
        }
        else
        {
            //We didn't consider encoding of the str, and space chars such as 0xa0 and \u3000 can't be skipped
            count++;
        }

        str++;
    }

    return count;
}
