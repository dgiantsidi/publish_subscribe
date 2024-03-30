#pragma once
#include <atomic>
#include <condition_variable>
#include <fmt/color.h>
#include <fmt/printf.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <tuple>
#include <unordered_map>
#include <vector>

enum kEventType { EVENT0 = 0, EVENT1 = 1, EVENT2 = 2 };

static std::atomic<int> counter{0};
constexpr size_t kMemPoolSz = 1024 + 1;
constexpr size_t kDataSz = 512 + 1;

static int gen_unique_id() { return counter.fetch_add(1); }

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

class Callbacks {
public:
  std::map<std::string, std::function<void()>> callbacks_map;
};

class Context {
public:
  Context() {
    subscribers_per_event.insert({kEventType::EVENT0, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT1, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT2, std::vector<int>()});
    shared_mem = std::make_unique<uint8_t[]>(kMemPoolSz);
    shared_mem[kMemPoolSz - 1] = '\0';
  }

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>, size_t>
  wait(const kEventType event) {
    std::unique_lock<std::mutex> lk(m);
    // cv.wait(lk, []{return pred;});
    cv.wait(lk, [&] { return ready; });
    std::shared_ptr<uint8_t[]> return_data(
        new uint8_t[kDataSz]); // @dimitra: this
                               // std::make_shared<uint8_t[kDataSz]>() does not
                               // support arrays in contrast to
                               // std::make_unique<>
    // fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "{}\n", kDataSz);
    ::memcpy(return_data.get(), shared_mem.get(), kDataSz);

    return_data[kDataSz - 1] = '\0';
    return {kEventType : EVENT0, return_data, kDataSz};
  }

  void subscribe(const kEventType &event, int &id) {
    auto &subscribers = subscribers_per_event[event];
    if (std::find(subscribers.begin(), subscribers.end(), id) !=
        subscribers.end())
      subscribers_per_event[event].push_back(id);
  }

  void unsubscribe(const kEventType &event, int &id) {
    auto &subscribers = subscribers_per_event[event];
    auto it = std::find(subscribers.begin(), subscribers.end(), id);
    if (it != subscribers.end())
      subscribers_per_event[event].erase(it);
  }

  void notify(const kEventType &event) {
    {
      std::lock_guard<std::mutex> lk(m);
      // publishing data to the common memory area
      ::memset(shared_mem.get(), '1', kDataSz - 1);
      shared_mem[kDataSz] = '\0';
      // std::cout << shared_mem << "\n";
      ready = true;
    }
    cv.notify_all();
  }

private:
  std::unordered_map<int, std::vector<int>> subscribers_per_event;
  bool ready =
      false; /* shared data are ready to be read from the subscribers */
  std::unique_ptr<uint8_t[]>
      shared_mem; /* common area where parallel threads find data (read-only) */
  std::mutex m;
  std::condition_variable cv;
};

class Publisher {
public:
  explicit Publisher(std::shared_ptr<Context> _ctx) {
    ctx = _ctx;
    p_id = gen_unique_id();
  }

  void notify_on_event(kEventType event) { ctx->notify(event); }

  ~Publisher() { fmt::print("[{}] w/ id={}\n", __PRETTY_FUNCTION__, p_id); }

private:
  std::shared_ptr<Context> ctx;
  int p_id = -1;
};

class Subscriber {
public:
  explicit Subscriber(std::shared_ptr<Context> _ctx) {
    ctx = _ctx;
    s_id = gen_unique_id();
    fmt::print("[{}] w/ id={}\n", __PRETTY_FUNCTION__, s_id);
  }

  ~Subscriber() { fmt::print("[{}] w/ id={}\n", __PRETTY_FUNCTION__, s_id); }

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>, size_t>
  listen_on_event(kEventType event) {
    return ctx->wait(event);
  }

  void subscribe(kEventType e) { ctx->subscribe(e, s_id); }

  void unsubscribe(kEventType e) { ctx->unsubscribe(e, s_id); }

private:
  std::shared_ptr<Context> ctx;
  int s_id = -1;
};