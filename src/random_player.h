#pragma once

#include <random>
#include <vector>

#include "engine.h"

namespace tigerdragon {

class RandomPlayer {
 public:
  explicit RandomPlayer(uint32_t seed);

  bool ChooseAction(const std::vector<Action>& actions, Action* out_action);

 private:
  std::mt19937 rng_;
};

}  // namespace tigerdragon
