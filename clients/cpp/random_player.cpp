#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using Client = websocketpp::client<websocketpp::config::asio_client>;

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

std::vector<std::string> SplitCsv(const std::string& value) {
  std::vector<std::string> items;
  if (value.empty()) {
    return items;
  }
  std::stringstream ss(value);
  std::string item;
  while (std::getline(ss, item, ',')) {
    item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char c) { return !std::isspace(c); }));
    item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), item.end());
    if (!item.empty()) {
      items.push_back(item);
    }
  }
  return items;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "usage: random_player ws://localhost:9002 room1 p1\n";
    return 1;
  }
  std::string uri = argv[1];
  std::string room_id = (argc > 2) ? argv[2] : "room1";
  std::string player_id = (argc > 3) ? argv[3] : "p1";

  Client client;
  client.init_asio();
  client.clear_access_channels(websocketpp::log::alevel::all);
  client.clear_error_channels(websocketpp::log::elevel::all);

  std::mt19937 rng{std::random_device{}()};
  bool done = false;

  client.set_open_handler([&](websocketpp::connection_hdl hdl) {
    std::cout << "C++ client joining room=" << room_id << " player_id=" << player_id << "\n";
    std::ostringstream join;
    join << "{\"type\":\"join\",\"room_id\":\"" << room_id << "\",";
    join << "\"player_id\":\"" << player_id << "\",\"role\":\"player\"}";
    client.send(hdl, join.str(), websocketpp::frame::opcode::text);
  });

  client.set_message_handler([&](websocketpp::connection_hdl hdl, Client::message_ptr msg) {
    std::string payload = msg->get_payload();
    auto type = ExtractString(payload, "type");
    if (!type.has_value()) {
      return;
    }
    if (done) {
      return;
    }
    if (type.value() == "state") {
      auto legal = ExtractString(payload, "legal");
      if (legal.has_value()) {
        auto choices = SplitCsv(legal.value());
        if (!choices.empty()) {
          std::uniform_int_distribution<size_t> dist(0, choices.size() - 1);
          std::string choice = choices[dist(rng)];
          std::ostringstream action;
          action << "{\"type\":\"action\",\"room_id\":\"" << room_id << "\",";
          action << "\"player_id\":\"" << player_id << "\",\"choice\":\"" << choice << "\"}";
          client.send(hdl, action.str(), websocketpp::frame::opcode::text);
        }
      }
    } else if (type.value() == "game_over") {
      std::cout << "game_over\n";
      done = true;
      websocketpp::lib::error_code close_ec;
      client.close(hdl, websocketpp::close::status::normal, "done", close_ec);
      client.stop();
    } else if (type.value() == "error") {
      auto message = ExtractString(payload, "message");
      if (message.has_value()) {
        std::cout << "error: " << message.value() << "\n";
      }
    }
  });

  websocketpp::lib::error_code ec;
  Client::connection_ptr con = client.get_connection(uri, ec);
  if (ec) {
    std::cerr << "connect failed: " << ec.message() << "\n";
    return 1;
  }
  client.connect(con);
  client.run();
  return 0;
}
