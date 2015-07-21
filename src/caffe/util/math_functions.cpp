#include <boost/math/special_functions/next.hpp>
#include <boost/random.hpp>

#include <limits>
#include <vector>

#include "caffe/common.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/rng.hpp"

namespace caffe {

template<>
void caffe_cpu_gemm<float,float>(const CBLAS_TRANSPOSE TransA,
    const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
    const float alpha, const float* A, const float* B, const float beta,
    float* C) {
  int lda = (TransA == CblasNoTrans) ? K : M;
  int ldb = (TransB == CblasNoTrans) ? N : K;
  cblas_sgemm(CblasRowMajor, TransA, TransB, M, N, K, alpha, A, lda, B,
      ldb, beta, C, N);
}

template<>
void caffe_cpu_gemm<double,double>(const CBLAS_TRANSPOSE TransA,
    const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
    const double alpha, const double* A, const double* B, const double beta,
    double* C) {
  int lda = (TransA == CblasNoTrans) ? K : M;
  int ldb = (TransB == CblasNoTrans) ? N : K;
  cblas_dgemm(CblasRowMajor, TransA, TransB, M, N, K, alpha, A, lda, B,
      ldb, beta, C, N);
}

#ifndef CPU_ONLY
template<>
void caffe_cpu_gemm<half,float>(const CBLAS_TRANSPOSE TransA,
    const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
    const float alpha, const half* A, const half* B, const float beta,
    half* C) {
  if (M <= 0 || N <= 0 || K <= 0) {
    return;
  }
  std::vector<float> a(M*K), b(K*N), c(M*N);
  caffe_cpu_convert(a.size(), A, &a.front());
  caffe_cpu_convert(b.size(), B, &b.front());
  caffe_cpu_convert(c.size(), C, &c.front());
  const int lda = (TransA == CblasNoTrans) ? K : M;
  const int ldb = (TransB == CblasNoTrans) ? N : K;
  cblas_sgemm(CblasRowMajor, TransA, TransB, M, N, K, alpha, &a.front(), lda, &b.front(),
      ldb, beta, &c.front(), N);
  caffe_cpu_convert(c.size(), &c.front(), C);
}
#endif

template <>
void caffe_cpu_gemv<float,float>(const CBLAS_TRANSPOSE TransA, const int M,
    const int N, const float alpha, const float* A, const float* x,
    const float beta, float* y) {
  cblas_sgemv(CblasRowMajor, TransA, M, N, alpha, A, N, x, 1, beta, y, 1);
}

template <>
void caffe_cpu_gemv<double,double>(const CBLAS_TRANSPOSE TransA, const int M,
    const int N, const double alpha, const double* A, const double* x,
    const double beta, double* y) {
  cblas_dgemv(CblasRowMajor, TransA, M, N, alpha, A, N, x, 1, beta, y, 1);
}

#ifndef CPU_ONLY
template <>
void caffe_cpu_gemv<half,float>(const CBLAS_TRANSPOSE TransA, const int M,
    const int N, const float alpha, const half* A, const half* x,
    const float beta, half* y) {
  if (M <= 0 || N <= 0) {
    return;
  }
  const int lx = (TransA == CblasNoTrans) ? N : M;
  const int ly = (TransA == CblasNoTrans) ? M : N;
  std::vector<float> a(M*N), xv(lx), yv(ly);
  caffe_cpu_convert(a.size(), A, &a.front());
  caffe_cpu_convert(xv.size(), x, &xv.front());
  caffe_cpu_convert(yv.size(), y, &yv.front());
  cblas_sgemv(CblasRowMajor, TransA, M, N, alpha, &a.front(), N, &xv.front(), 1, beta, &yv.front(), 1);
  caffe_cpu_convert(yv.size(), &yv.front(), y);
}
#endif

template <>
void caffe_axpy<float,float>(const int N, const float alpha, const float* X,
    float* Y) { cblas_saxpy(N, alpha, X, 1, Y, 1); }

template <>
void caffe_axpy<double,double>(const int N, const double alpha, const double* X,
    double* Y) { cblas_daxpy(N, alpha, X, 1, Y, 1); }

#ifndef CPU_ONLY
template<>
void caffe_axpy<half,float>(const int N, const float alpha, const half* X,
    half* Y) {
  for (int i=0; i<N; i++) {
    Y[i] = Get<half>( alpha * Get<float>(X[i]) + Get<float>(Y[i]) );
  }
}
#endif

template <typename Dtype, typename Mtype>
void caffe_set(const int N, const Mtype alpha, Dtype* Y) {
  if (alpha == 0) {
    memset(Y, 0, sizeof(Dtype) * N);  // NOLINT(caffe/alt_fn)
    return;
  }
  for (int i = 0; i < N; ++i) {
    Y[i] = Get<Dtype>(alpha);
  }
}

template void caffe_set<int,int>(const int N, const int alpha, int* Y);
template void caffe_set<float,float>(const int N, const float alpha, float* Y);
template void caffe_set<double,double>(const int N, const double alpha, double* Y);

#ifndef CPU_ONLY
template void caffe_set<half,float>(const int N, const float alpha, half* Y);
#endif

template <>
void caffe_add_scalar(const int N, const float alpha, float* Y) {
  for (int i = 0; i < N; ++i) {
    Y[i] += alpha;
  }
}

template <>
void caffe_add_scalar(const int N, const double alpha, double* Y) {
  for (int i = 0; i < N; ++i) {
    Y[i] += alpha;
  }
}

#ifndef CPU_ONLY
template <>
void caffe_add_scalar(const int N, const float alpha, half* Y) {
  for (int i = 0; i < N; ++i) {
    Y[i] = Get<half>( Get<float>(Y[i]) + alpha );
  }
}
#endif

template <typename Dtype, typename Mtype>
void caffe_copy(const int N, const Dtype* X, Dtype* Y) {
  if (X != Y) {
    if (Caffe::mode() == Caffe::GPU) {
#ifndef CPU_ONLY
      // NOLINT_NEXT_LINE(caffe/alt_fn)
      CUDA_CHECK(cudaMemcpy(Y, X, sizeof(Dtype) * N, cudaMemcpyDefault));
#else
      NO_GPU;
#endif
    } else {
      memcpy(Y, X, sizeof(Dtype) * N);  // NOLINT(caffe/alt_fn)
    }
  }
}

template void caffe_copy<int,int>(const int N, const int* X, int* Y);
template void caffe_copy<unsigned int, unsigned int>(const int N, const unsigned int* X,
    unsigned int* Y);
template void caffe_copy<float,float>(const int N, const float* X, float* Y);
template void caffe_copy<double,double>(const int N, const double* X, double* Y);

#ifndef CPU_ONLY
template void caffe_copy<half,float>(const int N, const half* X, half* Y);
#endif

template <>
void caffe_scal<float,float>(const int N, const float alpha, float *X) {
  cblas_sscal(N, alpha, X, 1);
}

template <>
void caffe_scal<double,double>(const int N, const double alpha, double *X) {
  cblas_dscal(N, alpha, X, 1);
}

#ifndef CPU_ONLY
template <>
void caffe_scal<half,float>(const int N, const float alpha, half *X) {
  for (int i=0; i<N; i++) {
    X[i] = Get<half>( alpha * Get<float>(X[i]) );
  }
}
#endif

template <>
void caffe_cpu_axpby<float,float>(const int N, const float alpha, const float* X,
                            const float beta, float* Y) {
  cblas_saxpby(N, alpha, X, 1, beta, Y, 1);
}

template <>
void caffe_cpu_axpby<double,double>(const int N, const double alpha, const double* X,
                             const double beta, double* Y) {
  cblas_daxpby(N, alpha, X, 1, beta, Y, 1);
}

#ifndef CPU_ONLY
template <>
void caffe_cpu_axpby<half,float>(const int N, const float alpha, const half* X,
                             const float beta, half* Y) {
  for (int i=0; i<N; i++) {
    Y[i] = Get<half>( alpha * Get<float>(X[i]) + beta * Get<float>(Y[i]) );
  }
}
#endif

template <>
void caffe_add<float,float>(const int n, const float* a, const float* b,
    float* y) {
  vsAdd(n, a, b, y);
}

template <>
void caffe_add<double,double>(const int n, const double* a, const double* b,
    double* y) {
  vdAdd(n, a, b, y);
}

#ifndef CPU_ONLY
template <>
void caffe_add<half,float>(const int n, const half* a, const half* b,
    half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( Get<float>(a[i]) + Get<float>(b[i]) );
  }
}
#endif

