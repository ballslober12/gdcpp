#include "level_parser.h"
#include <zlib.h>
#include <cstdlib>
#include <cstring>

const ObjDef* getObjDef(int id) {
  static std::unordered_map<int, ObjDef> table;
  static bool built = false;
  if (!built) { buildObjectTable(table); built = true; }
  auto it = table.find(id);
  return it == table.end() ? nullptr : &it->second;
}

static std::vector<std::string> split(const std::string& s, char c) {
  std::vector<std::string> out;
  size_t st = 0;
  for (;;) {
    size_t p = s.find(c, st);
    if (p == std::string::npos) { out.push_back(s.substr(st)); break; }
    out.push_back(s.substr(st, p - st));
    st = p + 1;
  }
  return out;
}

// helperFn16
static bool parseItem(const std::string& str, LevelItem& it) {
  std::vector<std::string> v = split(str, ',');
  std::map<int, std::string> o;
  for (size_t i = 0; i + 1 < v.size(); i += 2) o[(int)std::strtol(v[i].c_str(), nullptr, 10)] = v[i + 1];
  auto get = [&](int k, const char* d) -> std::string {
    auto f = o.find(k);
    return (f == o.end() || f->second.empty()) ? std::string(d) : f->second;  // JS `||`
  };
  int id = (int)std::strtol(get(1, "0").c_str(), nullptr, 10);
  if (id == 0) return false;
  it.id = id;
  it.x = std::strtod(get(2, "0").c_str(), nullptr);
  it.y = std::strtod(get(3, "0").c_str(), nullptr);
  it.flipX = o.count(4) && o[4] == "1";
  it.flipY = o.count(5) && o[5] == "1";
  it.rot = std::strtod(get(6, "0").c_str(), nullptr);
  it.scale = std::strtod(get(32, "1").c_str(), nullptr);
  it.zLayer = (int)std::strtol(get(24, "0").c_str(), nullptr, 10);
  it.zOrder = (int)std::strtol(get(25, "0").c_str(), nullptr, 10);
  it.groups = get(57, "");
  it.color1 = (int)std::strtol(get(21, "0").c_str(), nullptr, 10);
  it.color2 = (int)std::strtol(get(22, "0").c_str(), nullptr, 10);
  it.raw = std::move(o);
  return true;
}

static bool decodeLevel(const std::string& text, std::string& out) {
  // trim, url-safe base64 -> bytes
  size_t a = 0, b = text.size();
  while (a < b && (unsigned char)text[a] <= ' ') a++;
  while (b > a && (unsigned char)text[b - 1] <= ' ') b--;
  std::vector<unsigned char> bytes;
  unsigned val = 0; int bits = 0;
  for (size_t i = a; i < b; i++) {
    char c = text[i];
    int d;
    if (c >= 'A' && c <= 'Z') d = c - 'A';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
    else if (c >= '0' && c <= '9') d = c - '0' + 52;
    else if (c == '+' || c == '-') d = 62;
    else if (c == '/' || c == '_') d = 63;
    else continue;  // '=' padding
    val = (val << 6) | d; bits += 6;
    if (bits >= 8) { bits -= 8; bytes.push_back((val >> bits) & 255); }
  }
  // pako.inflate auto-detects gzip / zlib headers
  z_stream zs; std::memset(&zs, 0, sizeof zs);
  if (inflateInit2(&zs, 15 + 32) != Z_OK) return false;
  zs.next_in = bytes.data(); zs.avail_in = (uInt)bytes.size();
  unsigned char buf[65536];
  int rc;
  do {
    zs.next_out = buf; zs.avail_out = sizeof buf;
    rc = inflate(&zs, Z_NO_FLUSH);
    if (rc != Z_OK && rc != Z_STREAM_END) { inflateEnd(&zs); return false; }
    out.append((char*)buf, sizeof buf - zs.avail_out);
  } while (rc != Z_STREAM_END && (zs.avail_in > 0 || zs.avail_out == 0));
  inflateEnd(&zs);
  return true;
}

bool parseLevelString(const std::string& levelText, std::vector<LevelItem>& out) {
  std::string s;
  if (!decodeLevel(levelText, s)) return false;
  std::vector<std::string> parts = split(s, ';');
  for (size_t i = 1; i < parts.size(); i++) {
    if (parts[i].empty()) continue;
    LevelItem it;
    if (parseItem(parts[i], it)) out.push_back(std::move(it));
  }
  return true;
}
