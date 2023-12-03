#include "exsort.h"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <ranges>
#include <set>
#include <string>
#include <vector>

void MergeSorter::SortFile(fs::path input, fs::path output, fs::path workdir) {

  auto inputSize = fs::file_size(input);
  if (inputSize < ramSize) {
    SortInRAM(input, output);
    return;
  }

  std::deque<fs::path> files;
  std::cout << "Sorting file: " << input << std::endl;

  // 1. Sort blocks in RAM
  auto is = std::ifstream(input, std::ios::binary);
  while (!is.eof()) {
    auto path = workdir / std::to_string(fileCount);
    ++fileCount;
    files.push_back(path);
    std::cout << "Sorting block #" << fileCount << std::endl;
    SortBlock(is, path);
  }

  std::cout << "\nTotal blocks sorted: " << fileCount << std::endl;

  // 2. Merge files efficiently
  while (fileCount > 1) {
    fs::directory_iterator iter(workdir);
    std::size_t batchSize = std::max<std::size_t>(
        std::min<std::size_t>(std::sqrt(fileCount), ramSize / minBlockSize), 2);

    while (iter != fs::directory_iterator{} && fileCount > 1) {
      auto filesMerged =
          MergeFiles(iter, batchSize, iter->path().string() + "_");
      fileCount -= filesMerged - 1;
    }
    std::cout << "File count: " << fileCount << std::endl;
  }
  fs::directory_iterator iter(workdir);
  fs::copy(iter->path(), output);
}

void MergeSorter::SortInRAM(fs::path input, fs::path output) {
  auto is = std::ifstream(input, std::ios::binary);
  std::vector<int32_t> values;
  while (!is.eof()) {
    values.push_back(ReadValue(is));
  }

  std::ranges::sort(values.begin(), values.end());

  auto os = std::ofstream(output);
  os.write(reinterpret_cast<char *>(values.data()),
           values.size() * sizeof(int32_t));
}

void MergeSorter::SortBlock(std::ifstream &in, fs::path out) {
  std::size_t elemCount = ramSize / sizeof(int32_t);
  std::vector<int32_t> values;
  auto os = std::ofstream(out, std::ios::binary);

  for (int i = 0; i < elemCount && !in.eof(); ++i) {
    values.push_back(ReadValue(in));
  }

  std::ranges::sort(values.begin(), values.end());

  os.write(reinterpret_cast<char *>(values.data()),
           values.size() * sizeof(int32_t));
}

int MergeSorter::MergeFiles(fs::directory_iterator &it, std::size_t batchSize,
                            std::string outName) {
  std::vector<std::ifstream> files;
  std::vector<fs::path> paths;
  for (auto i = 0; i < batchSize && it != fs::directory_iterator{}; ++i, ++it) {
    files.emplace_back(it->path(), std::ios::binary);
    paths.push_back(it->path());
  }
  std::cout << "Merging files from: " << paths.front()
            << " To: " << paths.back() << std::endl;
  std::multiset<FileValue> values;
  auto ioSize = ramSize / batchSize;

  std::vector<std::vector<int32_t>> ranges(files.size());
  std::vector<std::vector<int32_t>::iterator> iters;
  for (auto i = 0; i < ranges.size(); ++i) {
    ranges[i] = ReadValueBatch(files[i], ioSize);
    iters.push_back(ranges[i].begin());
    values.emplace(*(iters[i]), i);
    ++iters[i];
  }

  std::ofstream out(outName, std::ios::binary);
  while (!values.empty()) {
    // TODO change to batch writing
    auto val = *values.begin();
    out.write(reinterpret_cast<char *>(&val.value), sizeof(int32_t));
    values.erase(values.begin());
    if (iters[val.index] != ranges[val.index].end()) {
      values.emplace(*iters[val.index], val.index);
      ++iters[val.index];
    }
  }

  for (const auto &p : paths) {
    fs::remove(p);
  }
  std::cout << "Merged successfully\n";
  return files.size();
}

std::vector<int32_t> MergeSorter::ReadValueBatch(std::ifstream &is,
                                                 size_t ioSize) {
  char data[ioSize * sizeof(int32_t)];
  is.read(data, ioSize * sizeof(int32_t));
  return std::vector<int32_t>(reinterpret_cast<int32_t *>(data),
                              reinterpret_cast<int32_t *>(data) + ioSize);
}