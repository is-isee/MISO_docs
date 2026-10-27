// 実習 heat1d 03: 1 次元熱伝導の GPU 版（解答例）
// 同じソースを CPU（g++）でも CUDA（nvcc）でもビルドできる。
// 使い方: mpiexec -np P ./heat1d_gpu [N] [r]
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

#include <mpi.h>

#include <miso/array1d.hpp>
#include <miso/backend.hpp>
#include <miso/constants.hpp>
#include <miso/env.hpp>
#include <miso/execution.hpp>
#include <miso/utility.hpp>

using namespace miso;
using Real = double;

#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif

/// ゴーストセルの交換と物理境界。
/// MPI にはホスト側の配列を渡す（CUDA-aware MPI を前提にしない）。
struct Halo {
  int n;            // このランクの物理セル数
  int left, right;  // 左右の隣のランク（なければ MPI_PROC_NULL）
  MPI_Comm comm;
  Array1D<Real, Backend> send, recv;  // [0]: 左側, [1]: 右側
  Array1D<Real> h_send, h_recv;       // ホスト側の送受信バッファ

  Halo(int n, int left, int right, MPI_Comm comm)
      : n(n), left(left), right(right), comm(comm), send(2), recv(2), h_send(2),
        h_recv(2) {}

  void apply(Array1D<Real, Backend> &u) {
    // メンバ変数はラムダの前でローカル変数にコピーする（this を取り込まない）
    const int n_ = n;
    const bool has_left = (left != MPI_PROC_NULL);
    const bool has_right = (right != MPI_PROC_NULL);
    auto uv = u.view();
    auto sv = send.view();
    auto rv = recv.view();

    // (1) 送る値を詰める（計算する側のメモリで）
    for_each(
        Backend{}, Range1D{0, 1}, MISO_LAMBDA(int) {
          sv[0] = uv[1];   // 最初の物理セル -> 左へ
          sv[1] = uv[n_];  // 最後の物理セル -> 右へ
        });
    // (2) ホストへ写して MPI で送受信し、(3) 計算する側へ戻す
    h_send.copy_from(send);
    MPI_Sendrecv(h_send.data(), 1, MPI_DOUBLE, left, 0, h_recv.data() + 1, 1,
                 MPI_DOUBLE, right, 0, comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(h_send.data() + 1, 1, MPI_DOUBLE, right, 1, h_recv.data(), 1,
                 MPI_DOUBLE, left, 1, comm, MPI_STATUS_IGNORE);
    recv.copy_from(h_recv);

    // (4) ゴーストセルへ書き戻す。領域の端では物理境界
    for_each(
        Backend{}, Range1D{0, 1}, MISO_LAMBDA(int) {
          uv[0] = has_left ? rv[0] : -uv[1];
          uv[n_ + 1] = has_right ? rv[1] : -uv[n_];
        });
  }
};

int main(int argc, char **argv) {
  Env env(argc, argv);  // MPI と（CUDA ビルドでは）GPU の割り当て
  const int rank = mpi::rank();
  const int n_procs = mpi::size();
  MPI_Comm comm = mpi::comm();

  const int n_global = (argc > 1) ? std::atoi(argv[1]) : 64;
  const Real r_max = (argc > 2) ? std::atof(argv[2]) : Real(0.4);
  if (n_global % n_procs != 0) {
    if (rank == 0)
      std::fprintf(stderr, "N = %d is not divisible by P = %d\n", n_global,
                   n_procs);
    MPI_Abort(comm, 1);
  }

  const Real kappa = 1.0;
  const Real t_end = 0.1;
  const Real dx = Real(1) / n_global;
  const int n_steps =
      static_cast<int>(std::ceil(t_end / (r_max * dx * dx / kappa)));
  const Real dt = t_end / n_steps;
  const Real r = kappa * dt / (dx * dx);

  const int n = n_global / n_procs;
  const int offset = rank * n;
  const int margin = 1;
  const int n_total = n + 2 * margin;
  const int left = (rank > 0) ? rank - 1 : MPI_PROC_NULL;
  const int right = (rank < n_procs - 1) ? rank + 1 : MPI_PROC_NULL;

  Array1D<Real, Backend> u(n_total);
  Array1D<Real, Backend> u_new(n_total);
  Halo halo(n, left, right, comm);

  {
    auto uv = u.view();
    for_each(
        Backend{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
          const Real x = (offset + i - margin + Real(0.5)) * dx;
          uv[i] = util::sin(pi<Real> * x);
        });
  }

  for (int step = 0; step < n_steps; ++step) {
    halo.apply(u);
    auto uv = u.view();
    auto unv = u_new.view();
    for_each(
        Backend{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
          unv[i] = uv[i] + r * (uv[i - 1] - Real(2) * uv[i] + uv[i + 1]);
        });
    std::swap(u, u_new);
  }

  const Real s = util::sin(pi<Real> * dx / 2);
  const Real g = Real(1) - Real(4) * r * s * s;
  const Real amp_discrete = std::pow(g, n_steps);
  const Real amp_exact = util::exp(-pi<Real> * pi<Real> * kappa * t_end);

  auto uv = u.const_view();
  auto max_op = MISO_LAMBDA(Real a, Real b) { return a > b ? a : b; };
  const Real err_discrete_local = reduce(
      Backend{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (offset + i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_discrete * util::sin(pi<Real> * x));
      },
      max_op);
  const Real err_exact_local = reduce(
      Backend{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (offset + i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_exact * util::sin(pi<Real> * x));
      },
      max_op);

  Real err_discrete = 0, err_exact = 0;
  MPI_Allreduce(&err_discrete_local, &err_discrete, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&err_exact_local, &err_exact, 1, MPI_DOUBLE, MPI_MAX, comm);

  // 計算する側の配列をホストへ写してから集める
  Array1D<Real> u_host(n_total);
  u_host.copy_from(u);
  std::vector<Real> u_all(rank == 0 ? n_global : 0);
  MPI_Gather(&u_host[margin], n, MPI_DOUBLE, u_all.data(), n, MPI_DOUBLE, 0,
             comm);

  if (rank == 0) {
#ifdef __CUDACC__
    const char *tag = "gpu";
#else
    const char *tag = "cpu";
#endif
    std::printf("[%s] N = %d, P = %d, steps = %d, r = %.4f\n", tag, n_global,
                n_procs, n_steps, r);
    std::printf("max error (discrete) = %.3e\n", err_discrete);
    std::printf("max error (exact)    = %.6e\n", err_exact);

    const std::string fname = "result_N" + std::to_string(n_global) + "_P" +
                              std::to_string(n_procs) + "_" + tag + ".txt";
    std::ofstream ofs(fname);
    ofs << std::scientific << std::setprecision(17);
    for (int j = 0; j < n_global; ++j) {
      ofs << (j + Real(0.5)) * dx << " " << u_all[j] << "\n";
    }
  }
  return 0;
}
