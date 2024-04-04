#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include "logger.h"
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdlib.h>
#include <tuple>
#include <unordered_map>
#include <vector>

enum kEventType { EVENT0 = 0, EVENT1 = 1, EVENT2 = 2 };

static std::atomic<int> counter{0};
constexpr size_t kMemPoolSz = 1024 + 1;
constexpr size_t kDataSz = 512 + 1;

static int gen_unique_id() { return counter.fetch_add(1); }

class Callbacks {
public:
  using callback_type =
      std::function<void(std::shared_ptr<uint8_t[]> data, size_t data_sz)>;

  std::map<kEventType, callback_type> callbacks_map;

  void reg(kEventType event, callback_type cb) {
    callbacks_map.insert({event, cb});
  }

  void invoke_callback(const kEventType &event, std::shared_ptr<uint8_t[]> data,
                       size_t data_sz) {
    auto callback_it = callbacks_map.find(event);
    callback_it->second(data, data_sz);
  }
};

class Context {
public:
  Context() {
    subscribers_per_event.insert({kEventType::EVENT0, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT1, std::vector<int>()});
    subscribers_per_event.insert({kEventType::EVENT2, std::vector<int>()});

    locks_per_event.insert(
        {kEventType::EVENT0, std::make_unique<middleware>()});
    locks_per_event.insert(
        {kEventType::EVENT1, std::make_unique<middleware>()});
    locks_per_event.insert(
        {kEventType::EVENT2, std::make_unique<middleware>()});

    shared_mem = std::make_unique<uint8_t[]>(kMemPoolSz);
    shared_mem[kMemPoolSz - 1] = '\0';
  }

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>, size_t>
  wait(const kEventType event, int s_id) {
    {
      std::shared_lock<std::shared_mutex> shared_lck(subscribers_list_m);
      auto &subscribers = subscribers_per_event[event];
      if (std::find(subscribers.begin(), subscribers.end(), s_id) ==
          subscribers.end()) {
        std::string to_be_printed = "id=" + std::to_string(s_id) +
                                    "not subscribed to event=" +
                                    std::to_string(static_cast<int>(event));
        DEBUG_LOG(to_be_printed);
        return {event, std::shared_ptr<uint8_t[]>(new uint8_t[1]), -1};
      }
    }
    auto &m = (locks_per_event[event]->m);
    auto &cv = (locks_per_event[event]->cv);
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
    std::unique_lock<std::shared_mutex> exclusive_lck(subscribers_list_m);
    auto &subscribers = subscribers_per_event[event];
    std::string to_be_printed =
        "subscribe id=" + std::to_string(id) +
        " to event=" + std::to_string(static_cast<int>(event)) + "\n";
    DEBUG_LOG(to_be_printed);
    if (std::find(subscribers.begin(), subscribers.end(), id) ==
        subscribers.end())
      subscribers_per_event[event].push_back(id);
  }

  void unsubscribe(const kEventType &event, int &id) {
    std::unique_lock<std::shared_mutex> exclusive_lck(subscribers_list_m);
    auto &subscribers = subscribers_per_event[event];
    auto it = std::find(subscribers.begin(), subscribers.end(), id);
    if (it != subscribers.end())
      subscribers_per_event[event].erase(it);
  }

  void notify(const kEventType &event) {

    auto &m = (locks_per_event[event]->m);
    auto &cv = (locks_per_event[event]->cv);
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

  void reg(kEventType event, Callbacks::callback_type cb) {
    cbs.reg(event, cb);
  }

  void invoke_callback(const kEventType &event, std::shared_ptr<uint8_t[]> data,
                       size_t data_sz) {
    cbs.invoke_callback(event, data, data_sz);
  }

private:
  struct middleware {
    std::mutex m;
    std::condition_variable cv;
  };
  std::unordered_map<int, std::vector<int>> subscribers_per_event;
  std::unordered_map<int, std::unique_ptr<middleware>> locks_per_event;
  bool ready =
      false; /* shared data are ready to be read from the subscribers */
  std::unique_ptr<uint8_t[]>
      shared_mem; /* common area where parallel threads find data (read-only) */
  std::shared_mutex subscribers_list_m;

  Callbacks cbs;
};

class Publisher {
public:
  explicit Publisher(std::shared_ptr<Context> _ctx) {
    ctx = _ctx;
    p_id = gen_unique_id();
  }

  void notify_on_event(kEventType event) { ctx->notify(event); }

  ~Publisher() {
    std::string to_be_printed = "id=" + std::to_string(p_id) + "\n";
    DEBUG_LOG(to_be_printed);
  }

private:
  std::shared_ptr<Context> ctx;
  int p_id = -1;
};

class Subscriber {
public:
  explicit Subscriber(std::shared_ptr<Context> _ctx) {
    ctx = _ctx;
    s_id = gen_unique_id();
    std::string to_be_printed = "id=" + std::to_string(s_id) + "\n";
    DEBUG_LOG(to_be_printed.c_str());
  }

  ~Subscriber() {
    std::string to_be_printed = "id=" + std::to_string(s_id) + "\n";
    DEBUG_LOG(to_be_printed.c_str());
  }

  std::tuple<kEventType, std::shared_ptr<uint8_t[]>, size_t>
  listen_on_event(kEventType event) {
    return ctx->wait(event, s_id);
  }

  void subscribe(kEventType e) { ctx->subscribe(e, s_id); }

  void unsubscribe(kEventType e) { ctx->unsubscribe(e, s_id); }

private:
  std::shared_ptr<Context> ctx;
  int s_id = -1;
};