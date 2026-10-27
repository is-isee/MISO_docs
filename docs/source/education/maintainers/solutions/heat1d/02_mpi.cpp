// 実習 heat1d 02: 1 次元熱伝導の MPI 版（解答例）
// 使い方: mpiexec -np P ./heat1d_mpi [N] [r]   (N は P で割り切れること)
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

int main(int argc, char **argv) {
  Env env(argc, argv);  // MPI_Init / MPI_Finalize
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

  const int n = n_global / n_procs;  // このランクの物理セル数
  const int offset = rank * n;       // 最初の物理セルの全体添字
  const int margin = 1;
  const int n_total = n + 2 * margin;

  // 左右の隣。領域の端では MPI_PROC_NULL（通信しない相手）
  const int left = (rank > 0) ? rank - 1 : MPI_PROC_NULL;
  const int right = (rank < n_procs - 1) ? rank + 1 : MPI_PROC_NULL;

  Array1D<Real> u(n_total);
  Array1D<Real> u_new(n_total);

  {
    auto uv = u.view();
    for_each(
        backend::Host{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
          const Real x = (offset + i - margin + Real(0.5)) * dx;
          uv[i] = util::sin(pi<Real> * x);
        });
  }

  for (int step = 0; step < n_steps; ++step) {
    // ゴーストセルの交換
    // (1) 最初の物理セルを左へ送り、右のゴーストセルへ右から受け取る
    MPI_Sendrecv(&u[margin], 1, MPI_DOUBLE, left, 0, &u[n + margin], 1,
                 MPI_DOUBLE, right, 0, comm, MPI_STATUS_IGNORE);
    // (2) 最後の物理セルを右へ送り、左のゴーストセルへ左から受け取る
    MPI_Sendrecv(&u[n + margin - 1], 1, MPI_DOUBLE, right, 1, &u[0], 1,
                 MPI_DOUBLE, left, 1, comm, MPI_STATUS_IGNORE);

    // 物理境界（領域の端を持つランクだけ）
    auto uv = u.view();
    auto unv = u_new.view();
    if (left == MPI_PROC_NULL)
      for_each(
          backend::Host{}, Range1D{0, 1},
          MISO_LAMBDA(int) { uv[0] = -uv[margin]; });
    if (right == MPI_PROC_NULL)
      for_each(
          backend::Host{}, Range1D{0, 1},
          MISO_LAMBDA(int) { uv[n + margin] = -uv[n + margin - 1]; });

    for_each(
        backend::Host{}, Range1D{margin, margin + n}, MISO_LAMBDA(int i) {
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
      backend::Host{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (offset + i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_discrete * util::sin(pi<Real> * x));
      },
      max_op);
  const Real err_exact_local = reduce(
      backend::Host{}, Range1D{margin, margin + n}, Real(0),
      MISO_LAMBDA(int i) {
        const Real x = (offset + i - margin + Real(0.5)) * dx;
        return util::fabs(uv[i] - amp_exact * util::sin(pi<Real> * x));
      },
      max_op);

  Real err_discrete = 0, err_exact = 0;
  MPI_Allreduce(&err_discrete_local, &err_discrete, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&err_exact_local, &err_exact, 1, MPI_DOUBLE, MPI_MAX, comm);

  // 物理セルをランク 0 に集める（ランク順に並ぶ）
  std::vector<Real> u_all(rank == 0 ? n_global : 0);
  MPI_Gather(&u[margin], n, MPI_DOUBLE, u_all.data(), n, MPI_DOUBLE, 0, comm);

  if (rank == 0) {
    std::printf("N = %d, P = %d, steps = %d, r = %.4f\n", n_global, n_procs,
                n_steps, r);
    std::printf("max error (discrete) = %.3e\n", err_discrete);
    std::printf("max error (exact)    = %.6e\n", err_exact);

    const std::string fname = "result_N" + std::to_string(n_global) + "_P" +
                              std::to_string(n_procs) + ".txt";
    std::ofstream ofs(fname);
    ofs << std::scientific << std::setprecision(17);
    for (int j = 0; j < n_global; ++j) {
      ofs << (j + Real(0.5)) * dx << " " << u_all[j] << "\n";
    }
  }
  return 0;
}
