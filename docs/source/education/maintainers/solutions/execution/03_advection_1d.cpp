// practice/execution/03_advection_1d の解答例
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include <miso/array1d.hpp>
#include <miso/execution.hpp>
#include <miso/utility.hpp>

using namespace miso;

#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif

constexpr double kL = 1.0, kX0 = 0.5, kSigma = 0.05, kA = 1.0, kC = 0.37;

// 周期的なガウシアン。初期値（t = 0）と厳密解の両方に使う
__host__ __device__ inline double gaussian(const double x) {
  double d = util::fmod(x - kX0, kL);  // (-L, L) に入る
  if (d > 0.5 * kL)
    d -= kL;  // [-L/2, L/2] へ折り返す
  if (d < -0.5 * kL)
    d += kL;
  return util::exp(-d * d / (2.0 * kSigma * kSigma));
}

// 左右の ghost cell を反対側の物理セルで埋める
void fill_ghosts(Array1D<double, Backend> &u, const int nx) {
  auto uv = u.view();
  for_each(Backend{}, Range1D{0, 1}, MISO_LAMBDA(int) {
    uv[0] = uv[nx];       // 左 ghost <- 最後の物理セル
    uv[nx + 1] = uv[1];   // 右 ghost <- 最初の物理セル
  });
}

// u を読み、1 ステップ後の値を u_new へ書く
void step(const Array1D<double, Backend> &u, Array1D<double, Backend> &u_new,
          const int nx) {
  auto uv = u.view();  // const な配列からは Array1DView<const double>
  auto nv = u_new.view();
  for_each(Backend{}, Range1D{1, nx + 1}, MISO_LAMBDA(int i) {
    nv[i] = uv[i] - kC * (uv[i] - uv[i - 1]);
  });
}

// 物理セルに初期値を入れる
void initialize(Array1D<double, Backend> &u, const int nx) {
  auto uv = u.view();
  for_each(Backend{}, Range1D{1, nx + 1},
           MISO_LAMBDA(int i) { uv[i] = gaussian((i - 1) * kL / nx); });
}

// 1 ステップだけ進め、ホストの逐次計算と比べる
void check_one_step(const int nx) {
  Array1D<double, Backend> u(nx + 2), u_new(nx + 2);
  initialize(u, nx);
  fill_ghosts(u, nx);
  Array1D<double, backend::Host> h(nx + 2), hn(nx + 2);
  h.copy_from(u);  // 更新前（ghost 込み）
  step(u, u_new, nx);
  hn.copy_from(u_new);
  double max_err = 0.0;
  for (int i = 1; i <= nx; ++i) {
    const double ref = h[i] - kC * (h[i] - h[i - 1]);
    max_err = std::max(max_err, std::fabs(hn[i] - ref));
  }
  std::printf("one step, NX = %d: max abs error = %g (%s)\n", nx, max_err,
              max_err <= 1e-14 ? "ok" : "NG");
}

// nstep 進めて、厳密解との相対 L2 誤差を返す
double relative_l2(const int nx, const int nstep) {
  const double dx = kL / nx;
  const double dt = kC * dx / kA;
  Array1D<double, Backend> u(nx + 2), u_new(nx + 2);
  initialize(u, nx);
  for (int n = 0; n < nstep; ++n) {
    fill_ghosts(u, nx);
    step(u, u_new, nx);
    std::swap(u, u_new);  // 中身のポインタを入れ替えるだけで、要素はコピーしない
  }
  const double shift = kA * nstep * dt;
  auto uv = u.view();  // swap の後で View を取り直す
  const auto plus = MISO_LAMBDA(double a, double b) { return a + b; };
  const double num = reduce(
      Backend{}, Range1D{1, nx + 1}, 0.0,
      MISO_LAMBDA(int i) {
        const double diff = uv[i] - gaussian((i - 1) * kL / nx - shift);
        return diff * diff;
      },
      plus);
  const double den = reduce(
      Backend{}, Range1D{1, nx + 1}, 0.0,
      MISO_LAMBDA(int i) {
        const double exact = gaussian((i - 1) * kL / nx - shift);
        return exact * exact;
      },
      plus);
  return std::sqrt(num / den);
}

int main() {
  check_one_step(256);
  for (int k = 0; k < 3; ++k) {
    const int nx = 256 << k, nstep = 200 << k;  // C と終了時刻を揃える
    std::printf("NX = %4d, steps = %4d: relative L2 error = %.4f\n", nx, nstep,
                relative_l2(nx, nstep));
  }
  return 0;
}
