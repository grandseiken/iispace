#ifndef II_GAME_NETWORK_TOOLS_H
#define II_GAME_NETWORK_TOOLS_H
#include "game/common/result.h"
#include "game/data/packet.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/logic/sim/networked_sim_state.h"
#include <concurrentqueue.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace ii {

// For each player index, the remote ID.
using topology_t = std::vector<std::string>;

inline result<topology_t>
parse_topology(const std::string& topology_string, std::uint32_t player_count) {
  if (topology_string.size() != player_count) {
    return unexpected("topology " + topology_string + " is invalid for replay with player count " +
                      std::to_string(player_count));
  }
  topology_t topology;
  for (std::uint32_t i = 0; i < player_count; ++i) {
    topology.emplace_back("peer:" + topology_string.substr(i, 1));
  }
  return topology;
}

inline NetworkedSimState::input_mapping
mapping_for(const topology_t& topology, const std::string& id) {
  NetworkedSimState::input_mapping m;
  for (std::uint32_t i = 0; i < topology.size(); ++i) {
    if (topology[i] == id) {
      m.local.player_numbers.emplace_back(i);
    } else {
      m.remote[topology[i]].player_numbers.emplace_back(i);
    }
  }
  return m;
}

struct network_options_t {
  std::string topology;
  std::uint64_t max_tick_difference = 0;
  std::uint64_t max_tick_delivery_delay = 0;
};

inline result<void> parse_network_args(std::vector<std::string>& args, network_options_t& options) {
  if (auto r = flag_parse(args, "topology", options.topology); !r) {
    return unexpected(r.error());
  }
  if (auto r =
          flag_parse<std::uint64_t>(args, "max_tick_difference", options.max_tick_difference, 32ul);
      !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse<std::uint64_t>(args, "max_tick_delivery_delay",
                                         options.max_tick_delivery_delay, 8ul);
      !r) {
    return unexpected(r.error());
  }
  return {};
}

struct peer_t {
  peer_t(const initial_conditions& conditions, const topology_t& topology,
         network_options_t options, const std::string& id,
         std::vector<std::unique_ptr<peer_t>>& peers, std::span<std::uint32_t> ai_players = {})
  : id{id}
  , replay_writer{conditions}
  , mapping{mapping_for(topology, id)}
  , options{std::move(options)}
  , sim{conditions, mapping, &replay_writer, ai_players}
  , peers{peers}
  , engine{std::hash<std::string>{}(id)} {
    for (const auto& pair : mapping.remote) {
      inbox[pair.first];
    }
    std::cout << "initialising " << id << " with local players ";
    bool first = true;
    for (auto n : mapping.local.player_numbers) {
      if (!first) {
        std::cout << "; ";
      }
      first = false;
      std::cout << n;
    }
    std::cout << std::endl;
  }

  struct received_packet {
    std::uint64_t delivery_tick_count = 0;
    sim_packet packet;
  };

  std::string id;
  ReplayWriter replay_writer;
  NetworkedSimState::input_mapping mapping;
  network_options_t options;
  NetworkedSimState sim;
  std::vector<std::unique_ptr<peer_t>>& peers;
  std::mt19937_64 engine;

  struct remote_inbox {
    moodycamel::ConcurrentQueue<received_packet> delivery_queue;
    std::vector<received_packet> receive_buffer;
  };
  std::thread thread;
  std::unordered_map<std::string, remote_inbox> inbox;

  std::condition_variable tick_count_cv;
  std::mutex tick_count_mutex;
  std::atomic<std::uint64_t> tick_count{0};

  void deliver(std::string source_id, received_packet packet) {
    inbox.find(source_id)->second.delivery_queue.enqueue(std::move(packet));
  }

  void wait_for_sync(std::uint64_t target_tick_count) {
    // TODO: limit drift with new timing functions in NetworkedSimState instead?
    auto minimum_tick =
        target_tick_count - std::min(target_tick_count, options.max_tick_difference);
    if (tick_count >= minimum_tick) {
      return;
    }
    std::unique_lock lock{tick_count_mutex};
    tick_count_cv.wait(lock, [&] { return tick_count >= minimum_tick; });
  }

  void update(std::vector<input_frame>&& local_input) {
    auto current_tick = sim.predicted().tick_count();
    for (auto& peer : peers) {
      peer->wait_for_sync(current_tick);
    }
    for (auto& pair : inbox) {
      auto& e = pair.second;
      received_packet packet;
      while (e.delivery_queue.try_dequeue(packet)) {
        e.receive_buffer.emplace_back(packet);
      }
      auto end = std::find_if(e.receive_buffer.begin(), e.receive_buffer.end(),
                              [&](const auto& p) { return p.delivery_tick_count > current_tick; });
      for (auto it = e.receive_buffer.begin(); it != end; ++it) {
        sim.input_packet(pair.first, it->packet);
      }
      e.receive_buffer.erase(e.receive_buffer.begin(), end);
    }
    auto packets = sim.update(std::move(local_input));
    sim.clear_output();

    for (auto& p : packets) {
      received_packet packet;
      packet.packet = std::move(p);
      packet.delivery_tick_count = current_tick +
          std::uniform_int_distribution<std::uint64_t>{0, options.max_tick_delivery_delay}(engine);
      for (auto& peer : peers) {
        if (peer->id != id) {
          peer->deliver(id, packet);
        }
      }
    }

    {
      std::lock_guard lock{tick_count_mutex};
      ++tick_count;
    }
    tick_count_cv.notify_all();
  }
};

}  // namespace ii

#endif