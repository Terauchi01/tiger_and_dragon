#include "engine.h"

#include <iostream>
#include <random>
#include <vector>

using tigerdragon::Action;
using tigerdragon::GameConfig;
using tigerdragon::GameState;

int main() {
  GameConfig config;
  config.players = 4;
  config.seed = 42;

  const int kMatches = 10;
  const int kMaxTurns = 500;

  std::mt19937 rng(1234);

  int total_turns = 0;
  std::vector<int> wins(config.players, 0);
  int no_winner = 0;

  for (int match = 0; match < kMatches; ++match) {
    GameState state = tigerdragon::CreateInitialState(config);
    int turns = 0;

    while (!state.finished && turns < kMaxTurns) {
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

    total_turns += turns;
    std::cout << "Match " << match + 1 << ": Turns=" << turns << " ";
    if (state.winner >= 0) {
      ++wins[state.winner];
      std::cout << "Winner=Player " << state.winner << "\n";
    } else {
      ++no_winner;
      std::cout << "No winner\n";
    }
  }

  std::cout << "Summary: matches=" << kMatches << " avg_turns="
            << (kMatches > 0 ? (total_turns / kMatches) : 0) << "\n";
  for (int player = 0; player < config.players; ++player) {
    std::cout << "Player " << player << " wins: " << wins[player] << "\n";
  }
  if (no_winner > 0) {
    std::cout << "No winner: " << no_winner << "\n";
  }

  return 0;
}