template <>
void caffe_sub<float,float>(const int n, const float* a, const float* b,
    float* y) {
  vsSub(n, a, b, y);
}

template <>
void caffe_sub<double,double>(const int n, const double* a, const double* b,
    double* y) {
  vdSub(n, a, b, y);
}

#ifndef CPU_ONLY
template <>
void caffe_sub<half,float>(const int n, const half* a, const half* b,
    half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( Get<float>(a[i]) - Get<float>(b[i]) );
  }
}
#endif

template <>
void caffe_mul<float,float>(const int n, const float* a, const float* b,
    float* y) {
  vsMul(n, a, b, y);
}

template <>
void caffe_mul<double,double>(const int n, const double* a, const double* b,
    double* y) {
  vdMul(n, a, b, y);
}

#ifndef CPU_ONLY
template <>
void caffe_mul<half,float>(const int n, const half* a, const half* b,
    half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( Get<float>(a[i]) * Get<float>(b[i]) );
  }
}
#endif

template <>
void caffe_div<float,float>(const int n, const float* a, const float* b,
    float* y) {
  vsDiv(n, a, b, y);
}

template <>
void caffe_div<double,double>(const int n, const double* a, const double* b,
    double* y) {
  vdDiv(n, a, b, y);
}

#ifndef CPU_ONLY
template <>
void caffe_div<half,float>(const int n, const half* a, const half* b,
    half* y)
{
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( Get<float>(a[i]) / Get<float>(b[i]) );
  }
}
#endif

