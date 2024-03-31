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

static void callback0(std::shared_ptr<uint8_t[]> data, size_t data_sz) {
  // TODO: implement me
  fmt::print("[{}]", __PRETTY_FUNCTION__);
}

static void callback1(std::shared_ptr<uint8_t[]> data, size_t data_sz) {
  // TODO: implement me
  fmt::print("[{}]", __PRETTY_FUNCTION__);
}

static void callback2(std::shared_ptr<uint8_t[]> data, size_t data_sz) {
  // TODO: implement me
  fmt::print("[{}]", __PRETTY_FUNCTION__);
}