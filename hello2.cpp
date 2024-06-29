//
#include <iostream>

class Number {

  int64_t integer_;
  int64_t float_;

  Number() = default;

public:
  static constexpr int MAX_DIGITS_AFTER_DOT = 12;

  Number(const char* str) {
    const char* temp = str;
    while (temp[0] != '\0' && temp[0] != '.') {
      temp++;
    }
    if (temp[0] == '\0') {
      integer_ = parse_int(str, temp);
      float_ = 0;
    } else {
      integer_ = parse_int(str, temp);
      const char* end = temp;
      while (end[0] != '\0') {
	end++;
      }
      float_ = parse_int(temp+1, end);
      int digits = end-temp;
      while (digits <= MAX_DIGITS_AFTER_DOT) {
	float_ *= 10;
	digits++;
      }
    }
  }

  Number operator+(const Number& other) const {
    // constexpr int64_t div = 1000'000'000'000;
    constexpr int64_t div = 1e(MAX_DIGITS_AFTER_DOT);

    Number result;
    result.float_ = (this->float_ + other.float_) % div;
    result.integer_ = this->integer_ + other.integer_ + (this->float_ + other.float_) / div;

    return result;
  }

public:
  static int64_t parse_int(const char* begin, const char* end) {
    if (begin == end)
      return 0;
    int64_t result = 0;
    int64_t pow = 1;

    const char* it = end-1;
    while (it >= begin) {
      result += (it[0]-'0')*pow;
      pow *= 10;
      it--;
    }
    return result;
  }

  void print() {
    std::cout << integer_ << '.' << float_ << '\n';
  }

};

int main()
{
  const char* s = "1234395.";
  std::cout << Number::parse_int(s, s+7) << '\n';

  Number number(s);
  number.print();

  Number n1("0.46");
  Number n2("0.999");
  n1.print();
  n2.print();
  (n1+n2).print();

  return 0;
}
