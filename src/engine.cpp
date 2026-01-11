#include "engine.h"

#include <algorithm>
#include <array>
#include <sstream>

namespace tigerdragon {

namespace {

bool IsEven(TileKind kind) {
  return kind == TileKind::Num2 || kind == TileKind::Num4 || kind == TileKind::Num6 ||
         kind == TileKind::Num8;
}

bool IsOdd(TileKind kind) {
  return kind == TileKind::Num1 || kind == TileKind::Num3 || kind == TileKind::Num5 ||
         kind == TileKind::Num7;
}

bool IsNumber(TileKind kind) {
  return kind <= TileKind::Num8;
}

int NumberValue(TileKind kind) {
  return static_cast<int>(kind) + 1;
}

bool CanDefendWith(TileKind attack, TileKind defend) {
  if (IsNumber(attack) && IsNumber(defend)) {
    return NumberValue(attack) == NumberValue(defend);
  }
  if (defend == TileKind::Tiger) {
    return IsEven(attack);
  }
  if (defend == TileKind::Dragon) {
    return IsOdd(attack);
  }
  return false;
}

int HandSizeForPlayers(int players) {
  switch (players) {
    case 2:
      return 12;
    case 3:
      return 11;
    case 4:
      return 9;
    case 5:
      return 7;
    default:
      return 0;
  }
}

}  // namespace

std::vector<Tile> BuildDeck() {
  std::vector<Tile> deck;
  deck.reserve(38);
  for (int value = 1; value <= 8; ++value) {
    for (int count = 0; count < value; ++count) {
      deck.push_back(Tile{static_cast<TileKind>(value - 1)});
    }
  }
  deck.push_back(Tile{TileKind::Tiger});
  deck.push_back(Tile{TileKind::Dragon});
  return deck;
}

GameState CreateInitialState(const GameConfig& config) {
  GameState state;
  state.players = config.players;
  state.current_player = 0;
  state.attack_player = -1;
  state.hands.assign(config.players, {});
  state.bonus_discards.assign(config.players, 0);

  std::vector<Tile> deck = BuildDeck();
  std::mt19937 rng(config.seed);
  std::shuffle(deck.begin(), deck.end(), rng);

  const int hand_size = HandSizeForPlayers(config.players);
  for (int player = 0; player < config.players; ++player) {
    state.hands[player].insert(state.hands[player].end(),
                               deck.begin() + player * hand_size,
                               deck.begin() + (player + 1) * hand_size);
  }

  const int start_player = 0;
  state.current_player = start_player;
  state.attack_player = start_player;

  const int start_draw_index = hand_size * config.players;
  if (start_draw_index < static_cast<int>(deck.size())) {
    state.hands[start_player].push_back(deck[start_draw_index]);
  }

  state.phase = GameState::Phase::Attack;
  return state;
}

std::vector<Action> GenerateLegalActions(const GameState& state) {
  std::vector<Action> actions;
  if (state.finished) {
    return actions;
  }

  const int player = state.current_player;
  const auto& hand = state.hands[player];

  if (state.phase == GameState::Phase::Attack) {
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
      actions.push_back(Action{Action::Type::Attack, player, i});
    }
    return actions;
  }

  if (state.phase == GameState::Phase::BonusReceive) {
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
      actions.push_back(Action{Action::Type::BonusReceive, player, i});
    }
    return actions;
  }

  if (state.phase == GameState::Phase::Defend) {
    if (!state.attack_tile.has_value()) {
      return actions;
    }

    bool has_defend = false;
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
      if (CanDefendWith(state.attack_tile->kind, hand[i].kind)) {
        actions.push_back(Action{Action::Type::Defend, player, i});
        has_defend = true;
      }
    }

    actions.push_back(Action{Action::Type::Pass, player, -1});

    if (!has_defend) {
      return actions;
    }
    return actions;
  }

  return actions;
}

bool ApplyAction(GameState& state, const Action& action) {
  if (state.finished || action.player != state.current_player) {
    return false;
  }

  auto& hand = state.hands[action.player];

  auto remove_from_hand = [&](int index) -> Tile {
    Tile tile = hand[index];
    hand.erase(hand.begin() + index);
    return tile;
  };

  if (state.phase == GameState::Phase::Attack) {
    if (action.type != Action::Type::Attack || action.hand_index < 0 ||
        action.hand_index >= static_cast<int>(hand.size())) {
      return false;
    }

    state.attack_tile = remove_from_hand(action.hand_index);
    state.attack_player = action.player;
    state.phase = GameState::Phase::Defend;
    state.current_player = (action.player + 1) % state.players;
    return true;
  }

  if (state.phase == GameState::Phase::BonusReceive) {
    if (action.type != Action::Type::BonusReceive || action.hand_index < 0 ||
        action.hand_index >= static_cast<int>(hand.size())) {
      return false;
    }

    remove_from_hand(action.hand_index);
    state.bonus_discards[action.player] += 1;
    state.phase = GameState::Phase::Attack;
    return true;
  }

  if (state.phase == GameState::Phase::Defend) {
    if (!state.attack_tile.has_value()) {
      return false;
    }

    if (action.type == Action::Type::Pass) {
      state.current_player = (state.current_player + 1) % state.players;
      if (state.current_player == state.attack_player) {
        state.phase = GameState::Phase::BonusReceive;
      }
      return true;
    }

    if (action.type != Action::Type::Defend || action.hand_index < 0 ||
        action.hand_index >= static_cast<int>(hand.size())) {
      return false;
    }

    if (!CanDefendWith(state.attack_tile->kind, hand[action.hand_index].kind)) {
      return false;
    }

    remove_from_hand(action.hand_index);
    state.attack_tile.reset();
    state.attack_player = action.player;
    state.phase = GameState::Phase::Attack;
    state.current_player = action.player;
    return true;
  }

  return false;
}

std::string ToString(TileKind kind) {
  switch (kind) {
    case TileKind::Num1:
      return "1";
    case TileKind::Num2:
      return "2";
    case TileKind::Num3:
      return "3";
    case TileKind::Num4:
      return "4";
    case TileKind::Num5:
      return "5";
    case TileKind::Num6:
      return "6";
    case TileKind::Num7:
      return "7";
    case TileKind::Num8:
      return "8";
    case TileKind::Tiger:
      return "Tiger";
    case TileKind::Dragon:
      return "Dragon";
  }
  return "?";
}

}  // namespace tigerdragon
