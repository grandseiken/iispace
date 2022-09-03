#include "game/data/replay.h"
#include "game/common/math.h"
#include "game/data/crypt.h"
#include "game/data/proto/replay.pb.h"
#include <array>
#include <sstream>

namespace ii::data {
namespace {
const char* kReplayVersion = "iispace";
const char* kLegacyReplayVersion = "WiiSPACE v1.3 replay";
const char* kLegacyReplayEncryptionKey =
    "B';hGk9NenDlAb()]hv4<E1PnZ[[Smo=|PG*&R,/`Nqp_,bd%/"
    "yHg=AACMR%rdm!R~oegY5(z}R)6#3%pi!8v9{#WLl0WGkjYkP}^JUZe(tE}?e|-QuASKs##'YxSNc]>4s2ZUzo6|"
    "FlKU7y51e;IF|C2QKK%eJYAwK{z>}J&&.f'1U/k5KP3R~EbP`i!F}i/"
    "yeN`f]~YXF)6{>jU'K#{ySl&VJlQa5-m@\"1.:[Wxc&{^F?]E[8+wumL{:!-xrw0ic|vrm'UxmL^)3ZCQc9n=;6Z1f{/"
    "vJ^o*NJ1hB>MP7HUa,Recz>^JGT<Wy(k'Q}26Sz1_?x_hL^2~0jUN!Y'\"gX2C>%?8NP!1p$$PhrcrWPne+<{Bgx<"
    "S\"q\"EVX}4679X%jJE8qf$p7Ggv\"e%/,._3srqO{Y>oI;|r4K~X;$g63=0BmSDglsO(oJiQ3TDLdlv/"
    "Rwd^1=z%TC:b6e*eK;@7I?Vc|>9A_6<v\"J>(vG?Fr~ryyg&{;X>U^qb~Z.a^G*cCOJQUf{_*h)CJ=kcd#Q9VS)$3c'{`"
    "soJx/XV4Z6s0r4mMR~b2KN(k_?Ctx-VZ!N]Rx!2vde0@+=fbEGJ8$o`DpR3bewn.+_/ZZ5SRo\"~*A`rk0{oR+9/"
    "|~(_mX0O%1`*2:9m37Lkof$+(E.q*w%@r7|tA]nRtx<,&nSP|-/"
    "sJC#wT.A!J9uIPpy5nHZR8%uo9Lzt5QUN\\JyE1Z3^\\7(+$$1_oQSZd/AsK!FvfiCQ5q|c6FgiH73.L?Ml/"
    "F^5]+L%4A;w`/JGy%W-/"
    "%LcY8y4e}f|3=K[=\\OOhE-Y7E=4+tr+\\y~b\nRQQ6NQ$-FAG\\=2b|t4wBQs_@uCb(;{c?[&pq+=f-QG1&!tG1P$Eh*"
    "Bd9w|X1`>\\2jBoe1'63!~K3IwOJF.O[c~7.myzSc!Tx7R=,$qYJ=}@Or2P#c?{fPo*&?:!jfl10rRM4zW6O%0(@?arQ?"
    "XZUQ4A$Q5hL|fh.L}xhdJ07VDv2FQ=hi|ng9Ug%7+\"!)";
const std::array<std::uint8_t, 2> kReplayEncryptionKey = {'<', '>'};

result<proto::Replay> read_replay_file(std::span<const std::uint8_t> bytes) {
  proto::Replay result;
  auto decompressed = decompress(crypt(bytes, kReplayEncryptionKey));
  if (decompressed) {
    if (!result.ParseFromArray(decompressed->data(), static_cast<int>(decompressed->size()))) {
      return unexpected("not a valid replay file");
    }
    return {std::move(result)};
  }

  // Try parsing as legacy v1.3 replay.
  decompressed =
      decompress(crypt(bytes,
                       {reinterpret_cast<const std::uint8_t*>(kLegacyReplayEncryptionKey),
                        std::strlen(kLegacyReplayEncryptionKey)}));
  if (!decompressed) {
    return unexpected("couldn't decompress replay");
  }

  std::string dc_string{reinterpret_cast<const char*>(decompressed->data()),
                        reinterpret_cast<const char*>(decompressed->data()) + decompressed->size()};
  std::stringstream dc{dc_string, std::ios::in | std::ios::binary};
  std::string line;
  std::getline(dc, line);
  if (line != kLegacyReplayVersion) {
    return unexpected("not a valid replay file");
  }

  result.set_game_version(kLegacyReplayVersion);
  std::uint32_t players = 0;
  std::uint32_t seed = 0;
  proto::Replay::GameMode::Enum mode = proto::Replay::GameMode::kNormal;
  bool can_face_secret_boss = false;

  std::getline(dc, line);
  std::stringstream ss{line};
  ss >> players;
  ss >> can_face_secret_boss;

  bool bb = false;
  ss >> bb;
  mode = bb ? proto::Replay::GameMode::kBoss : mode;
  ss >> bb;
  mode = bb ? proto::Replay::GameMode::kHard : mode;
  ss >> bb;
  mode = bb ? proto::Replay::GameMode::kFast : mode;
  ss >> bb;
  mode = bb ? proto::Replay::GameMode::kWhat : mode;
  ss >> seed;

  result.set_players(std::max(1u, std::min(4u, players)));
  result.set_can_face_secret_boss(can_face_secret_boss);
  result.set_game_mode(mode);
  result.set_seed(seed);

  auto fixed_read = [](std::stringstream& in) {
    std::int64_t t = 0;
    in.read(reinterpret_cast<char*>(&t), sizeof(t));
    return t;
  };

  while (!dc.eof()) {
    proto::PlayerFrame& frame = *result.add_player_frame();
    frame.set_velocity_x(fixed_read(dc));
    frame.set_velocity_y(fixed_read(dc));
    frame.set_target_x(fixed_read(dc));
    frame.set_target_y(fixed_read(dc));
    char k = 0;
    dc.read(&k, sizeof(char));
    frame.set_keys(k);
  }
  return {std::move(result)};
}
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
  auto replay_proto = read_replay_file(bytes);
  if (!replay_proto) {
    return unexpected(replay_proto.error());
  }
  ReplayReader reader;
  reader.impl_ = std::make_unique<impl_t>();
  reader.impl_->replay = std::move(*replay_proto);
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
  if (impl_->frame_index >= total_input_frames()) {
    return std::nullopt;
  }
  auto read_frame = [&](const auto& replay_frame) {
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
  };
  return impl_->replay.player_frame().empty()
      ? read_frame(impl_->replay.legacy_player_frame(impl_->frame_index++))
      : read_frame(impl_->replay.player_frame(impl_->frame_index++));
}

std::vector<input_frame> ReplayReader::next_tick_input_frames() {
  std::vector<input_frame> frames;
  for (std::uint32_t i = 0; i < impl_->conditions.player_count; ++i) {
    auto frame = next_input_frame();
    if (frame) {
      frames.emplace_back(*frame);
    } else {
      frames.emplace_back();
    }
  }
  return frames;
}

std::size_t ReplayReader::current_input_frame() const {
  return impl_->frame_index;
}

std::size_t ReplayReader::total_input_frames() const {
  return impl_->replay.player_frame().empty() ? impl_->replay.legacy_player_frame().size()
                                              : impl_->replay.player_frame().size();
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

}  // namespace ii::data
