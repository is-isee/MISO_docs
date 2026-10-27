// practice/execution/02_reduction の解答例
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

#include <miso/array1d.hpp>
#include <miso/execution.hpp>
#include <miso/utility.hpp>

using namespace miso;

#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif

void check(const int n) {
  Array1D<double, Backend> x(n);
  auto xv = x.view();
  for_each(Backend{}, Range1D{0, n},
           MISO_LAMBDA(int i) { xv[i] = util::sin(0.001 * i); });

  // 二乗和：各点の値は x^2、まとめ方は足し算、初期値は足し算の単位元 0
  const double s = reduce(
      Backend{}, Range1D{0, n}, 0.0, MISO_LAMBDA(int i) { return xv[i] * xv[i]; },
      MISO_LAMBDA(double a, double b) { return a + b; });

  // 最大値：まとめ方は max、初期値は max の単位元（double の最小の値）
  // util::max2 は double を float に丸めるので使わない（MISO #189）
  const double lowest = std::numeric_limits<double>::lowest();
  const double m = reduce(
      Backend{}, Range1D{0, n}, lowest, MISO_LAMBDA(int i) { return xv[i]; },
      MISO_LAMBDA(double a, double b) { return util::fmax(a, b); });

  // 期待値は、同じ配列をホストへ写して通常のループで求める
  Array1D<double, backend::Host> h(n);
  h.copy_from(x);
  long double s_ref = 0.0L;
  double m_ref = h[0];
  for (int i = 0; i < n; ++i) {
    s_ref += static_cast<long double>(h[i]) * static_cast<long double>(h[i]);
    m_ref = std::max(m_ref, h[i]);
  }
  const double s_err = std::fabs(s - static_cast<double>(s_ref));
  const double s_tol = 1e-12 * std::max(1.0, std::fabs(static_cast<double>(s_ref)));
  std::printf("N = %d: sum = %.15g (diff %g, %s), max = %.17g (%s)\n", n, s, s_err,
              s_err <= s_tol ? "ok" : "NG", m, m == m_ref ? "exact" : "NG");
}

int main() {
  check(1000);
  check(1000000);
  return 0;
}
