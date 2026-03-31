// Minimal main that reads a regex over {a,b} and then Q strings
// Prints YES/NO per line depending on acceptance
#include <iostream>
#include <string>
#include "src/regex.hpp"
using namespace std;
using namespace Grammar;

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  string pattern;
  if (!(cin >> pattern)) return 0;
  int Q;
  if (!(cin >> Q)) Q = 0;
  RegexChecker checker(pattern);
  while (Q--) {
    string s;
    cin >> s;
    cout << (checker.Check(s) ? "YES" : "NO") << '\n';
  }
  return 0;
}
