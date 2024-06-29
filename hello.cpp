#include <iostream>
#include <vector>
#include <deque>

// на вход N положительных чисел
// окно K - скользящее
// в каждом окне нужно посчитать минимум

// 3 4 9 0 1
// K=3
// 3 0 0
// N-(K+1)//2

// N: 5, K: 2
// Out: 4

std::vector<int> sliding_min(const std::vector<int>& a, int K)
{
  int N = a.size();

  std::vector<int> b;
  b.reserve(N-K+1);
  std::deque<int> q;

  for (int i = 0; i < N; i++) {
    if (!q.empty() && (q.front() <= i-K)) {
      q.pop_front();
    }
    while (!q.empty() && a[q.back()] > a[i]) {
      q.pop_back();
    }
    q.push_back(i);

    if (i >= K-1) {
      b.push_back(a[q.front()]);
    }

    /*    int min = a[i];
    for (int j = 1; j < K; j++) {
      if (a[i+j] < min) {
	min = a[i+j];
      }
    }
    b.push_back(min);*/
  }
  return b;
}

int main()
{
  std::cout << "Hello world!\n";

  std::vector<int> a = {3, 4, 9, 0, 1};
  int K = 3;
  std::vector<int> b = sliding_min(a, K);

  for (int i = 0; i < b.size(); i++) {
    std::cout << b[i] << ' ';
  }
  std::cout << '\n';

  return 0;
}
