#pragma once
#include <atomic>
#include <condition_variable>
#include <fmt/printf.h>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <tuple>
#include <unordered_map>
#include <vector>

enum kEventType { EVENT0 = 0, EVENT1 = 1, EVENT2 = 2 };

static std::atomic<int> counter{0};

static int gen_unique_id() { return counter.fetch_add(1); }

class Context {
public:
  Context() {
    subscribers_per_event.insert({kEventType::EVENT0, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT1, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT2, std::vector<int>()});
  }

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>>
  wait(const kEventType event) {
    std::unique_lock<std::mutex> lk(m);
    // cv.wait(lk, []{return pred;});
    cv.wait(lk);
    return {kEventType : EVENT0, std::make_shared<uint8_t[]>(1024)};
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
      ready = true;
    }
    cv.notify_all();
  }

private:
  std::unordered_map<int, std::vector<int>> subscribers_per_event;
  bool ready = false;
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

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>>
  listen_on_event(kEventType event) {
    return ctx->wait(event);
  }

  void subscribe(kEventType e) { ctx->subscribe(e, s_id); }

  void unsubscribe(kEventType e) { ctx->unsubscribe(e, s_id); }

private:
  std::shared_ptr<Context> ctx;
  int s_id = -1;
};
