#include "engine.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using tigerdragon::Action;
using tigerdragon::GameConfig;
using tigerdragon::GameState;
using tigerdragon::Tile;
using tigerdragon::TileKind;

namespace {

using Server = websocketpp::server<websocketpp::config::asio>;
using ConnectionHdl = websocketpp::connection_hdl;

struct ClientInfo {
  std::string player_id;
  int seat = -1;
  bool spectator = false;
};

struct ScoreEntry {
  int base = 0;
  bool add_bonus = false;
  bool set = false;
};

struct DiscardRecord {
  TileKind kind;
  Action::Type type;
};

constexpr size_t kTileKinds = static_cast<size_t>(TileKind::Dragon) + 1;

std::string Trim(const std::string& input) {
  size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
    ++start;
  }
  size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    --end;
  }
  return input.substr(start, end - start);
}

std::vector<std::string> SplitTokens(const std::string& input, char delim) {
  std::vector<std::string> tokens;
  std::string item;
  std::stringstream ss(input);
  while (std::getline(ss, item, delim)) {
    std::string trimmed = Trim(item);
    if (!trimmed.empty()) {
      tokens.push_back(trimmed);
    }
  }
  return tokens;
}

std::optional<std::string> ExtractString(const std::string& json, const std::string& key) {
  std::string pattern = "\"" + key + "\"";
  size_t pos = json.find(pattern);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  pos = json.find(':', pos + pattern.size());
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
    ++pos;
  }
  if (pos >= json.size() || json[pos] != '"') {
    return std::nullopt;
  }
  ++pos;
  size_t end = json.find('"', pos);
  if (end == std::string::npos) {
    return std::nullopt;
  }
  return json.substr(pos, end - pos);
}

std::optional<int> ExtractInt(const std::string& json, const std::string& key) {
  std::string pattern = "\"" + key + "\"";
  size_t pos = json.find(pattern);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  pos = json.find(':', pos + pattern.size());
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
    ++pos;
  }
  size_t end = pos;
  while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) {
    ++end;
  }
  if (end == pos) {
    return std::nullopt;
  }
  return std::stoi(json.substr(pos, end - pos));
}

std::string ToLabel(TileKind kind) {
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
      return "T";
    case TileKind::Dragon:
      return "D";
  }
  return "?";
}

std::optional<TileKind> ParseLabel(const std::string& token) {
  if (token == "1") return TileKind::Num1;
  if (token == "2") return TileKind::Num2;
  if (token == "3") return TileKind::Num3;
  if (token == "4") return TileKind::Num4;
  if (token == "5") return TileKind::Num5;
  if (token == "6") return TileKind::Num6;
  if (token == "7") return TileKind::Num7;
  if (token == "8") return TileKind::Num8;
  if (token == "T" || token == "t") return TileKind::Tiger;
  if (token == "D" || token == "d") return TileKind::Dragon;
  return std::nullopt;
}

std::string JoinInts(const std::vector<int>& values) {
  std::ostringstream out;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

std::string JoinLabels(const std::vector<Tile>& tiles) {
  std::ostringstream out;
  for (size_t i = 0; i < tiles.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << ToLabel(tiles[i].kind);
  }
  return out.str();
}

char SuffixForAction(Action::Type type) {
  switch (type) {
    case Action::Type::Attack:
      return 'A';
    case Action::Type::Defend:
      return 'D';
    case Action::Type::BonusReceive:
      return 'B';
    case Action::Type::Pass:
      break;
  }
  return '?';
}

std::string JoinDiscards(const std::vector<DiscardRecord>& records) {
  std::ostringstream out;
  size_t count = 0;
  for (const auto& record : records) {
    if (count > 0) {
      out << ",";
    }
    out << ToLabel(record.kind) << SuffixForAction(record.type);
    ++count;
  }
  return out.str();
}

std::string JoinSet(const std::set<std::string>& values) {
  std::ostringstream out;
  size_t i = 0;
  for (const auto& value : values) {
    if (i > 0) {
      out << ",";
    }
    out << value;
    ++i;
  }
  return out.str();
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

bool ParseScoreRules(const std::string& path, std::array<ScoreEntry, kTileKinds>* table) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }
  std::string line;
  while (std::getline(file, line)) {
    size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = Trim(line);
    if (line.empty()) {
      continue;
    }
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
      return false;
    }
    std::string left = Trim(line.substr(0, colon));
    std::string right = Trim(line.substr(colon + 1));
    bool add_bonus = false;
    std::string base_str = right;
    size_t bonus_pos = right.find("+bonus");
    if (bonus_pos != std::string::npos) {
      add_bonus = true;
      base_str = Trim(right.substr(0, bonus_pos));
    }
    int base = 0;
    try {
      base = std::stoi(base_str);
    } catch (const std::exception&) {
      return false;
    }
    auto tokens = SplitTokens(left, ',');
    if (tokens.empty()) {
      return false;
    }
    for (const auto& token : tokens) {
      auto kind = ParseLabel(token);
      if (!kind.has_value()) {
        return false;
      }
      ScoreEntry& entry = (*table)[static_cast<size_t>(kind.value())];
      entry.base = base;
      entry.add_bonus = add_bonus;
      entry.set = true;
    }
  }
  for (const auto& entry : *table) {
    if (!entry.set) {
      return false;
    }
  }
  return true;
}

