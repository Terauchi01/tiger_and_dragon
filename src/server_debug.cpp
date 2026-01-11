#include "engine.h"
#include "random_player.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

using tigerdragon::Action;
using tigerdragon::GameConfig;
using tigerdragon::GameState;
using tigerdragon::Tile;
using tigerdragon::TileKind;

namespace {

void PrintHandInline(const std::vector<Tile>& hand) {
  std::cout << "Hand: ";
  for (size_t i = 0; i < hand.size(); ++i) {
    if (i > 0) {
      std::cout << " ";
    }
    switch (hand[i].kind) {
      case TileKind::Tiger:
        std::cout << "T";
        break;
      case TileKind::Dragon:
        std::cout << "D";
        break;
      default:
        std::cout << tigerdragon::ToString(hand[i].kind);
        break;
    }
  }
  std::cout << "\n";
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

const char* ColorForActionType(Action::Type type) {
  switch (type) {
    case Action::Type::Attack:
      return "\033[31m";
    case Action::Type::Defend:
      return "\033[34m";
    case Action::Type::Pass:
    case Action::Type::BonusReceive:
      return "\033[32m";
  }
  return "\033[0m";
}

const char* LabelForActionType(Action::Type type) {
  switch (type) {
    case Action::Type::Attack:
      return "Attack";
    case Action::Type::Defend:
      return "Defend";
    case Action::Type::Pass:
      return "Pass";
    case Action::Type::BonusReceive:
      return "Bonus";
  }
  return "Unknown";
}

const char* ColorForPhase(GameState::Phase phase) {
  switch (phase) {
    case GameState::Phase::Attack:
      return "\033[31m";
    case GameState::Phase::Defend:
      return "\033[34m";
    case GameState::Phase::BonusReceive:
      return "\033[32m";
    case GameState::Phase::Finished:
      return "\033[0m";
  }
  return "\033[0m";
}

void RenderState(const GameState& state,
                 const std::optional<Action>& last_action,
                 const std::optional<TileKind>& last_tile) {
  std::cout << "\n=== Debug GUI ===\n";
  std::cout << "Phase: " << ColorForPhase(state.phase) << PhaseLabel(state.phase) << "\033[0m\n";
  std::cout << "Current player: " << state.current_player << "\n";
  if (state.attack_tile.has_value()) {
    std::cout << "Attack tile: " << tigerdragon::ToString(state.attack_tile->kind) << "\n";
  } else {
    std::cout << "Attack tile: (none)\n";
  }
  std::cout << "Bonus discards: ";
  for (int player = 0; player < state.players; ++player) {
    if (player > 0) {
      std::cout << " ";
    }
    std::cout << player << ":" << state.bonus_discards[player];
  }
  std::cout << "\n";
  if (last_action.has_value()) {
    const Action& action = last_action.value();
    std::cout << "Last action: " << ColorForActionType(action.type) << LabelForActionType(action.type)
              << "\033[0m";
    std::cout << " by P" << action.player;
    if (last_tile.has_value()) {
      std::cout << " -> " << tigerdragon::ToString(last_tile.value());
    }
    std::cout << "\n";
  }
  for (int player = 0; player < state.players; ++player) {
    std::cout << "Player " << player << " hand size: " << state.hands[player].size() << "\n";
  }
  std::cout << "==============\n";
}

std::string ReadToken() {
  std::string token;
  while (true) {
    std::cout << "Input (1-8, T, D, 0/p/pass, q): ";
    if (!(std::cin >> token)) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }
    return token;
  }
}

void PrintAction(const GameState& state, const Action& action, int index) {
  if (index >= 0) {
    std::cout << "  (" << index << ") ";
  } else {
    std::cout << "  ";
  }
  const char* color = ColorForActionType(action.type);
  const char* reset = "\033[0m";
  switch (action.type) {
    case Action::Type::Attack:
      std::cout << color << "Attack" << reset << " with hand[" << action.hand_index << "]";
      break;
    case Action::Type::Defend:
      std::cout << color << "Defend" << reset << " with hand[" << action.hand_index << "]";
      break;
    case Action::Type::Pass:
      std::cout << color << "Pass" << reset;
      break;
    case Action::Type::BonusReceive:
      std::cout << color << "Bonus" << reset << " receive with hand[" << action.hand_index << "]";
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

std::optional<Action> FindActionByTile(const GameState& state,
                                       const std::vector<Action>& actions,
                                       Action::Type type,
                                       TileKind kind) {
  for (const auto& action : actions) {
    if (action.type != type || action.hand_index < 0 ||
        action.hand_index >= static_cast<int>(state.hands[action.player].size())) {
      continue;
    }
    if (state.hands[action.player][action.hand_index].kind == kind) {
      return action;
    }
  }
  return std::nullopt;
}

std::optional<Action> FindPass(const std::vector<Action>& actions) {
  for (const auto& action : actions) {
    if (action.type == Action::Type::Pass) {
      return action;
    }
  }
  return std::nullopt;
}

std::optional<TileKind> ParseTileToken(const std::string& token) {
  if (token.size() == 1 && std::isdigit(static_cast<unsigned char>(token[0]))) {
    switch (token[0]) {
      case '1':
        return TileKind::Num1;
      case '2':
        return TileKind::Num2;
      case '3':
        return TileKind::Num3;
      case '4':
        return TileKind::Num4;
      case '5':
        return TileKind::Num5;
      case '6':
        return TileKind::Num6;
      case '7':
        return TileKind::Num7;
      case '8':
        return TileKind::Num8;
      default:
        return std::nullopt;
    }
  }
  if (token.size() == 1) {
    char c = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
    if (c == 'T') {
      return TileKind::Tiger;
    }
    if (c == 'D') {
      return TileKind::Dragon;
    }
  }
  return std::nullopt;
}

}  // namespace

int main(int argc, char** argv) {
  GameConfig config;
  config.players = 4;
  config.seed = 42;
  int human_player = 0;
  if (argc > 1) {
    config.players = std::atoi(argv[1]);
  }
  if (argc > 2) {
    config.seed = static_cast<uint32_t>(std::atoi(argv[2]));
  }
  if (argc > 3) {
    human_player = std::atoi(argv[3]);
  }

  GameState state = tigerdragon::CreateInitialState(config);
  tigerdragon::RandomPlayer random_player(config.seed + 100);
  std::optional<Action> last_action;
  std::optional<TileKind> last_tile;

  while (!state.finished) {
    RenderState(state, last_action, last_tile);
    auto actions = tigerdragon::GenerateLegalActions(state);
    if (actions.empty()) {
      std::cout << "No legal actions.\n";
      break;
    }
    Action action;
    if (state.current_player == human_player) {
      PrintHandInline(state.hands[state.current_player]);
      while (true) {
        std::string token = ReadToken();
        std::string lower;
        lower.reserve(token.size());
        for (char c : token) {
          lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        if (lower == "q") {
          std::cout << "Quit.\n";
          return 0;
        }
        if (lower == "0" || lower == "p" || lower == "pass") {
          auto pass_action = FindPass(actions);
          if (pass_action.has_value()) {
            action = pass_action.value();
            break;
          }
          std::cout << "Pass is not available.\n";
          continue;
        }
        auto tile_kind = ParseTileToken(token);
        if (!tile_kind.has_value()) {
          std::cout << "Invalid input.\n";
          continue;
        }
        Action::Type desired_type = Action::Type::Attack;
        if (state.phase == GameState::Phase::Defend) {
          desired_type = Action::Type::Defend;
        } else if (state.phase == GameState::Phase::BonusReceive) {
          desired_type = Action::Type::BonusReceive;
        }
        auto chosen = FindActionByTile(state, actions, desired_type, tile_kind.value());
        if (chosen.has_value()) {
          action = chosen.value();
          break;
        }
        std::cout << "That tile is not playable now.\n";
      }
    } else {
      if (!random_player.ChooseAction(actions, &action)) {
        std::cout << "Random player has no legal actions.\n";
        break;
      }
      std::cout << "Random player " << state.current_player << " selects:\n";
      PrintAction(state, action, -1);
    }

    last_tile.reset();
    if (action.hand_index >= 0 && action.hand_index < static_cast<int>(state.hands[action.player].size())) {
      last_tile = state.hands[action.player][action.hand_index].kind;
    }

    if (!tigerdragon::ApplyAction(state, action)) {
      std::cout << "Invalid action.\n";
      continue;
    }
    last_action = action;

    if (state.hands[action.player].empty()) {
      state.finished = true;
      state.winner = action.player;
      state.phase = GameState::Phase::Finished;
    }
  }

  if (state.winner >= 0) {
    std::cout << "Winner: Player " << state.winner << "\n";
  }

  return 0;
}
