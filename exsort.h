#pragma once

#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

// External merge sort for int32_t
class MergeSorter {
public:
  MergeSorter(std::size_t ramSize, std::size_t minBlockSize)
      : ramSize(ramSize), minBlockSize(minBlockSize) {}

  void SortFile(fs::path input, fs::path output,
                fs::path workdir = fs::temp_directory_path());

private:
  void SortInRAM(fs::path input, fs::path output);

  void SortBlock(std::ifstream &in, fs::path out);

  int32_t ReadValue(std::ifstream &in) {
    int32_t value;
    in.read(reinterpret_cast<char *>(&value), 4);
    return value;
  }

  int ReadN(std::ifstream &in, int32_t *array, std::size_t len) {
    in.read(reinterpret_cast<char *>(array), len);
    return in.gcount();
  }

  struct FileValue {
    int32_t value;
    int index;

    friend auto operator<=>(const FileValue &a, const FileValue &b) {
      return a.value <=> b.value;
    }
  };

  int MergeFiles(fs::directory_iterator &it, std::size_t batchSize,
                 std::string outName);

  std::vector<int32_t> ReadValueBatch(std::ifstream &is, size_t ioSize);

  std::size_t ramSize;
  std::size_t minBlockSize;
  std::size_t fileCount{0};
};