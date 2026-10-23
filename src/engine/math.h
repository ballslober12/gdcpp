// Small math helpers that mirror the Phaser/JS semantics the game relies on.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace px {

constexpr double PI = 3.14159265358979323846;
inline double rad(double deg) { return deg * PI / 180.0; }
inline double clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Math.random()
inline std::mt19937_64& rng() {
  static std::mt19937_64 g(std::random_device{}());
  return g;
}
inline double random01() {
  return std::uniform_real_distribution<double>(0.0, 1.0)(rng());
}
inline double floatBetween(double a, double b) { return a + (b - a) * random01(); }

// Phaser TransformMatrix layout: x' = a*x + c*y + e ; y' = b*x + d*y + f
struct Mat {
  double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
  static Mat TRS(double x, double y, double rot, double sx, double sy) {
    double cs = std::cos(rot), sn = std::sin(rot);
    Mat m;
    m.a = cs * sx; m.b = sn * sx; m.c = -sn * sy; m.d = cs * sy; m.e = x; m.f = y;
    return m;
  }
  // result = this * o (o is applied first)
  Mat operator*(const Mat& o) const {
    Mat r;
    r.a = a * o.a + c * o.b;
    r.b = b * o.a + d * o.b;
    r.c = a * o.c + c * o.d;
    r.d = b * o.c + d * o.d;
    r.e = a * o.e + c * o.f + e;
    r.f = b * o.e + d * o.f + f;
    return r;
  }
  void apply(double x, double y, double& ox, double& oy) const {
    ox = a * x + c * y + e;
    oy = b * x + d * y + f;
  }
  bool invert(Mat& out) const {
    double det = a * d - b * c;
    if (std::fabs(det) < 1e-12) return false;
    double id = 1.0 / det;
    out.a = d * id; out.b = -b * id; out.c = -c * id; out.d = a * id;
    out.e = (c * f - d * e) * id;
    out.f = (b * e - a * f) * id;
    return true;
  }
};

}  // namespace px