class MatchServer {
 public:
  MatchServer(int players, uint32_t seed, uint16_t port, const std::string& score_rules_path)
      : players_(players), seed_(seed), port_(port), score_rules_path_(score_rules_path) {
    if (!ParseScoreRules(score_rules_path_, &score_table_)) {
      throw std::runtime_error("Failed to load score rules: " + score_rules_path_);
    }
    scores_.assign(players_, 0);
    server_.init_asio();
    server_.set_open_handler([this](ConnectionHdl hdl) { OnOpen(hdl); });
    server_.set_close_handler([this](ConnectionHdl hdl) { OnClose(hdl); });
    server_.set_message_handler(
        [this](ConnectionHdl hdl, Server::message_ptr msg) { OnMessage(hdl, msg); });
  }

  void Run() {
    server_.listen(port_);
    server_.start_accept();
    std::cout << "WS server listening on " << port_ << "\n";
    server_.run();
  }

 private:
  void OnOpen(ConnectionHdl hdl) {
    clients_[hdl] = ClientInfo{};
  }

  void OnClose(ConnectionHdl hdl) {
    clients_.erase(hdl);
  }

  void OnMessage(ConnectionHdl hdl, const Server::message_ptr& msg) {
    const std::string payload = msg->get_payload();
    auto type = ExtractString(payload, "type");
    if (!type.has_value()) {
      SendError(hdl, "missing type");
      return;
    }
    if (type.value() == "join") {
      HandleJoin(hdl, payload);
      return;
    }
    if (type.value() == "action") {
      HandleAction(hdl, payload);
      return;
    }
    SendError(hdl, "unknown type");
  }

  void HandleJoin(ConnectionHdl hdl, const std::string& payload) {
    auto room_id = ExtractString(payload, "room_id");
    auto player_id = ExtractString(payload, "player_id");
    auto role = ExtractString(payload, "role");
    if (!room_id.has_value() || !player_id.has_value() || !role.has_value()) {
      SendError(hdl, "missing join fields");
      return;
    }

    ClientInfo& info = clients_[hdl];
    info.player_id = player_id.value();
    info.spectator = (role.value() == "spectator");

    if (!info.spectator) {
      if (static_cast<int>(players_joined_.size()) >= players_) {
        SendError(hdl, "room full");
        return;
      }
      info.seat = static_cast<int>(players_joined_.size());
      players_joined_.push_back(hdl);
    }

    std::ostringstream out;
    out << "{\"type\":\"join_ack\",\"room_id\":\"" << room_id.value() << "\",";
    out << "\"player_id\":\"" << info.player_id << "\",";
    out << "\"seat\":" << info.seat << ",\"players\":" << players_ << "}";
    server_.send(hdl, out.str(), websocketpp::frame::opcode::text);

    if (!game_started_ && static_cast<int>(players_joined_.size()) == players_) {
      StartGame();
      BroadcastState();
    } else if (game_started_) {
      SendState(hdl, info);
    }
  }

