// Implementation of a simple NFA-based regex engine for alphabet {a,b}
// Supported operators: concatenation, '+', '*', '?', and '|' (lowest precedence)
// Do not include extra headers beyond those allowed by the assignment.

#pragma once
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

namespace Grammar {

class NFA;
NFA MakeStar(const char &character);
NFA MakePlus(const char &character);
NFA MakeQuestion(const char &character);
NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
NFA Union(const NFA &nfa1, const NFA &nfa2);
NFA MakeSimple(const char &character);

enum class TransitionType { Epsilon, a, b };

struct Transition {
  TransitionType type;
  int to;
  Transition(TransitionType type, int to) : type(type), to(to) {}
};

class NFA {
private:
  int start{};
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;

public:
  NFA() = default;
  ~NFA() = default;

  std::unordered_set<int> GetEpsilonClosure(std::unordered_set<int> states) const {
    std::unordered_set<int> closure;
    std::queue<int> q;
    for (const auto &s : states) {
      if (closure.find(s) != closure.end()) continue;
      q.push(s);
      closure.insert(s);
    }
    while (!q.empty()) {
      int cur = q.front();
      q.pop();
      if (cur < 0 || cur >= (int)transitions.size()) continue;
      for (const auto &tr : transitions[cur]) {
        if (tr.type == TransitionType::Epsilon) {
          if (closure.find(tr.to) == closure.end()) {
            q.push(tr.to);
            closure.insert(tr.to);
          }
        }
      }
    }
    return closure;
  }

  std::unordered_set<int> Advance(std::unordered_set<int> current_states, char character) const {
    // Compute epsilon-closure of current states
    std::unordered_set<int> closure = GetEpsilonClosure(current_states);
    // Move via character edges
    std::unordered_set<int> dest;
    TransitionType want = (character == 'a') ? TransitionType::a : TransitionType::b;
    for (int s : closure) {
      if (s < 0 || s >= (int)transitions.size()) continue;
      for (const auto &tr : transitions[s]) {
        if (tr.type == want) {
          dest.insert(tr.to);
        }
      }
    }
    // Epsilon-closure of destinations
    return GetEpsilonClosure(dest);
  }

  bool IsAccepted(int state) const { return ends.find(state) != ends.end(); }

  int GetStart() const { return start; }

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
};

// a* or b*
NFA MakeStar(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.transitions.resize(1);
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 0});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 0});
  }
  return nfa;
}

// a+ or b+
NFA MakePlus(const char &character) {
  NFA nfa;
  // states: 0(start) ->(char)-> 1(accept), and 1 loops on char
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.resize(2);
  TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
  nfa.transitions[0].push_back({t, 1});
  nfa.transitions[1].push_back({t, 1});
  return nfa;
}

// a? or b?
NFA MakeQuestion(const char &character) {
  NFA nfa;
  // states: 0(start, accept) --(char)--> 1(accept)
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.ends.insert(1);
  nfa.transitions.resize(2);
  TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
  nfa.transitions[0].push_back({t, 1});
  return nfa;
}

// Concatenate nfa1 then nfa2
NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  // Copy nfa1 transitions
  int size1 = (int)nfa1.transitions.size();
  int size2 = (int)nfa2.transitions.size();
  res.start = nfa1.start;
  res.transitions = nfa1.transitions;
  // Ensure size for combined states
  res.transitions.resize(size1 + size2);

  // Copy nfa2 transitions with offset
  for (int i = 0; i < size2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[i + size1].push_back({tr.type, tr.to + size1});
    }
  }
  // Ends are nfa2 ends offset by size1
  for (int e : nfa2.ends) res.ends.insert(e + size1);
  // Connect each end of nfa1 to nfa2.start via epsilon
  for (int e1 : nfa1.ends) {
    // Make sure e1 exists in transitions
    if (e1 >= 0 && e1 < (int)res.transitions.size()) {
      res.transitions[e1].push_back({TransitionType::Epsilon, nfa2.start + size1});
    }
  }
  return res;
}

