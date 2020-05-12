#include <regex>
#include <string>
#include <iostream>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>

using namespace std;

int main() {
  std::string s = "bytes=0-50, 100-150";
  auto re_first_range = std::regex(R"(bytes=(\d*-\d*(?:,\s*\d*-\d*)*))");
  std::smatch m;
  if (std::regex_match(s, m, re_first_range)) {
    auto pos = static_cast<size_t>(m.position(1));
    auto len = static_cast<size_t>(m.length(1));
    cout << pos << ' ' << len;
  } else {
    cout << "No match" << endl;
  }
}