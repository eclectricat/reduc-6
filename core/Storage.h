#pragma once

#include <string>
#include <vector>

class Storage {
public:
  virtual ~Storage() {}
  virtual bool begin() = 0;
  virtual bool exists(const std::string &path) = 0;
  virtual bool mkdir(const std::string &path) = 0;
  virtual bool remove(const std::string &path) = 0;
  virtual bool writeLines(const std::string &path, const std::vector<std::string> &lines) = 0;
  virtual bool readLines(const std::string &path, std::vector<std::string> &outLines) = 0;
  virtual bool listDir(const std::string &path, std::vector<std::string> &outEntries) = 0;
};
