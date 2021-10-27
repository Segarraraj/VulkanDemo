#ifndef __UTILS_H__
#define __UTILS_H__ 1

#include <vector>
#include <string>

class Utils
{
public:
  static std::vector<char> readFile(const std::string& file_name);
};

#endif // __UTILS_H__
