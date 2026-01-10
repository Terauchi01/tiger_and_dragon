#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace tigerdragon {

enum class TileKind : uint8_t {
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Tiger,
  Dragon,
};

struct Tile {
  TileKind kind;
};

struct Action {
  enum class Type : uint8_t {
    Attack,
    Defend,
    Pass,
    BonusReceive,
  } type;

  int player = 0;
  int hand_index = -1;
};

struct GameConfig {
  int players = 2;
  uint32_t seed = 0;
};

struct GameState {
  enum class Phase : uint8_t {
    Attack,
    Defend,
    BonusReceive,
    Finished,
  } phase = Phase::Attack;

  int players = 0;
  int current_player = 0;
  int attack_player = -1;
  std::optional<Tile> attack_tile;

  std::vector<std::vector<Tile>> hands;
  std::vector<int> bonus_discards;

  bool finished = false;
  int winner = -1;
};

std::vector<Tile> BuildDeck();

GameState CreateInitialState(const GameConfig& config);

std::vector<Action> GenerateLegalActions(const GameState& state);

bool ApplyAction(GameState& state, const Action& action);

std::string ToString(TileKind kind);

}  // namespace tigerdragon
