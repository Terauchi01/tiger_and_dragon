#include "engine.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using tigerdragon::Action;
using tigerdragon::GameConfig;
using tigerdragon::GameState;
using tigerdragon::Tile;

namespace {

void PrintHand(const std::vector<Tile>& hand) {
  for (size_t i = 0; i < hand.size(); ++i) {
    std::cout << "  [" << i << "] " << tigerdragon::ToString(hand[i].kind) << "\n";
  }
}

std::string PhaseLabel(GameState::Phase phase) {
  switch (phase) {
    case GameState::Phase::Attack:
      return "Attack";
    case GameState::Phase::Defend:
      return "Defend";
    case GameState::Phase::BonusReceive:
      return "BonusReceive";
    case GameState::Phase::Finished:
      return "Finished";
  }
  return "Unknown";
}

void RenderState(const GameState& state) {
  std::cout << "\n=== Debug GUI ===\n";
  std::cout << "Phase: " << PhaseLabel(state.phase) << "\n";
  std::cout << "Current player: " << state.current_player << "\n";
  if (state.attack_tile.has_value()) {
    std::cout << "Attack tile: " << tigerdragon::ToString(state.attack_tile->kind) << "\n";
  } else {
    std::cout << "Attack tile: (none)\n";
  }
  for (int player = 0; player < state.players; ++player) {
    std::cout << "Player " << player << " hand size: " << state.hands[player].size() << "\n";
  }
  std::cout << "==============\n";
}

int ReadChoice(int max_index) {
  int choice = -1;
  while (true) {
    std::cout << "Select action index (0-" << max_index << "), or -1 to quit: ";
    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }
    if (choice == -1 || (choice >= 0 && choice <= max_index)) {
      return choice;
    }
  }
}

void PrintAction(const GameState& state, const Action& action, int index) {
  std::cout << "  (" << index << ") ";
  switch (action.type) {
    case Action::Type::Attack:
      std::cout << "Attack with hand[" << action.hand_index << "]";
      break;
    case Action::Type::Defend:
      std::cout << "Defend with hand[" << action.hand_index << "]";
      break;
    case Action::Type::Pass:
      std::cout << "Pass";
      break;
    case Action::Type::BonusReceive:
      std::cout << "Bonus receive with hand[" << action.hand_index << "]";
      break;
  }
  if (action.hand_index >= 0) {
    const auto& hand = state.hands[action.player];
    if (action.hand_index < static_cast<int>(hand.size())) {
      std::cout << " -> " << tigerdragon::ToString(hand[action.hand_index].kind);
    }
  }
  std::cout << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  GameConfig config;
  config.players = 4;
  config.seed = 42;
  if (argc > 1) {
    config.players = std::atoi(argv[1]);
  }
  if (argc > 2) {
    config.seed = static_cast<uint32_t>(std::atoi(argv[2]));
  }

  GameState state = tigerdragon::CreateInitialState(config);

  while (!state.finished) {
    RenderState(state);
    std::cout << "Current hand for player " << state.current_player << ":\n";
    PrintHand(state.hands[state.current_player]);

    auto actions = tigerdragon::GenerateLegalActions(state);
    if (actions.empty()) {
      std::cout << "No legal actions.\n";
      break;
    }

    std::cout << "Available actions:\n";
    for (size_t i = 0; i < actions.size(); ++i) {
      PrintAction(state, actions[i], static_cast<int>(i));
    }

    int choice = ReadChoice(static_cast<int>(actions.size() - 1));
    if (choice == -1) {
      std::cout << "Quit.\n";
      break;
    }

    if (!tigerdragon::ApplyAction(state, actions[choice])) {
      std::cout << "Invalid action.\n";
      continue;
    }

    if (state.hands[actions[choice].player].empty()) {
      state.finished = true;
      state.winner = actions[choice].player;
      state.phase = GameState::Phase::Finished;
    }
  }

  if (state.winner >= 0) {
    std::cout << "Winner: Player " << state.winner << "\n";
  }

  return 0;
}