template <>
void caffe_powx<float,float>(const int n, const float* a, const float b,
    float* y) {
  vsPowx(n, a, b, y);
}

template <>
void caffe_powx<double,double>(const int n, const double* a, const double b,
    double* y) {
  vdPowx(n, a, b, y);
}

#ifndef CPU_ONLY
template <>
void caffe_powx<half,float>(const int n, const half* a, const float b, half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( pow(Get<float>(a[i]), b) );
  }
}
#endif

template <>
void caffe_sqr<float,float>(const int n, const float* a, float* y) {
  vsSqr(n, a, y);
}

template <>
void caffe_sqr<double,double>(const int n, const double* a, double* y) {
  vdSqr(n, a, y);
}

#ifndef CPU_ONLY
template <>
void caffe_sqr<half,float>(const int n, const half* a, half* y) {
  float f;
  for (int i = 0; i < n; ++i) {
    f = Get<float>(a[i]);
    y[i] = Get<half>(f * f);
  }
}
#endif

template <>
void caffe_exp<float,float>(const int n, const float* a, float* y) {
  vsExp(n, a, y);
}

template <>
void caffe_exp<double,double>(const int n, const double* a, double* y) {
  vdExp(n, a, y);
}

#ifndef CPU_ONLY
template <>
void caffe_exp<half,float>(const int n, const half* a, half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( exp(Get<float>(a[i])) );
  }
}
#endif

template <>
void caffe_abs<float,float>(const int n, const float* a, float* y) {
    vsAbs(n, a, y);
}

template <>
void caffe_abs<double,double>(const int n, const double* a, double* y) {
    vdAbs(n, a, y);
}

#ifndef CPU_ONLY
template <>
void caffe_abs<half,float>(const int n, const half* a, half* y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( fabs(Get<float>(a[i])) );
  }
}
#endif

