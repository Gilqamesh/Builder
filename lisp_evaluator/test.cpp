#include <iostream>
using namespace std;

struct a_t {
  a_t(): d{
    &a_t::g,
    &a_t::h
  } {
  }

  using dispatcher_fn = void (a_t::* const [8])();
  dispatcher_fn d;

  void f(int a) {
    const auto c = d[a];
    if (c) {
      (this->*c)();
    }
  }
  void g() {
    cout << "g" << endl;
  }
  void h() {
    cout << "h" << endl;
  }
};

int main() {
  a_t a;
  a.f(0);
  a.f(1);
  a.f(2);
}

