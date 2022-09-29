#include "game/data/replay.h"
#include "game/common/math.h"
#include "game/data/conditions.h"
#include "game/data/crypt.h"
#include "game/data/input_frame.h"
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
    return unexpected("not a valid replay file");
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
  proto::GameMode::Enum mode = proto::GameMode::kNormal;
  bool can_face_secret_boss = false;

  std::getline(dc, line);
  std::stringstream ss{line};
  ss >> players;
  ss >> can_face_secret_boss;

  bool bb = false;
  ss >> bb;
  mode = bb ? proto::GameMode::kBoss : mode;
  ss >> bb;
  mode = bb ? proto::GameMode::kHard : mode;
  ss >> bb;
  mode = bb ? proto::GameMode::kFast : mode;
  ss >> bb;
  mode = bb ? proto::GameMode::kWhat : mode;
  ss >> seed;

  auto& conditions = *result.mutable_conditions();
  conditions.set_compatibility(proto::CompatibilityLevel::kLegacy);
  conditions.set_seed(seed);
  conditions.set_player_count(std::max(1u, std::min(4u, players)));
  conditions.set_game_mode(mode);
  if (can_face_secret_boss) {
    conditions.set_flags(proto::InitialConditions::kLegacy_CanFaceSecretBoss);
  }

  auto fixed_read = [](std::stringstream& in) {
    std::int64_t t = 0;
    in.read(reinterpret_cast<char*>(&t), sizeof(t));
    return t;
  };

  while (!dc.eof()) {
    proto::InputFrame& frame = *result.add_player_frame();
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
  auto conditions_result = read_initial_conditions(replay.conditions());
  if (!conditions_result) {
    return unexpected(conditions_result.error());
  }
  conditions = *conditions_result;
  return {std::move(reader)};
}

ii::initial_conditions ReplayReader::initial_conditions() const {
  return impl_->conditions;
}

std::optional<input_frame> ReplayReader::next_input_frame() {
  if (impl_->frame_index >= total_input_frames()) {
    return std::nullopt;
  }
  return read_input_frame(impl_->replay.player_frame(impl_->frame_index++));
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
  *impl_->replay.mutable_conditions() = write_initial_conditions(conditions);
}

void ReplayWriter::add_input_frame(const input_frame& frame) {
  *impl_->replay.add_player_frame() = write_input_frame(frame);
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
