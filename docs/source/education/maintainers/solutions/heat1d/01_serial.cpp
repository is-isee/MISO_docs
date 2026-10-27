// 実習 heat1d 01: 1 次元熱伝導の逐次版（解答例）
//   du/dt = kappa d^2u/dx^2,  0 < x < 1,  u(0) = u(1) = 0
//   u(x, 0) = sin(pi x)  ->  u(x, t) = exp(-pi^2 kappa t) sin(pi x)
// 使い方: ./heat1d_serial [N] [r]   (N: セル数, r = kappa dt / dx^2)
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>

#include <miso/array1d.hpp>
#include <miso/backend.hpp>
#include <miso/constants.hpp>
#include <miso/execution.hpp>
#include <miso/utility.hpp>

using namespace miso;
using Real = double;

int main(int argc, char **argv) {
  const int n = (argc > 1) ? std::atoi(argv[1]) : 64;
  const Real r_max = (argc > 2) ? std::atof(argv[2]) : Real(0.4);

  const Real kappa = 1.0;
  const Real t_end = 0.1;
  const Real dx = Real(1) / n;
  const int n_steps =
      static_cast<int>(std::ceil(t_end / (r_max * dx * dx / kappa)));
  const Real dt = t_end / n_steps;
  const Real r = kappa * dt / (dx * dx);

  const int margin = 1;  // ゴーストセルの数（片側）
  const int n_total = n + 2 * margin;

  Array1D<Real> u(n_total);
  Array1D<Real> u_new(n_total);

  // 初期条件: 物理セル i の中心は x = (i - margin + 0.5) dx
  {
    auto uv = u.view();
    for_each(
        backend::Host{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
          const Real x = (i - margin + Real(0.5)) * dx;
          uv[i] = util::sin(pi<Real> * x);
        });
  }

  for (int step = 0; step < n_steps; ++step) {
    auto uv = u.view();
    auto unv = u_new.view();

    // 境界条件 u = 0（壁はセルの面）: ゴーストセルに符号を反転した値
    for_each(
        backend::Host{}, Range1D{0, 1}, MISO_LAMBDA(int) {
          uv[0] = -uv[margin];
          uv[n + margin] = -uv[n + margin - 1];
        });

    // 陽解法 1 ステップ
    for_each(
        backend::Host{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
          unv[i] = uv[i] + r * (uv[i - 1] - Real(2) * uv[i] + uv[i + 1]);
        });

    std::swap(u, u_new);  // 次のステップでは更新後の配列を読む
  }

  // 誤差: 離散解 G^n sin(pi x) と解析解 exp(-pi^2 kappa t) sin(pi x)
  const Real s = util::sin(pi<Real> * dx / 2);
  const Real g = Real(1) - Real(4) * r * s * s;
  const Real amp_discrete = std::pow(g, n_steps);
  const Real amp_exact = util::exp(-pi<Real> * pi<Real> * kappa * t_end);

  auto uv = u.const_view();
  auto max_op = MISO_LAMBDA(Real a, Real b) { return a > b ? a : b; };
  const Real err_discrete = reduce(
      backend::Host{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_discrete * util::sin(pi<Real> * x));
      },
      max_op);
  const Real err_exact = reduce(
      backend::Host{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_exact * util::sin(pi<Real> * x));
      },
      max_op);

  std::printf("N = %d, steps = %d, r = %.4f\n", n, n_steps, r);
  std::printf("max error (discrete) = %.3e\n", err_discrete);
  std::printf("max error (exact)    = %.6e\n", err_exact);

  const std::string fname = "result_N" + std::to_string(n) + "_serial.txt";
  std::ofstream ofs(fname);
  ofs << std::scientific << std::setprecision(17);
  for (int i = margin; i < margin + n; ++i) {
    ofs << (i - margin + Real(0.5)) * dx << " " << u[i] << "\n";
  }
  return 0;
}
