#include "game/data/replay.h"
#include "game/common/math.h"
#include "game/data/crypt.h"
#include "game/data/proto/replay.pb.h"
#include <array>

namespace ii {
namespace {
const char* kReplayVersion = "iispace";
const char* kLegacyReplayVersion = "WiiSPACE v1.3 replay";
const std::array<std::uint8_t, 2> kReplayEncryptionKey = {'<', '>'};
}  // namespace

struct ReplayReader::impl_t {
  proto::Replay replay;
  ii::initial_conditions conditions;
  std::size_t frame_index = 0;
};

ReplayReader::~ReplayReader() = default;
ReplayReader::ReplayReader(ReplayReader&&) noexcept = default;
ReplayReader& ReplayReader::operator=(ReplayReader&&) noexcept = default;

result<ReplayReader> ReplayReader::create(std::span<const std::uint8_t> bytes) {
  auto decompressed = decompress(crypt(bytes, kReplayEncryptionKey));
  if (!decompressed) {
    return unexpected("couldn't decompress replay: " + decompressed.error());
  }
  ReplayReader reader;
  reader.impl_ = std::make_unique<impl_t>();
  if (!reader.impl_->replay.ParseFromArray(decompressed->data(),
                                           static_cast<int>(decompressed->size()))) {
    return unexpected("couldn't parse replay");
  }
  if (reader.impl_->replay.game_version() != kReplayVersion &&
      reader.impl_->replay.game_version() != kLegacyReplayVersion) {
    return unexpected("unknown replay game version");
  }

  auto& conditions = reader.impl_->conditions;
  auto& replay = reader.impl_->replay;
  conditions.seed = replay.seed();
  switch (replay.compatibility()) {
  default:
    return unexpected("unknown replay compatibility level");
  case proto::Replay::CompatibilityLevel::kLegacy:
    conditions.compatibility = compatibility_level::kLegacy;
    break;
  case proto::Replay::CompatibilityLevel::kIispaceV0:
    conditions.compatibility = compatibility_level::kIispaceV0;
    break;
  }
  switch (replay.game_mode()) {
  default:
    return unexpected("unknown replay game mode");
  case proto::Replay::GameMode::kNormal:
    conditions.mode = game_mode::kNormal;
    break;
  case proto::Replay::GameMode::kBoss:
    conditions.mode = game_mode::kBoss;
    break;
  case proto::Replay::GameMode::kHard:
    conditions.mode = game_mode::kHard;
    break;
  case proto::Replay::GameMode::kFast:
    conditions.mode = game_mode::kFast;
    break;
  case proto::Replay::GameMode::kWhat:
    conditions.mode = game_mode::kWhat;
    break;
  }
  conditions.player_count = replay.players();
  if (replay.can_face_secret_boss()) {
    conditions.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
  }
  return {std::move(reader)};
}

ii::initial_conditions ReplayReader::initial_conditions() const {
  return impl_->conditions;
}

std::optional<input_frame> ReplayReader::next_input_frame() {
  if (impl_->frame_index >= static_cast<std::size_t>(impl_->replay.player_frame().size())) {
    return std::nullopt;
  }
  const auto& replay_frame = impl_->replay.player_frame(impl_->frame_index++);
  input_frame frame;
  frame.velocity = {fixed::from_internal(replay_frame.velocity_x()),
                    fixed::from_internal(replay_frame.velocity_y())};
  vec2 target{fixed::from_internal(replay_frame.target_x()),
              fixed::from_internal(replay_frame.target_y())};
  if (replay_frame.target_relative()) {
    frame.target_relative = target;
  } else {
    frame.target_absolute = target;
  }
  frame.keys = replay_frame.keys();
  return frame;
}

std::size_t ReplayReader::current_input_frame() const {
  return impl_->frame_index;
}

std::size_t ReplayReader::total_input_frames() const {
  return impl_->replay.player_frame().size();
}

ReplayReader::ReplayReader() = default;

struct ReplayWriter::impl_t {
  ii::initial_conditions conditions;
  proto::Replay replay;
};

ReplayWriter::~ReplayWriter() = default;
ReplayWriter::ReplayWriter(ReplayWriter&&) noexcept = default;
ReplayWriter& ReplayWriter::operator=(ReplayWriter&&) noexcept = default;

ReplayWriter::ReplayWriter(const ii::initial_conditions& conditions)
: impl_{std::make_unique<impl_t>()} {
  impl_->conditions = conditions;
  impl_->replay.set_game_version(kReplayVersion);
  impl_->replay.set_seed(conditions.seed);
  switch (conditions.compatibility) {
  case compatibility_level::kLegacy:
    impl_->replay.set_compatibility(proto::Replay::CompatibilityLevel::kLegacy);
    break;
  case compatibility_level::kIispaceV0:
    impl_->replay.set_compatibility(proto::Replay::CompatibilityLevel::kIispaceV0);
    break;
  }
  switch (conditions.mode) {
  case game_mode::kNormal:
    impl_->replay.set_game_mode(proto::Replay::GameMode::kNormal);
    break;
  case game_mode::kBoss:
    impl_->replay.set_game_mode(proto::Replay::GameMode::kBoss);
    break;
  case game_mode::kHard:
    impl_->replay.set_game_mode(proto::Replay::GameMode::kHard);
    break;
  case game_mode::kFast:
    impl_->replay.set_game_mode(proto::Replay::GameMode::kFast);
    break;
  case game_mode::kWhat:
    impl_->replay.set_game_mode(proto::Replay::GameMode::kWhat);
    break;
  case game_mode::kMax:
    break;
  }
  impl_->replay.set_players(conditions.player_count);
  impl_->replay.set_can_face_secret_boss(
      +(conditions.flags & ii::initial_conditions::flag::kLegacy_CanFaceSecretBoss));
}

void ReplayWriter::add_input_frame(const input_frame& frame) {
  auto& replay_frame = *impl_->replay.add_player_frame();
  replay_frame.set_velocity_x(frame.velocity.x.to_internal());
  replay_frame.set_velocity_y(frame.velocity.y.to_internal());
  if (frame.target_relative) {
    replay_frame.set_target_relative(true);
    replay_frame.set_target_x(frame.target_relative->x.to_internal());
    replay_frame.set_target_y(frame.target_relative->y.to_internal());
  } else if (frame.target_absolute) {
    replay_frame.set_target_x(frame.target_absolute->x.to_internal());
    replay_frame.set_target_y(frame.target_absolute->y.to_internal());
  }
  replay_frame.set_keys(frame.keys);
}

result<std::vector<std::uint8_t>> ReplayWriter::write() const {
  std::vector<std::uint8_t> data;
  data.resize(impl_->replay.ByteSizeLong());
  if (!impl_->replay.SerializeToArray(data.data(), static_cast<int>(data.size()))) {
    return unexpected("couldn't serialize replay");
  }
  auto compressed = compress(data);
  if (!compressed) {
    return unexpected("couldn't compress replay: " + compressed.error());
  }
  data = crypt(*compressed, kReplayEncryptionKey);
  return {std::move(data)};
}

const ii::initial_conditions& ReplayWriter::initial_conditions() const {
  return impl_->conditions;
}

ReplayInputAdapter::ReplayInputAdapter(ReplayReader& reader) : reader_{reader} {}

std::vector<input_frame>& ReplayInputAdapter::get() {
  frames_.clear();
  for (std::uint32_t i = 0; i < reader_.initial_conditions().player_count; ++i) {
    auto frame = reader_.next_input_frame();
    if (frame) {
      frames_.emplace_back(*frame);
    } else {
      frames_.emplace_back();
    }
  }
  return frames_;
}

}  // namespace ii
