#pragma once

#include "../core/Storage.h"
#include <vector>
#include <string>

class TeensyStorage : public Storage {
public:
  TeensyStorage(int csPin = -1);
  virtual ~TeensyStorage();

  bool begin() override;
  bool exists(const std::string &path) override;
  bool mkdir(const std::string &path) override;
  bool remove(const std::string &path) override;
  bool writeLines(const std::string &path, const std::vector<std::string> &lines) override;
  bool readLines(const std::string &path, std::vector<std::string> &outLines) override;
  bool listDir(const std::string &path, std::vector<std::string> &outEntries) override;

private:
  int csPin;
};
