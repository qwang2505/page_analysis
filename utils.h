#ifndef _URL_PARSE_H_
#define _URL_PARSE_H_
#include <string>
#include <vector>

using namespace std;

struct ParseResult
{
public:
    ParseResult()
    {
    }

    ParseResult(const char* protocol, const char* host, const char* path, const char* query) :
        protocol(protocol), host(host), path(path), query(query)
    {
    }

    std::string protocol, host, path, query;
};

ParseResult urlparse(const string& url);
string get_basename(const string& file_path);
void split(const string& str, const char* delimeter, vector<string>& segments);

// pattern: 0: str startswith any
// pattern: 1: str full matches any
// pattern: 2: str contains any
// pattern: 3: str endswith any
int match_list(const char* str, const vector<string>& string_list, int pattern = 0);
int count_without_spaces(const char* str);
#endif
