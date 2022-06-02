#ifndef II_GAME_MIXER_SOUND_H
#define II_GAME_MIXER_SOUND_H

namespace ii {

enum class sound {
  kPlayerDestroy,
  kPlayerRespawn,
  kPlayerFire,
  kPlayerShield,
  kExplosion,
  kEnemyHit,
  kEnemyDestroy,
  kEnemyShatter,
  kEnemySpawn,
  kBossAttack,
  kBossFire,
  kPowerupLife,
  kPowerupOther,
  kMenuClick,
  kMenuAccept,
  kMax,
};

}  // namespace ii

#endif