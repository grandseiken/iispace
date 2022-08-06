#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/replay_tools.h"
#include <concurrentqueue.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ii {
namespace {

struct options_t {
  std::string topology;
  std::uint64_t max_tick_difference = 0;
  std::uint64_t max_tick_delivery_delay = 0;
};

// For each player index, the remote ID.
using topology_t = std::vector<std::string>;

result<topology_t> parse_topology(const std::string& topology_string, std::uint32_t player_count) {
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

NetworkedSimState::input_mapping mapping_for(const topology_t& topology, const std::string& id) {
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

bool run(const options_t& options, const std::string& replay_path) {
  io::StdFilesystem fs{".", ".", "."};
  auto replay_bytes = fs.read(replay_path);
  if (!replay_bytes) {
    std::cerr << replay_bytes.error() << std::endl;
    return false;
  }

  auto replay_reader = ReplayReader::create(*replay_bytes);
  if (!replay_reader) {
    std::cerr << replay_reader.error() << std::endl;
    return false;
  }
  auto player_count = replay_reader->initial_conditions().player_count;
  auto topology_result = parse_topology(options.topology, player_count);
  if (!topology_result) {
    std::cerr << topology_result.error() << std::endl;
    return false;
  }
  auto topology = std::move(*topology_result);

  auto canonical_results = replay_results(*replay_bytes);
  if (!canonical_results) {
    std::cerr << canonical_results.error() << std::endl;
    return false;
  }
  print_replay_info(std::cout, replay_path, *canonical_results);

  using per_player_inputs_t = std::vector<std::vector<input_frame>>;
  per_player_inputs_t per_player_inputs;
  per_player_inputs.resize(player_count);
  ReplayWriter canonical_bytes_writer{replay_reader->initial_conditions()};
  std::size_t canonical_frames_written = 0;
  while (replay_reader->current_input_frame() < replay_reader->total_input_frames()) {
    auto inputs = replay_reader->next_tick_input_frames();
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      per_player_inputs[i].emplace_back(inputs[i]);
      if (canonical_frames_written < canonical_results->replay_frames_read) {
        canonical_bytes_writer.add_input_frame(inputs[i]);
        ++canonical_frames_written;
      }
    }
  }
  auto canonical_bytes = canonical_bytes_writer.write();
  if (!canonical_bytes) {
    std::cerr << canonical_bytes.error() << std::endl;
    return false;
  }
  std::cout << "================================================" << std::endl;

  struct received_packet {
    std::uint64_t delivery_tick_count = 0;
    sim_packet packet;
  };

  struct peer_t {
    peer_t(const initial_conditions& conditions, const topology_t& topology, options_t options,
           const std::string& id, std::vector<std::unique_ptr<peer_t>>& peers)
    : id{id}
    , replay_writer{conditions}
    , mapping{mapping_for(topology, id)}
    , options{std::move(options)}
    , sim{conditions, mapping, &replay_writer}
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

    std::string id;
    ReplayWriter replay_writer;
    NetworkedSimState::input_mapping mapping;
    options_t options;
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

    void update(const per_player_inputs_t& replay) {
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
        auto end =
            std::find_if(e.receive_buffer.begin(), e.receive_buffer.end(),
                         [&](const auto& p) { return p.delivery_tick_count > current_tick; });
        for (auto it = e.receive_buffer.begin(); it != end; ++it) {
          sim.input_packet(pair.first, it->packet);
        }
        e.receive_buffer.erase(e.receive_buffer.begin(), end);
      }
      std::vector<input_frame> local_input;
      for (auto n : mapping.local.player_numbers) {
        local_input.emplace_back(current_tick < replay[n].size() ? replay[n][current_tick]
                                                                 : input_frame{});
      }
      received_packet packet;
      packet.packet = sim.update(std::move(local_input));
      packet.delivery_tick_count = current_tick +
          std::uniform_int_distribution<std::uint64_t>{0, options.max_tick_delivery_delay}(engine);
      for (auto& peer : peers) {
        if (peer->id != id) {
          peer->deliver(id, packet);
        }
      }

      {
        std::lock_guard lock{tick_count_mutex};
        ++tick_count;
      }
      tick_count_cv.notify_all();
    }
  };

  std::unordered_set<std::string> peer_ids{topology.begin(), topology.end()};
  std::vector<std::unique_ptr<peer_t>> peers;
  peers.reserve(peer_ids.size());
  for (const auto& id : peer_ids) {
    peers.emplace_back(std::make_unique<peer_t>(replay_reader->initial_conditions(), topology,
                                                options, id, peers));
  }

  for (auto& peer : peers) {
    peer->thread = std::thread{[&] {
      while (!peer->sim.canonical().game_over()) {
        peer->update(per_player_inputs);
      }
    }};
  }
  for (auto& peer : peers) {
    peer->thread.join();
  }
  std::cout << "================================================\n"
            << "simulation complete\n"
            << "================================================" << std::endl;

  bool success = true;
  for (const auto& peer : peers) {
    for (const auto& id : peer->sim.checksum_failed_remote_ids()) {
      std::cerr << "error: " << peer->id << " reported checksum failure for " << id << std::endl;
      success = false;
    }
    auto results = peer->sim.canonical().get_results();
    if (results.score != canonical_results->sim.score) {
      std::cout << "verification failed for " << peer->id << ": score was " << results.score
                << std::endl;
      success = false;
    }
    if (results.tick_count < canonical_results->sim.tick_count) {
      std::cout << "verification failed for " << peer->id << ": ticks was " << results.tick_count
                << std::endl;
      success = false;
    }
    auto peer_replay_bytes = peer->replay_writer.write();
    if (!peer_replay_bytes) {
      std::cout << "replay serialization failed for " << peer->id << ": "
                << peer_replay_bytes.error() << std::endl;
      success = false;
    }
    if (*peer_replay_bytes != *canonical_bytes) {
      std::cout << "verification failed for " << peer->id << ": replay output bytes did not match"
                << std::endl;
      success = false;
    }
  }
  return success;
}

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;
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
  return {std::move(options)};
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  auto options = ii::parse_args(args);
  if (!options) {
    std::cerr << options.error() << std::endl;
    return 1;
  }
  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (args.empty()) {
    std::cerr << "no paths" << std::endl;
    return 1;
  }
  int exit = 0;
  for (const auto& path : args) {
    if (!ii::run(*options, path)) {
      exit = 1;
    }
  }
  return exit;
}
