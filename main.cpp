#include "exsort.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>

class ArgsParser {
public:
  ArgsParser(int argc, char **argv) {
    for (int i = 1; i < argc; i += 2) {
      std::string arg(argv[i]), val(argv[i + 1]);
      // std::cout << arg << " " << val << std::endl;
      args.emplace(arg, val);
    }
  }

  std::optional<std::string> GetArg(const std::string &name) {
    if (!args.contains(name)) {
      return std::nullopt;
    }
    return args[name];
  }

private:
  std::unordered_map<std::string, std::string> args;
};

void generateTest(fs::path op, int num) {
  std::ofstream o("test");
  std::random_device rd;  // a seed source for the random number engine
  std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<int32_t> distrib(0, 100'000);
  for (int i = 0; i < num; ++i) {
    int32_t val = distrib(gen);
    o.write(reinterpret_cast<char *>(&val), 4);
  }
}

void testFile(fs::path p) {
  auto read = [](int32_t &v, std::ifstream &is) {
    is.read(reinterpret_cast<char *>(&v), sizeof(int32_t));
    // std::cout << v << " ";
  };

  std::ifstream in(p);
  int32_t value = 0;
  in.read(reinterpret_cast<char *>(&value), sizeof(int32_t));

  while (!in.eof()) {
    int32_t t;
    read(t, in);
    if (t < value) {
      throw std::runtime_error("Wrong sort order");
    } else {
      value = t;
    }
  }
}

namespace fs = std::filesystem;

void printFile(fs::path f, int count) {
  std::ifstream is(f, std::ios::binary);
  for (int i = 0; i < count; ++i) {
    int32_t value = 0;
    is.read(reinterpret_cast<char *>(&value), sizeof(int32_t));
    std::cout << value << " ";
  }
  std::cout << std::endl;
}

int main(int argc, char **argv) {

  ArgsParser parser(argc, argv);
  size_t ramSize = 500 * 1024 * 1024; // Bytes (500 MB)
  if (auto rs = parser.GetArg("--ram"); rs.has_value()) {
    ramSize = std::stoll(*rs);
  }
  size_t minIO = 50 * 1024 * 1024; // Bytes (50MB)
  if (auto rs = parser.GetArg("--io"); rs.has_value()) {
    minIO = std::stoll(*rs);
  }
  fs::path in;
  if (auto rs = parser.GetArg("--in"); rs.has_value()) {
    in = *rs;
  } else {
    throw std::runtime_error("Input file required");
  }
  fs::path out;
  if (auto rs = parser.GetArg("--out"); rs.has_value()) {
    out = *rs;
  } else {
    throw std::runtime_error("Out file required");
  }
  std::optional<std::string> workdir = parser.GetArg("--workdir");

  MergeSorter sorter(ramSize, minIO);

  if (workdir.has_value()) {
    sorter.SortFile(in, out, *workdir);
  } else {
    sorter.SortFile(in, out);
  }
  testFile(out);
  std::cout << "Sorted successfully" << std::endl;
  // generateTest("test", 100'000);
}