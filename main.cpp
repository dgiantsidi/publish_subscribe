#include "callbacks.h"
#include "lib.h"
#include <chrono>
#include <thread>
#include <vector>

static inline void rand_timer_func() {
  using namespace std::chrono_literals;
  std::chrono::duration<int, std::ratio<1, 1000>> timer(rand() % 100);
  std::this_thread::sleep_for(timer);
}

static void func(std::shared_ptr<Context> ptr) {
  Subscriber sub(ptr);
  auto num = rand() % 3;

  sub.subscribe(static_cast<kEventType>(num));
  rand_timer_func();
  auto [event, data, data_sz] = sub.listen_on_event(kEventType::EVENT0);
  fmt::print("[{}] data_sz={}\n", __PRETTY_FUNCTION__, data_sz);
  if (data_sz != (size_t)(-1))
    ptr->invoke_callback(static_cast<kEventType>(num), data, data_sz);
}

int main(void) {
  std::shared_ptr<Context> ptr = std::make_unique<Context>();
  ptr->reg(kEventType::EVENT0, callback0);
  ptr->reg(kEventType::EVENT1, callback1);
  ptr->reg(kEventType::EVENT2, callback2);

  Publisher pub(ptr);
  std::vector<std::thread> threads;
  for (auto i = 0ULL; i < kThreadsNum; i++) {
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