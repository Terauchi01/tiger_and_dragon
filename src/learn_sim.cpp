#include "engine.h"

#include <iostream>
#include <random>

using tigerdragon::Action;
using tigerdragon::GameConfig;
using tigerdragon::GameState;

int main() {
  GameConfig config;
  config.players = 4;
  config.seed = 42;

  GameState state = tigerdragon::CreateInitialState(config);
  std::mt19937 rng(1234);

  int turns = 0;
  while (!state.finished && turns < 500) {
    auto actions = tigerdragon::GenerateLegalActions(state);
    if (actions.empty()) {
      break;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(actions.size()) - 1);
    Action action = actions[dist(rng)];

    if (!tigerdragon::ApplyAction(state, action)) {
      std::cerr << "Invalid action\n";
      return 1;
    }

    if (state.hands[action.player].empty()) {
      state.finished = true;
      state.winner = action.player;
      break;
    }

    ++turns;
  }

  std::cout << "Turns: " << turns << "\n";
  if (state.winner >= 0) {
    std::cout << "Winner: Player " << state.winner << "\n";
  } else {
    std::cout << "No winner\n";
  }

  return 0;
}
