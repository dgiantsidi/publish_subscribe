#include "lib.h"
#include <chrono>
#include <thread>
#include <vector>

static void func(std::shared_ptr<Context> ptr) {
  Subscriber sub(ptr);
  sub.listen_on_event(kEventType::EVENT0);
}

int main(void) {
  std::shared_ptr<Context> ptr = std::make_unique<Context>();
  Publisher pub(ptr);
  std::vector<std::thread> threads;
  for (auto i = 0ULL; i < 10; i++) {
    threads.emplace_back(std::thread(func, ptr));
  }
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2000ms);
  pub.notify_on_event(kEventType::EVENT0);
  for (auto &th : threads)
    th.join();

  fmt::print("[{}] all subscribers joined\n", __PRETTY_FUNCTION__);
  return 0;
}
