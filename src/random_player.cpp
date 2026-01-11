#include "random_player.h"

namespace tigerdragon {

RandomPlayer::RandomPlayer(uint32_t seed) : rng_(seed) {}

bool RandomPlayer::ChooseAction(const std::vector<Action>& actions, Action* out_action) {
  if (actions.empty() || out_action == nullptr) {
    return false;
  }
  std::uniform_int_distribution<size_t> dist(0, actions.size() - 1);
  *out_action = actions[dist(rng_)];
  return true;
}

}  // namespace tigerdragon