  void HandleAction(ConnectionHdl hdl, const std::string& payload) {
    if (!game_started_) {
      SendError(hdl, "game not started");
      return;
    }
    if (match_over_) {
      SendError(hdl, "match over");
      return;
    }
    const auto it = clients_.find(hdl);
    if (it == clients_.end()) {
      return;
    }
    const ClientInfo& info = it->second;
    if (info.spectator || info.seat < 0) {
      SendError(hdl, "spectator cannot act");
      return;
    }
    if (info.seat != state_.current_player) {
      SendError(hdl, "not your turn");
      return;
    }
    auto choice = ExtractString(payload, "choice");
    if (!choice.has_value()) {
      SendError(hdl, "missing choice");
      return;
    }

    std::string token = Trim(choice.value());
    std::string lower;
    lower.reserve(token.size());
    for (char c : token) {
      lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    auto actions = tigerdragon::GenerateLegalActions(state_);
    std::optional<Action> selected;
    if (lower == "pass") {
      for (const auto& action : actions) {
        if (action.type == Action::Type::Pass) {
          selected = action;
          break;
        }
      }
    } else {
      auto kind = ParseLabel(token);
      if (!kind.has_value()) {
        SendError(hdl, "invalid choice");
        return;
      }
      Action::Type desired = Action::Type::Attack;
      if (state_.phase == GameState::Phase::Defend) {
        desired = Action::Type::Defend;
      } else if (state_.phase == GameState::Phase::BonusReceive) {
        desired = Action::Type::BonusReceive;
      }
      for (const auto& action : actions) {
        if (action.type != desired || action.hand_index < 0 ||
            action.hand_index >= static_cast<int>(state_.hands[action.player].size())) {
          continue;
        }
        if (state_.hands[action.player][action.hand_index].kind == kind.value()) {
          selected = action;
          break;
        }
      }
    }

    if (!selected.has_value()) {
      SendError(hdl, "illegal action");
      return;
    }

    last_action_tile_.reset();
    if (selected->hand_index >= 0 &&
        selected->hand_index < static_cast<int>(state_.hands[selected->player].size())) {
      TileKind kind = state_.hands[selected->player][selected->hand_index].kind;
      last_action_tile_ = kind;
      if (selected->type != Action::Type::Pass &&
          selected->player >= 0 &&
          selected->player < static_cast<int>(discards_.size())) {
        discards_[selected->player].push_back(DiscardRecord{kind, selected->type});
      }
    }

    if (!tigerdragon::ApplyAction(state_, selected.value())) {
      SendError(hdl, "apply failed");
      return;
    }

    ++turn_id_;
    if (state_.hands[selected->player].empty()) {
      FinishRound(selected->player);
      return;
    }
    BroadcastState();
  }

  void StartGame() {
    GameConfig config;
    config.players = players_;
    config.seed = seed_;
    state_ = tigerdragon::CreateInitialState(config);
    game_started_ = true;
    turn_id_ = 0;
    last_action_tile_.reset();
    discards_.assign(players_, {});
  }

  void BroadcastState() {
    for (const auto& entry : clients_) {
      SendState(entry.first, entry.second);
    }
  }

  void BroadcastGameOver(int winner) {
    std::ostringstream out;
    out << "{\"type\":\"game_over\",\"winner\":" << winner << ",";
    out << "\"scores\":\"" << JoinInts(scores_) << "\"}";
    for (const auto& entry : clients_) {
      server_.send(entry.first, out.str(), websocketpp::frame::opcode::text);
    }
  }

  void SendState(ConnectionHdl hdl, const ClientInfo& info) {
    std::string hand;
    if (!info.spectator && info.seat >= 0) {
      hand = JoinLabels(state_.hands[info.seat]);
    }

    std::string legal;
    if (!info.spectator && info.seat == state_.current_player && !state_.finished) {
      std::set<std::string> choices;
      auto actions = tigerdragon::GenerateLegalActions(state_);
      for (const auto& action : actions) {
        if (action.type == Action::Type::Pass) {
          choices.insert("pass");
          continue;
        }
        if (action.hand_index >= 0 && action.hand_index < static_cast<int>(state_.hands[action.player].size())) {
          choices.insert(ToLabel(state_.hands[action.player][action.hand_index].kind));
        }
      }
      legal = JoinSet(choices);
    }

    std::string attack_tile;
    if (state_.attack_tile.has_value()) {
      attack_tile = ToLabel(state_.attack_tile->kind);
    }

    std::ostringstream out;
    out << "{\"type\":\"state\",\"room_id\":\"" << room_id_ << "\",";
    out << "\"turn\":" << turn_id_ << ",";
    out << "\"phase\":\"" << PhaseLabel(state_.phase) << "\",";
    out << "\"current_player\":" << state_.current_player << ",";
    out << "\"attack_tile\":\"" << attack_tile << "\",";
    out << "\"hand\":\"" << hand << "\",";
    out << "\"hand_sizes\":\"" << JoinInts(HandSizes()) << "\",";
    out << "\"bonus_discards\":\"" << JoinInts(state_.bonus_discards) << "\",";
    out << "\"legal\":\"" << legal << "\",";
    out << "\"scores\":\"" << JoinInts(scores_) << "\"}";

    server_.send(hdl, out.str(), websocketpp::frame::opcode::text);
  }

  std::vector<int> HandSizes() const {
    std::vector<int> sizes;
    sizes.reserve(state_.hands.size());
    for (const auto& hand : state_.hands) {
      sizes.push_back(static_cast<int>(hand.size()));
    }
    return sizes;
  }

  void SendError(ConnectionHdl hdl, const std::string& message) {
    std::ostringstream out;
    out << "{\"type\":\"error\",\"message\":\"" << message << "\"}";
    server_.send(hdl, out.str(), websocketpp::frame::opcode::text);
  }

  int ScoreForTile(TileKind kind, int bonus_discards) const {
    const ScoreEntry& entry = score_table_[static_cast<size_t>(kind)];
    if (!entry.set) {
      return 0;
    }
    int total = entry.base;
    if (entry.add_bonus) {
      total += bonus_discards;
    }
    return total;
  }

  void FinishRound(int winner) {
    state_.finished = true;
    state_.winner = winner;
    state_.phase = GameState::Phase::Finished;

    int bonus = 0;
    std::string winner_hand;
    std::string winner_discards;
    int winner_hand_size = 0;
    if (winner >= 0 && winner < static_cast<int>(state_.bonus_discards.size())) {
      bonus = state_.bonus_discards[winner];
      winner_hand = JoinLabels(state_.hands[winner]);
      winner_hand_size = static_cast<int>(state_.hands[winner].size());
      if (winner < static_cast<int>(discards_.size())) {
        winner_discards = JoinDiscards(discards_[winner]);
      }
    }
    int round_points = 0;
    std::string last_tile;
    if (last_action_tile_.has_value()) {
      last_tile = ToLabel(last_action_tile_.value());
      round_points = ScoreForTile(last_action_tile_.value(), bonus);
    }
    scores_[winner] += round_points;
    ++round_index_;

    std::ostringstream out;
    out << "{\"type\":\"round_result\",\"winner\":" << winner << ",";
    out << "\"last_tile\":\"" << last_tile << "\",";
    out << "\"bonus_discards\":" << bonus << ",";
    out << "\"round_points\":" << round_points << ",";
    out << "\"winner_hand\":\"" << winner_hand << "\",";
    out << "\"winner_hand_size\":" << winner_hand_size << ",";
    out << "\"winner_discards\":\"" << winner_discards << "\",";
    out << "\"scores\":\"" << JoinInts(scores_) << "\",";
    out << "\"round\":" << round_index_ << "}";
    for (const auto& entry : clients_) {
      server_.send(entry.first, out.str(), websocketpp::frame::opcode::text);
    }

    if (scores_[winner] >= target_score_) {
      match_over_ = true;
      BroadcastGameOver(winner);
      return;
    }

    StartGame();
    BroadcastState();
  }

  Server server_;
  std::map<ConnectionHdl, ClientInfo, std::owner_less<ConnectionHdl>> clients_;
  std::vector<ConnectionHdl> players_joined_;
  int players_ = 4;
  uint32_t seed_ = 42;
  uint16_t port_ = 9002;
  std::string score_rules_path_;
  std::string room_id_ = "room1";
  bool game_started_ = false;
  bool match_over_ = false;
  int turn_id_ = 0;
  int round_index_ = 0;
  int target_score_ = 10;
  std::array<ScoreEntry, kTileKinds> score_table_{};
  std::vector<int> scores_;
  std::vector<std::vector<DiscardRecord>> discards_;
  std::optional<TileKind> last_action_tile_;
  GameState state_;
};

}  // namespace

int main(int argc, char** argv) {
  int players = 4;
  uint32_t seed = 42;
  uint16_t port = 9002;
  std::string rules_path = "server/score_rules.md";
  if (argc > 1) {
    players = std::atoi(argv[1]);
  }
  if (argc > 2) {
    seed = static_cast<uint32_t>(std::atoi(argv[2]));
  }
  if (argc > 3) {
    port = static_cast<uint16_t>(std::atoi(argv[3]));
  }
  if (argc > 4) {
    rules_path = argv[4];
  }

  try {
    MatchServer server(players, seed, port, rules_path);
    server.Run();
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }
  return 0;
}
