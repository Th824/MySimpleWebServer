#include <regex>
#include <string>
#include <iostream>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>

using namespace std;

int main() {
  // std::regex re(R"(([^:]+):[\t ]*(.+))");
  // std::cmatch m;
  // std::string str(
  //     "Content-Type: multipart/form-data; boundary=---------------------------974767299852498929531610575");
  // const char* begin = &str[0], *end = &str[str.length() - 1];
  // if (std::regex_match(begin, end, m, re)) {
  //   auto key = std::string(m[1]);
  //   auto val = std::string(m[2]);
  //   std::cout << key << ' ' << val << std::endl;
  // }
  char buffer[1000];
  getcwd(buffer, 1000);
  cout << string(buffer) << endl;
}