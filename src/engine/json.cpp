#include "json.h"
#include <cstdlib>

namespace px {
namespace {
struct P {
  const std::string& s;
  size_t i = 0;
  explicit P(const std::string& str) : s(str) {}
  void ws() { while (i < s.size() && (unsigned char)s[i] <= ' ') i++; }
  std::string str() {
    std::string o;
    i++;  // opening quote
    while (i < s.size() && s[i] != '"') {
      if (s[i] == '\\' && i + 1 < s.size()) {
        i++;
        char c = s[i];
        if (c == 'n') o += '\n';
        else if (c == 't') o += '\t';
        else if (c == 'u' && i + 4 < s.size()) { o += '?'; i += 4; }
        else o += c;
      } else o += s[i];
      i++;
    }
    i++;
    return o;
  }
  Json val() {
    ws();
    Json j;
    if (i >= s.size()) return j;
    char c = s[i];
    if (c == '{') {
      j.type = Json::Obj; i++; ws();
      if (s[i] == '}') { i++; return j; }
      for (;;) {
        ws(); std::string k = str(); ws(); i++;  // ':'
        j.obj[k] = val(); ws();
        if (s[i] == ',') { i++; continue; }
        i++; break;
      }
    } else if (c == '[') {
      j.type = Json::Arr; i++; ws();
      if (s[i] == ']') { i++; return j; }
      for (;;) {
        j.arr.push_back(val()); ws();
        if (s[i] == ',') { i++; continue; }
        i++; break;
      }
    } else if (c == '"') {
      j.type = Json::Str; j.str = str();
    } else if (s.compare(i, 4, "true") == 0) { j.type = Json::Bool; j.b = true; i += 4; }
    else if (s.compare(i, 5, "false") == 0) { j.type = Json::Bool; i += 5; }
    else if (s.compare(i, 4, "null") == 0) { i += 4; }
    else { j.type = Json::Num; char* e; j.num = std::strtod(s.c_str() + i, &e); i = e - s.c_str(); }
    return j;
  }
};
}  // namespace
Json Json::parse(const std::string& s) { P p(s); return p.val(); }
}  // namespace px
