
#ifndef PARLAY_QUICKSORT_H_
#define PARLAY_QUICKSORT_H_

#include <algorithm>
#include <utility>

#include "sequence_ops.h"
#include "../utilities.h"

namespace parlay {
namespace internal {

template <class Iterator>
bool base_case(Iterator x, size_t n) {
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  bool large = std::is_pointer<value_type>::value || (sizeof(x) > 8);
  return large ? (n < 16) : (n < 24);
}

template <class Iterator, class BinPred>
void insertion_sort(Iterator A, size_t n, const BinPred& f) {
  for (size_t i = 1; i < n; i++) {
    long j = i;
    while (--j >= 0 && f(A[j + 1], A[j])) {
      std::swap(A[j + 1], A[j]);
    }
  }
}

// sorts 5 elements taken at even stride and puts them at the front
template <class Iterator, class BinPred>
void sort5(Iterator A, size_t n, const BinPred& f) {
  size_t size = 5;
  size_t m = n / (size + 1);
  for (size_t l = 0; l < size; l++) std::swap(A[l], A[m * (l + 1)]);
  insertion_sort(A, size, f);
}

// splits based on two pivots (p1 and p2) into 3 parts:
//    less than p1, greater than p2, and the rest in the middle
// If the pivots are the same, returns a true flag to indicate middle need not
// be sorted
template <class Iterator, class BinPred>
std::tuple<Iterator, Iterator, bool> split3(Iterator A, size_t n, const BinPred& f) {
  assert(n >= 5);
  sort5(A, n, f);
  
  // Use A[1] and A[3] as the pivots. Move them to
  // the front so that A[0] and A[1] are the pivots
  std::swap(A[0], A[1]);
  std::swap(A[1], A[3]);
  const auto& p1 = A[0];
  const auto& p2 = A[1];
  bool pivots_equal = !f(p1, p2);
  
  // set up initial invariants
  auto L = A + 2;
  auto R = A + n - 1;
  while (f(*L, p1)) L++;
  while (f(p2, *R)) R--;
  auto M = L;
  
  // invariants:
  //  below L is less than p1,
  //  above R is greater than p2
  //  between L and M are between p1 and p2 inclusive
  //  between M and R are unprocessed
  while (M <= R) {

    if (f(*M, p1)) {
      std::swap(*M, *L);
      L++;
    } else if (f(p2, *M)) {
      std::swap(*M, *R);
      if (f(*M, p1)) {
        std::swap(*L, *M);
        L++;
      }

      R--;
      while (f(p2, *R)) R--;
    }
    M++;
  }

  // Swap the pivots into position
  L -= 2;
  std::swap(A[1], *(L+1));
  std::swap(A[0], *L);
  std::swap(*(L+1), *R);

  return std::make_tuple(L, M, pivots_equal);
}

template <class Iterator, class BinPred>
void quicksort_serial(Iterator A, size_t n, const BinPred& f) {
  while (!base_case(A, n)) {
    Iterator L, M;
    bool mid_eq;
    std::tie(L, M, mid_eq) = split3(A, n, f);
    if (!mid_eq) quicksort_serial(L+1, M - L-1, f);
    quicksort_serial(M, A + n - M, f);
    n = L - A;
  }

  insertion_sort(A, n, f);
}

template <class Iterator, class BinPred>
void quicksort(Iterator A, size_t n, const BinPred& f) {
  if (n < (1 << 10))
    quicksort_serial(A, n, f);
  else {
    Iterator L;
    Iterator M;
    bool mid_eq;
    std::tie(L, M, mid_eq) = split3(A, n, f);
    auto left = [&]() { quicksort(A, L - A, f); };
    auto mid = [&]() { quicksort(L + 1, M - L - 1, f); };
    auto right = [&]() { quicksort(M, A + n - M, f); };

    if (!mid_eq)
      par_do3(left, mid, right);
    else
      par_do(left, right);
  }
}

template <class Iterator, class BinPred>
void quicksort(slice<Iterator, Iterator> A, const BinPred& f) {
  quicksort(A.begin(), A.size(), f);
}

//// Fully Parallel version below here

// ---------------  Not currently tested or used ----------------

template <class SeqA, class BinPred, typename Iterator>
std::tuple<size_t, size_t, bool> p_split3(SeqA const& A,
                                          slice<Iterator, Iterator> B,
                                          const BinPred& f) {
  using E = typename SeqA::value_type;
  size_t n = A.size();
  sort5(A.begin(), n, f);
  E p1 = A[1];
  E p2 = A[3];
  if (!f(A[0], A[1])) p1 = p2;  // if few elements less than p1, then set to p2
  if (!f(A[3], A[4]))
    p2 = p1;  // if few elements greater than p2, then set to p1
  auto flag = [&](size_t i) { return f(A[i], p1) ? 0 : f(p2, A[i]) ? 2 : 1; };
  auto r = split_three(A, B.slice(), delayed_seq<unsigned char>(n, flag),
                       fl_conservative);
  return std::make_tuple(r.first, r.first + r.second, !f(p1, p2));
  // sequence<size_t> r = count_sort(A, B.slice(),
  //				    delayed_seq<unsigned char>(n, flag), 3,
  //true);
  // return std::make_tuple(r[0],r[0]+r[1], !f(p1,p2));
}

// The fully parallel version copies back and forth between two arrays
// inplace: if true then result is put back in In
//     and Out is just used as temp space
//     otherwise result is in Out
//     In and Out cannot be the same (Out is needed as temp space)
// cut_size: is when to revert to  quicksort.
//    If -1 then it uses a default based on number of threads
template <typename InIterator, typename OutIterator, typename F>
void p_quicksort_(slice<InIterator, InIterator> In,
                  slice<OutIterator, OutIterator> Out,
                  const F& f,
                  bool inplace = false,
                  long cut_size = -1) {
  size_t n = In.size();
  if (cut_size == -1)
    cut_size = std::max<long>((3 * n) / num_workers(), (1 << 14));
  if (n < (size_t)cut_size) {
    quicksort(In.begin(), n, f);
    auto copy_out = [&](size_t i) { Out[i] = In[i]; };
    if (!inplace) parallel_for(0, n, copy_out, 2000);
  } else {
    size_t l, m;
    bool mid_eq;
    std::tie(l, m, mid_eq) = p_split3(In, Out, f);
    par_do3(
        [&]() {
          p_quicksort_(Out.slice(0, l), In.slice(0, l), f, !inplace, cut_size);
        },
        [&]() {
          auto copy_in = [&](size_t i) { In[i] = Out[i]; };
          if (!mid_eq)
            p_quicksort_(Out.slice(l, m), In.slice(l, m), f, !inplace,
                         cut_size);
          else if (inplace)
            parallel_for(l, m, copy_in, 2000);
        },
        [&]() {
          p_quicksort_(Out.slice(m, n), In.slice(m, n), f, !inplace, cut_size);
        });
  }
}

template <class SeqA, class F>
sequence<typename SeqA::value_type> p_quicksort(SeqA const& In, const F& f) {
  using T = typename SeqA::value_type;
  sequence<T> Out(In.size());
  p_quicksort_(In.slice(), Out.slice(), f);
  return Out;
}

template <typename Iterator, class F>
void p_quicksort_inplace(slice<Iterator, Iterator> In, const F& f) {
  using value_type = typename slice<Iterator, Iterator>::value_type;
  auto Tmp = sequence<value_type>::uninitialized(In.size());
  p_quicksort_(In, Tmp.slice(), f, true);
}

}  // namespace internal
}  // namespace parlay

#endif  // PARLAY_QUICKSORT_H_
