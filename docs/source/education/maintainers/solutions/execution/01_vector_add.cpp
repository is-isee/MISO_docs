// practice/execution/01_vector_add の解答例
#include <algorithm>
#include <cmath>
#include <cstdio>

#include <miso/array1d.hpp>
#include <miso/execution.hpp>

using namespace miso;

#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif

int main() {
  const int n = 1000;
  Array1D<double, Backend> x(n), y(n), z(n);
  auto xv = x.view();  // ラムダには所有しない View を値で渡す
  auto yv = y.view();
  auto zv = z.view();

  for_each(Backend{}, Range1D{0, n}, MISO_LAMBDA(int i) {
    xv[i] = 0.37 * i;
    yv[i] = 1.3 * i;
  });
  for_each(Backend{}, Range1D{0, n}, MISO_LAMBDA(int i) { zv[i] = xv[i] + yv[i]; });

  // 結果をホスト側の配列へ写してから読む（Host でも CUDA でも同じ書き方）
  Array1D<double, backend::Host> h(n);
  h.copy_from(z);

  double max_err = 0.0;
  int n_exact = 0, n_ok = 0;
  for (int i = 0; i < n; ++i) {
    const double xi = 0.37 * i;  // 初期化と同じ式を別々に計算してから足す
    const double yi = 1.3 * i;
    const double expected = xi + yi;
    const double err = std::fabs(h[i] - expected);
    max_err = std::max(max_err, err);
    n_exact += (h[i] == expected);
    n_ok += (err <= 1e-15 * std::max(1.0, std::fabs(expected)));
  }
  std::printf("z[0] = %g, z[999] = %.6f\n", h[0], h[n - 1]);
  std::printf("max abs error = %g, within tolerance = %d/%d, exact = %d/%d\n",
              max_err, n_ok, n, n_exact, n);
  return 0;
}
