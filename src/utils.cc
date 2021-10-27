#include "utils.h"

#include <fstream>

#include "logger.h"

std::vector<char> Utils::readFile(const std::string& file_name)
{
  std::ifstream file(file_name, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    LOG_WARNING("Utils", "Failed opening file: %s", file_name.c_str());
    return std::vector<char>();
  }

  size_t file_size = (size_t) file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();

  return buffer;
}