// Alternation nfa1 | nfa2
NFA Union(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int size1 = (int)nfa1.transitions.size();
  int size2 = (int)nfa2.transitions.size();

  // New start at 0, then nfa1 shifted by 1, nfa2 shifted by 1+size1
  res.start = 0;
  int off1 = 1;
  int off2 = 1 + size1;

  res.transitions.clear();
  res.transitions.resize(1 + size1 + size2);

  // Epsilon edges from new start to both original starts
  res.transitions[0].push_back({TransitionType::Epsilon, off1 + nfa1.start});
  res.transitions[0].push_back({TransitionType::Epsilon, off2 + nfa2.start});

  // Copy nfa1 transitions
  for (int i = 0; i < size1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      res.transitions[off1 + i].push_back({tr.type, off1 + tr.to});
    }
  }
  // Copy nfa2 transitions
  for (int i = 0; i < size2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[off2 + i].push_back({tr.type, off2 + tr.to});
    }
  }

  // Ends: union of ends with offsets
  for (int e : nfa1.ends) res.ends.insert(off1 + e);
  for (int e : nfa2.ends) res.ends.insert(off2 + e);
  return res;
}

// Single character a or b
NFA MakeSimple(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.resize(2);
  TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
  nfa.transitions[0].push_back({t, 1});
  return nfa;
}

class RegexChecker {
private:
  NFA nfa;

  static bool IsLetter(char c) { return c == 'a' || c == 'b'; }
  static bool IsQuant(char c) { return c == '*' || c == '+' || c == '?'; }

  // Build NFA for a concatenation string with only letters and postfix quantifiers, no '|'
  static NFA BuildConcat(const std::string &s) {
    bool has_any = false;
    NFA cur; // placeholder
    for (size_t i = 0; i < s.size(); ++i) {
      char c = s[i];
      if (!IsLetter(c)) continue; // skip unexpected
      NFA atom;
      if (i + 1 < s.size() && IsQuant(s[i + 1])) {
        char q = s[i + 1];
        if (q == '*') atom = MakeStar(c);
        else if (q == '+') atom = MakePlus(c);
        else /* q == '?' */ atom = MakeQuestion(c);
        ++i; // consume quantifier
      } else {
        atom = MakeSimple(c);
      }
      if (!has_any) {
        cur = atom;
        has_any = true;
      } else {
        cur = Concatenate(cur, atom);
      }
    }
    if (!has_any) {
      // As per problem statement, inputs won't include empty alternatives.
      // Build a trivial NFA that matches nothing meaningful is undesirable; fallback to MakeSimple('a').
      // However, test data should avoid this branch.
      return MakeSimple('a');
    }
    return cur;
  }

public:
  bool Check(const std::string &str) const {
    std::unordered_set<int> states;
    states.insert(nfa.GetStart());
    for (char ch : str) {
      if (ch != 'a' && ch != 'b') {
        // Invalid character, reject
        return false;
      }
      states = nfa.Advance(states, ch);
      if (states.empty()) return false; // early cut if dead
    }
    // Final epsilon-closure before acceptance check
    states = nfa.GetEpsilonClosure(states);
    for (int s : states) if (nfa.IsAccepted(s)) return true;
    return false;
  }

  RegexChecker(const std::string &regex) {
    // Split by '|' and union all alternatives
    std::vector<std::string> alts;
    std::string cur;
    for (char c : regex) {
      if (c == '|') {
        alts.push_back(cur);
        cur.clear();
      } else {
        cur.push_back(c);
      }
    }
    alts.push_back(cur);

    if (alts.empty()) {
      nfa = MakeSimple('a');
      return;
    }
    NFA built = BuildConcat(alts[0]);
    for (size_t i = 1; i < alts.size(); ++i) {
      NFA next = BuildConcat(alts[i]);
      built = Union(built, next);
    }
    nfa = built;
  }
};

} // namespace Grammar
