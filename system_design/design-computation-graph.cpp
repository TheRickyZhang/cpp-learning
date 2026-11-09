#include <bits/stdc++.h>
using namespace std;

// // Not necessary
// enum class Op {
//   ADD,
//   SUB,
//   MUL,
//   DIV
// };
//
// constexpr int apply(int x, int y, Op op) {
//   switch(op) {
//     case Op::ADD: return x + y;
//     case Op::SUB: return x - y;
//     case Op::MUL: return x * y;
//     case Op::DIV: return x / y;
//     std::unreachable();
//   }
// }

struct Cell {
  vector<int> deps;
  int val = 0;
  bool stale = false;
};

struct Table {
  int n, m;
  int sz;
  vector<Cell> cells;
  vector<vector<int>> down, up;
  Table(int n, int m) :
    n(n), m(m), sz(n * m),
    cells(sz), down(sz), up(sz) { }

  void setValue(int id, int x) {
    clearDeps(id);
    cells[id].val = x;
    cells[id].stale = false;
    markAboveStale(id);
  }

  bool setFormula(int id, const vector<int>& deps) {
    if(createsCycle(id, deps)) return false;

    clearDeps(id);
    down[id] = deps;
    for(int v : deps) up[v].push_back(id);
    cells[id].stale = true;
    markAboveStale(id);
    return true;
  }

  int get(int id) {
    auto dfs = [&](auto&& self, int u) {
      Cell& c = cells[u];
      if(!c.stale) return c.val;
      c.stale = false;
      if(down[u].empty()) {
        return c.val;
      }
      int res = 0;
      for(int v : down[u]) {
        res += self(self, v);
      }
      return c.val = res;
    };
    return dfs(dfs, id);
  }

private:
  void markAboveStale(int u) {
    // mark everything above stale 
    stack<int> s;
    for(int v : up[u]) {
      s.push(v);
    }
    unordered_set<int> seen;
    while(!s.empty()) {
      int u = s.top(); s.pop();
      if(seen.count(u)) continue;
      cells[u].stale = true;
      for(int v : up[u]) s.push(v);
    }
  }

  void clearDeps(int id) {
    for(int v : down[id]) erase(up[v], id);
    down[id].clear();
  }

  bool createsCycle(int id, const vector<int>& deps) {
    unordered_set<int> seen;
    vector<int> s(deps.begin(), deps.end());
    while(!s.empty()) {
      int u = s.back(); s.pop_back();
      if(u == id) return true;
      if(!seen.insert(u).second) continue;
      for(int v : down[u]) s.push_back(v);
    }
    return false;
  }
};

int main() {
  Table t(10, 1); 
  t.setValue(2, 2);
  t.setValue(3, 2);
  t.setFormula(1, {2, 3});
  t.setFormula(0, {1, 3});
  cout << t.get(2) << endl;
  cout << t.get(1) << endl;
  cout << t.get(0) << endl;
}
