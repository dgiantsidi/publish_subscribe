#include <fmt/printf.h>
#include <iostream>
#include <stdint.h>
#include <memory>

namespace logging {
class logger {
    public:
        logger(size_t lineNumber, char const* filename, char const* functionName)
            : m_lineNumber(lineNumber), m_filename(filename), m_functionName(functionName)
        {}

        template<class ...Args>
        inline void debug(Args&&... args) {
            fmt::print("[{}:{}:{}] {}\n", m_filename, m_functionName, m_lineNumber, std::forward<Args>(args)...);
        }
        
    private:
        size_t m_lineNumber;
        char const* m_filename;
        char const* m_functionName;
//        char const* m_format;
};

};

#ifdef VERBOSE
    #define DEBUG_LOG( ...) logging::logger(__LINE__, __FILE__, __func__).debug(__VA_ARGS__)
#else
    #define DEBUG_LOG( ...)  void();
#endif

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::shared_ptr<T> &data) {
  os << "[";
  size_t idx = 0ULL;
  for (;;) {
    if (data[idx] == '\0')
      break;
    os << data[idx];
    idx++;
  }
  os << "]";
  return os;
}
