#include "TeensyStorage.h"
#include <SD.h>
#include <Arduino.h>

TeensyStorage::TeensyStorage(int csPin_) : csPin(csPin_) {}
TeensyStorage::~TeensyStorage() {}

bool TeensyStorage::begin() {
  #ifdef BUILTIN_SDCARD
  if (csPin == -1) return SD.begin(BUILTIN_SDCARD);
  #endif
  return SD.begin(csPin);
}

bool TeensyStorage::exists(const std::string &path) {
  return SD.exists(path.c_str());
}

bool TeensyStorage::mkdir(const std::string &path) {
  return SD.mkdir(path.c_str());
}

bool TeensyStorage::remove(const std::string &path) {
  return SD.remove(path.c_str());
}

bool TeensyStorage::writeLines(const std::string &path, const std::vector<std::string> &lines) {
  File f = SD.open(path.c_str(), FILE_WRITE);
  if (!f) return false;
  for (const auto &ln : lines) {
    f.print(ln.c_str());
    f.print('\n');
  }
  f.close();
  return true;
}

bool TeensyStorage::readLines(const std::string &path, std::vector<std::string> &outLines) {
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) return false;
  std::string line;
  while (f.available()) {
    int c = f.read();
    if (c < 0) break;
    if (c == '\r') continue;
    if (c == '\n') {
      outLines.push_back(line);
      line.clear();
      continue;
    }
    line.push_back((char)c);
  }
  if (!line.empty()) outLines.push_back(line);
  f.close();
  return true;
}

bool TeensyStorage::listDir(const std::string &path, std::vector<std::string> &outEntries) {
  File dir = SD.open(path.c_str());
  if (!dir) return false;
  File entry = dir.openNextFile();
  while (entry) {
    outEntries.emplace_back(entry.name());
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return true;
}
