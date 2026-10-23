// Minimal JSON reader (enough for the TexturePacker atlas).
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace px {
struct Json {
  enum Type { Null, Bool, Num, Str, Arr, Obj } type = Null;
  double num = 0;
  bool b = false;
  std::string str;
  std::vector<Json> arr;
  std::map<std::string, Json> obj;
  const Json& operator[](const std::string& k) const {
    static Json n;
    auto it = obj.find(k);
    return it == obj.end() ? n : it->second;
  }
  bool has(const std::string& k) const { return obj.count(k) > 0; }
  static Json parse(const std::string& s);
};
}  // namespace px
