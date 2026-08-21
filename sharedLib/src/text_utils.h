#pragma once
#include <string.h>
#include <string>
#include <string_view>

inline size_t CopyFixedSizeString(char *destination, const char *source,
                                  size_t maxLen) {
  size_t len = strlen(source);
  if (len >= maxLen - 1) {
    len = maxLen - 1;
  }

  strncpy(destination, source, len);
  destination[len] = '\0';
  return len;
}

template <size_t N> class FixedSizeString {
private:
  char Value[N] = {0};
  size_t Lenght = 0;

public:
  FixedSizeString() {}

  FixedSizeString(const char *input) {
    Lenght = CopyFixedSizeString(Value, input, N);
  }

  FixedSizeString(std::string_view input) {
    Lenght = CopyFixedSizeString(Value, input.data(), N);
  }

  const char *Data() const { return Value; }

  char *Data() { return Value; }

  size_t Size() { return Lenght; }

  size_t Capacity() { return N; }

  FixedSizeString<N> &operator=(const FixedSizeString<N> &other) {
    Lenght = CopyFixedSizeString(Value, other.Data(), N);
    return *this;
  }

  template <size_t OtherN>
  FixedSizeString<N> &operator+=(const FixedSizeString<OtherN> &other) {
    return *this += std::string_view(other.Data(), other.Size());
  }

  FixedSizeString<N> &operator+=(const char *str) {
    if (str) {
      size_t appendLen = strlen(str);
      if (Lenght + appendLen >= N) {
        appendLen = N - 1 - Lenght;
      }
      if (appendLen > 0) {
        strncpy(Value + Lenght, str, appendLen);
        Lenght += appendLen;
        Value[Lenght] = '\0';
      }
    }
    return *this;
  }

  FixedSizeString<N> &operator+=(std::string_view str) {
    size_t appendLen = str.length();
    if (Lenght + appendLen >= N) {
      appendLen = N - 1 - Lenght;
    }
    if (appendLen > 0) {
      strncpy(Value + Lenght, str.data(), appendLen);
      Lenght += appendLen;
      Value[Lenght] = '\0';
    }
    return *this;
  }

  FixedSizeString<N> &operator+=(char c) {
    if (Lenght < N - 1) {
      Value[Lenght] = c;
      Lenght++;
      Value[Lenght] = '\0';
    }
    return *this;
  }

  operator std::string_view() const { return std::string_view(Value, Lenght); }

  operator const char *() const { return Value; }

  operator char *() const { return Value; }
};