unsigned int caffe_rng_rand() {
  return (*caffe_rng())();
}

template <typename Dtype, typename Mtype>
Dtype caffe_nextafter(const Mtype b) {
  return boost::math::nextafter<Dtype>(
      b, std::numeric_limits<Dtype>::max());
}

template
float caffe_nextafter(const float b);

template
double caffe_nextafter(const double b);

template <typename Dtype, typename Mtype>
void caffe_rng_uniform(const int n, const Mtype a, const Mtype b, Dtype* r) {
  CHECK_GE(n, 0);
  CHECK(r);
  CHECK_LE(a, b);
  boost::uniform_real<Mtype> random_distribution(a, caffe_nextafter<Mtype>(b));
  boost::variate_generator<caffe::rng_t*, boost::uniform_real<Mtype> >
      variate_generator(caffe_rng(), random_distribution);
  for (int i = 0; i < n; ++i) {
    r[i] = Get<Dtype>(variate_generator());
  }
}

template
void caffe_rng_uniform<float,float>(const int n, const float a, const float b,
                              float* r);

template
void caffe_rng_uniform<double,double>(const int n, const double a, const double b,
                               double* r);

#ifndef CPU_ONLY
template
void caffe_rng_uniform<half,float>(const int n, const float a, const float b,
                               half* r);
#endif

template <typename Dtype, typename Mtype>
void caffe_rng_gaussian(const int n, const Mtype a,
                        const Mtype sigma, Dtype* r) {
  CHECK_GE(n, 0);
  CHECK(r);
  CHECK_GT(sigma, 0);
  boost::normal_distribution<Mtype> random_distribution(a, sigma);
  boost::variate_generator<caffe::rng_t*, boost::normal_distribution<Mtype> >
      variate_generator(caffe_rng(), random_distribution);
  for (int i = 0; i < n; ++i) {
    r[i] = Get<Dtype>(variate_generator());
  }
}

template
void caffe_rng_gaussian<float,float>(const int n, const float mu,
                               const float sigma, float* r);

template
void caffe_rng_gaussian<double,double>(const int n, const double mu,
                                const double sigma, double* r);

#ifndef CPU_ONLY
template
void caffe_rng_gaussian<half,float>(const int n, const float mu,
                                const float sigma, half* r);
#endif

template <typename Dtype, typename Mtype>
void caffe_rng_bernoulli(const int n, const Mtype p, int* r) {
  CHECK_GE(n, 0);
  CHECK(r);
  CHECK_GE(p, 0);
  CHECK_LE(p, 1);
  boost::bernoulli_distribution<Mtype> random_distribution(p);
  boost::variate_generator<caffe::rng_t*, boost::bernoulli_distribution<Mtype> >
      variate_generator(caffe_rng(), random_distribution);
  for (int i = 0; i < n; ++i) {
    r[i] = Get<int>(variate_generator());
  }
}

template
void caffe_rng_bernoulli<double,double>(const int n, const double p, int* r);

template
void caffe_rng_bernoulli<float,float>(const int n, const float p, int* r);

#ifndef CPU_ONLY
template
void caffe_rng_bernoulli<half,float>(const int n, const float p, int* r);
#endif

template <typename Dtype, typename Mtype>
void caffe_rng_bernoulli(const int n, const Mtype p, unsigned int* r) {
  CHECK_GE(n, 0);
  CHECK(r);
  CHECK_GE(p, 0);
  CHECK_LE(p, 1);
  boost::bernoulli_distribution<Mtype> random_distribution(p);
  boost::variate_generator<caffe::rng_t*, boost::bernoulli_distribution<Mtype> >
      variate_generator(caffe_rng(), random_distribution);
  for (int i = 0; i < n; ++i) {
    r[i] = static_cast<unsigned int>(variate_generator());
  }
}

template
void caffe_rng_bernoulli<double,double>(const int n, const double p, unsigned int* r);

template
void caffe_rng_bernoulli<float,float>(const int n, const float p, unsigned int* r);

#ifndef CPU_ONLY
template
void caffe_rng_bernoulli<half,float>(const int n, const float p, unsigned int* r);
#endif

template <>
float caffe_cpu_strided_dot<float,float>(const int n, const float* x, const int incx,
    const float* y, const int incy) {
  return cblas_sdot(n, x, incx, y, incy);
}

template <>
double caffe_cpu_strided_dot<double,double>(const int n, const double* x,
    const int incx, const double* y, const int incy) {
  return cblas_ddot(n, x, incx, y, incy);
}

#ifndef CPU_ONLY
template <>
float caffe_cpu_strided_dot<half,float>(const int n, const half* x,
    const int incx, const half *y, const int incy) {
  float sum = 0.0f;

  for (int i=0; i<n; i++) {
    int idx_x = i*incx;
    int idx_y = i*incy;

    sum += Get<float>(x[idx_x]) * Get<float>(y[idx_y]);
  }
  return sum;
}
#endif

template <typename Dtype, typename Mtype>
Mtype caffe_cpu_dot(const int n, const Dtype* x, const Dtype* y) {
  return caffe_cpu_strided_dot<Dtype,Mtype>(n, x, 1, y, 1);
}

template
float caffe_cpu_dot<float,float>(const int n, const float* x, const float* y);

template
double caffe_cpu_dot<double,double>(const int n, const double* x, const double* y);

#ifndef CPU_ONLY
template
float caffe_cpu_dot<half,float>(const int n, const half* x, const half* y);
#endif

template <>
int caffe_cpu_hamming_distance<float,float>(const int n, const float* x,
                                  const float* y) {
  int dist = 0;
  for (int i = 0; i < n; ++i) {
    dist += __builtin_popcount(static_cast<uint32_t>(x[i]) ^
                               static_cast<uint32_t>(y[i]));
  }
  return dist;
}

template <>
int caffe_cpu_hamming_distance<double,double>(const int n, const double* x,
                                   const double* y) {
  int dist = 0;
  for (int i = 0; i < n; ++i) {
    dist += __builtin_popcountl(static_cast<uint64_t>(x[i]) ^
                                static_cast<uint64_t>(y[i]));
  }
  return dist;
}

#ifndef CPU_ONLY
template <>
int caffe_cpu_hamming_distance<half, float>(const int n, const half* x,
                                            const half* y) {
  int dist = 0;
  for (int i = 0; i < n; ++i) {
    dist += __builtin_popcount(static_cast<uint16_t>(Get<float>(x[i])) ^
                               static_cast<uint16_t>(Get<float>(y[i])));
  }
  return dist;
}
#endif

template <>
float caffe_cpu_asum<float,float>(const int n, const float* x) {
  return cblas_sasum(n, x, 1);
}

template <>
double caffe_cpu_asum<double,double>(const int n, const double* x) {
  return cblas_dasum(n, x, 1);
}

#ifndef CPU_ONLY
template <>
float caffe_cpu_asum<half,float>(const int n, const half *x) {
  float sum = 0.0f;
  for (int i=0; i<n; i++) {
    sum += fabs(Get<float>(x[i]));
  }
  return sum;
}
#endif

template <>
void caffe_cpu_scale<float,float>(const int n, const float alpha, const float *x,
                            float* y) {
  cblas_scopy(n, x, 1, y, 1);
  cblas_sscal(n, alpha, y, 1);
}

template <>
void caffe_cpu_scale<double,double>(const int n, const double alpha, const double *x,
                             double* y) {
  cblas_dcopy(n, x, 1, y, 1);
  cblas_dscal(n, alpha, y, 1);
}

#ifndef CPU_ONLY
template <>
void caffe_cpu_scale<half,float>(const int n, const float alpha, const half *x,
    half *y) {
  for (int i=0; i<n; i++) {
    y[i] = Get<half>( alpha * Get<float>(x[i]) );
  }
}
#endif

}  // namespace caffe
