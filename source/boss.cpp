#include "boss.h"
#include "powerup.h"
#include "bossenemy.h"
#include "player.h"

// Generic boss
//------------------------------
Boss::Boss( const Vec2& position, z0Game::BossList boss, int hp, int players, int cycle )
: Ship( position )
, _flag( boss )
, _hp( 0 )
, _score( 0 )
, _damaged( 0 )
, _maxHp( 0 )
, _showHp( false )
{
    SetBoundingWidth( 640 );
    SetIgnoreDamageColourIndex( 100 );
    _score = 5000 * ( cycle + 1 ) + 2500 * ( boss > z0Game::BOSS_1C );
    _score = long( float( _score ) * ( 1.0f + float( players - 1 ) * HP_PER_EXTRA_PLAYER ) );
    float fhp = hp * 30;
    fhp *= 1.0f + float( cycle )       * HP_PER_EXTRA_CYCLE;
    fhp *= 1.0f + float( players - 1 ) * HP_PER_EXTRA_PLAYER;
    _hp = int( fhp );
    _maxHp = _hp;
}

void Boss::Damage( int damage, Player* source ) {
    int actualDamage = GetDamage( damage );
    if ( actualDamage <= 0 )
        return;

    if ( damage >= Player::BOMB_DAMAGE ) {
        Explosion();
        Explosion( 0xffffffff, 16 );
        Explosion( 0, 24 );

        _damaged = 25;
    } else if ( _damaged < 1 ) {
        _damaged = 1;
    }

    actualDamage *= ( 60 / ( 1 + CountPlayers() - ( GetLives() == 0 ? Player::CountKilledPlayers() : 0 ) ) );
    _hp -= actualDamage;

    if ( _hp <= 0 && !IsDestroyed() ) {
        Destroy();
        OnDestroy();
    } else if ( !IsDestroyed() ) {
        PlaySoundRandom( Lib::SOUND_ENEMY_SHATTER );
    }
}

void Boss::Render()
{
    RenderHPBar();

    if ( !_damaged ) {
        Ship::Render();
        return;
    }
    for ( unsigned int i = 0; i < CountShapes(); i++ ) {
        GetShape( i ).Render( GetLib(), GetPosition(), GetRotation(), int( i ) < _ignoreDamageColour ? 0xffffffff : 0 );
    }
    _damaged--;
}

void Boss::RenderHPBar()
{
    if ( !_showHp && IsOnScreen() )
        _showHp = true;

    if ( _showHp ) {
        Ship::RenderHPBar( float( _hp ) / float( _maxHp ) );
    }
}

void Boss::OnDestroy()
{
    SetKilled();
    z0Game::ShipList s = GetShipsInRadius( GetPosition(), Lib::WIDTH * Lib::HEIGHT );
    for ( unsigned int i = 0; i < s.size(); i++ ) {
        if ( s[ i ]->IsEnemy() && s[ i ] != this )
            s[ i ]->Damage( Player::BOMB_DAMAGE, 0 );
    }
    Explosion();
    Explosion( 0xffffffff, 12 );
    Explosion( GetShape( 0 ).GetColour(), 24 );
    Explosion( 0xffffffff, 36 );
    Explosion( GetShape( 0 ).GetColour(), 48 );
    for ( int i = 0; i < Lib::PLAYERS; i++ ) {
        GetLib().Rumble( i, 25 );
    }
    PlaySound( Lib::SOUND_EXPLOSION );

    z0Game::ShipList players = GetPlayers();

    int n = 0;
    for ( unsigned int i = 0; i < players.size(); i++ ) {
        Player* p = ( Player* )players[ i ];
        if ( !p->IsKilled() )
            n++;
    }

    for ( unsigned int i = 0; i < players.size(); i++ ) {
        Player* p = ( Player* )players[ i ];
        if ( !p->IsKilled() )
            p->AddScore( GetScore() / n );
    }
}


// Big square boss
//------------------------------
BigSquareBoss::BigSquareBoss( int players, int cycle )
: Boss( Vec2( Lib::WIDTH * 0.75, Lib::HEIGHT * 2.0f ), z0Game::BOSS_1A, BASE_HP, players, cycle )
, _dir( 0, -1 )
, _reverse( false )
, _timer( TIMER * 6 )
, _spawnTimer( 0 )
, _specialTimer( 0 )
, _specialAttack( false )
, _attackPlayer( 0 )
{
    AddShape( new Polygon( Vec2( 0, 0 ), 160, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ), 140, 4, 0x9933ffff, 0, ENEMY ) );
    AddShape( new Polygon( Vec2( 0, 0 ), 120, 4, 0x9933ffff, 0, ENEMY ) );
    AddShape( new Polygon( Vec2( 0, 0 ), 100, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ),  80, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ),  60, 4, 0x9933ffff, 0, VULNERABLE ) );

    AddShape( new Polygon( Vec2( 0, 0 ), 155, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ), 135, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ), 115, 4, 0x9933ffff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ),  95, 4, 0x6600ccff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ),  75, 4, 0x6600ccff, 0, 0 ) );
    AddShape( new Polygon( Vec2( 0, 0 ),  55, 4, 0x330099ff, 0, SHIELD ) );
}

void BigSquareBoss::Update()
{
    Vec2 pos = GetPosition();
    if ( pos._y < Lib::HEIGHT * 0.25f && _dir._y == -1 )
        _dir.Set( _reverse ? 1 : -1, 0 );
    if ( pos._x < Lib::WIDTH * 0.25f && _dir._x == -1 )
        _dir.Set( 0, _reverse ? -1 : 1 );
    if ( pos._y > Lib::HEIGHT * 0.75f && _dir._y == 1 )
        _dir.Set( _reverse ? -1 : 1, 0 );
    if ( pos._x > Lib::WIDTH * 0.75f && _dir._x == 1 )
        _dir.Set( 0, _reverse ? 1 : -1 );

    if ( _specialAttack ) {
        _specialTimer--;
        if ( _attackPlayer->IsKilled() ) {
            _specialTimer = 0;
            _attackPlayer = 0;
        }
        else if ( !_specialTimer ) {
            Vec2 d( ATTACK_RADIUS, 0 );
            for ( int i = 0; i < 6; i++ ) {
                Ship* s = new Follow( _attackPlayer->GetPosition() + d );
                s->SetRotation( M_PI / 4.0f );
                Spawn( s );
                d.Rotate( 2.0f * M_PI / 6.0f );
            }
            _attackPlayer = 0;
            PlaySound( Lib::SOUND_ENEMY_SPAWN );
        }
        if ( !_attackPlayer )
            _specialAttack = false;
    }
    else if ( IsOnScreen() ) {
        _timer--;
        if ( _timer <= 0 ) {
            _timer = ( GetLib().RandInt( 6 ) + 1 ) * TIMER;
            _dir = Vec2() - _dir;
            _reverse = !_reverse;
        }
        _spawnTimer++;
        if ( _spawnTimer >= STIMER - ( CountPlayers() - Player::CountKilledPlayers() ) * 10 ) {
            _spawnTimer = 0;
            _specialTimer++;
            Spawn( new BigFollow( GetPosition() ) );
            PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
        }
        if ( _specialTimer >= 8 && GetLib().RandInt( 4 ) ) {
            _specialTimer = ATTACK_TIME;
            _specialAttack = true;
            _attackPlayer = GetNearestPlayer();
            PlaySound( Lib::SOUND_BOSS_ATTACK );
        }
    }

    if ( _specialAttack )
        return;
        
    Move( _dir * SPEED );
    GetShape( 0  ).Rotate(  0.05f );
    GetShape( 1  ).Rotate( -0.10f );
    GetShape( 2  ).Rotate(  0.15f );
    GetShape( 3  ).Rotate( -0.20f );
    GetShape( 4  ).Rotate(  0.25f );
    GetShape( 5  ).Rotate( -0.30f );

    GetShape( 6  ).SetRotation( GetShape( 0 ).GetRotation() );
    GetShape( 7  ).SetRotation( GetShape( 1 ).GetRotation() );
    GetShape( 8  ).SetRotation( GetShape( 2 ).GetRotation() );
    GetShape( 9  ).SetRotation( GetShape( 3 ).GetRotation() );
    GetShape( 10 ).SetRotation( GetShape( 4 ).GetRotation() );
    GetShape( 11 ).SetRotation( GetShape( 5 ).GetRotation() );
}

void BigSquareBoss::Render()
{
    Boss::Render();
    if ( ( _specialTimer / 4 ) % 2 && _attackPlayer ) {
        Vec2 d( ATTACK_RADIUS, 0 );
        for ( int i = 0; i < 6; i++ ) {
            Vec2 p = _attackPlayer->GetPosition() + d;
            Polygon s( Vec2(), 10, 4, 0x9933ffff, M_PI / 4.0f, 0 );
            s.Render( GetLib(), p, 0 );
            d.Rotate( 2.0f * M_PI / 6.0f );
        }
    }
}

int BigSquareBoss::GetDamage( int damage )
{
    return damage;
}

// Shield bomb boss
//------------------------------
ShieldBombBoss::ShieldBombBoss( int players, int cycle )
: Boss( Vec2( -Lib::WIDTH / 2.0f, Lib::HEIGHT / 2.0f ), z0Game::BOSS_1B, BASE_HP, players, cycle )
, _timer( 0 )
, _count( 0 )
, _unshielded( 0 )
, _attack( 0 )
, _side( false )
{
    AddShape( new Polygram( Vec2(),  48,  8, 0x339966ff, 0, ENEMY ) );

    for ( int i = 0; i < 16; i++ ) {
        Vec2 a( 120, 0 );
        Vec2 b( 80,  0 );

        a.Rotate( i * M_PI / 8.0f );
        b.Rotate( i * M_PI / 8.0f );

        AddShape( new Line( Vec2(), a, b, 0x999999ff, 0 ) );
    }

    AddShape( new Polygon ( Vec2(), 130, 16, 0xccccccff, 0, SHIELD | ENEMY ) );
    AddShape( new Polygon ( Vec2(), 125, 16, 0xccccccff, 0, 0 ) );
    AddShape( new Polygon ( Vec2(), 120, 16, 0xccccccff, 0, 0 ) );

    AddShape( new Polygon ( Vec2(), 42, 16, 0, 0, SHIELD ) );

    SetIgnoreDamageColourIndex( 1 );
}

void ShieldBombBoss::Update()
{
    if ( !_side && GetPosition()._x < Lib::WIDTH * 0.6f )
        Move( Vec2( 1, 0 ) * SPEED );
    else if ( _side && GetPosition()._x > Lib::WIDTH * 0.4f )
        Move( Vec2( -1, 0 ) * SPEED );

    Rotate( -0.02f );
    GetShape( 0 ).Rotate( 0.04f );
    GetShape( 20 ).SetRotation( GetShape( 0 ).GetRotation() );

    if ( !IsOnScreen() )
        return;

    if ( _unshielded ) {
        _timer++;

        _unshielded--;
        for ( int i = 0; i < 3; i++ )
            GetShape( i + 17 ).SetColour( ( _unshielded / 2 ) % 4 ? 0x00000000 : 0x666666ff );
        for ( int i = 0; i < 16; i++ )
            GetShape( i + 1 ).SetColour( ( _unshielded / 2 ) % 4 ? 0x00000000 : 0x333333ff );

        if ( !_unshielded ) {
            GetShape( 0 ).SetCategory( ENEMY );
            GetShape( 17 ).SetCategory( SHIELD | ENEMY );

            for ( int i = 0; i < 3; i++ )
                GetShape( i + 17 ).SetColour( 0xccccccff );
            for ( int i = 0; i < 16; i++ )
                GetShape( i + 1 ).SetColour( 0x999999ff );

        }
    }

    if ( _attack ) {
        Vec2 d( _attackDir );
        d.Rotate( ( ATTACK_TIME - _attack ) * 0.5f * M_PI / float( ATTACK_TIME ) );
        Spawn( new SBBossShot( GetPosition(), d ) );
        _attack--;
        PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
    }

    _timer++;
    if ( _timer > TIMER ) {
        _count++;
        _timer = 0;

        if ( _count >= 4 && ( !GetLib().RandInt( 4 ) || _count >= 8 ) ) {
            _count = 0;
            if ( !_unshielded ) {
                Spawn( new Bomb( GetPosition() ) );
            }
        }

        if ( !GetLib().RandInt( 3 ) )
            _side = !_side;

        if ( GetLib().RandInt( 2 ) ) {
            Vec2 d( 5, 0 );
            d.Rotate( GetRotation() );
            for ( int i = 0; i < 12; i++ ) {
                Spawn( new SBBossShot( GetPosition(), d ) );
                d.Rotate( 2.0f * M_PI / 12.0f );
            }
            PlaySound( Lib::SOUND_BOSS_ATTACK );
        }
        else {
            _attack = ATTACK_TIME;
            _attackDir.SetPolar( GetLib().RandFloat() * 2.0f * M_PI, 5 );
        }
    }
}

int ShieldBombBoss::GetDamage( int damage )
{
    if ( _unshielded ) {
        return damage;
    }
    if ( damage >= Player::BOMB_DAMAGE && !_unshielded ) {
        _unshielded = UNSHIELD_TIME;
        GetShape( 0 ).SetCategory( VULNERABLE | ENEMY );
        GetShape( 17 ).SetCategory( 0 );
    }
    return 0;
}

// Chaser boss
//------------------------------
int ChaserBoss::_sharedHp;

ChaserBoss::ChaserBoss( int players, int cycle, int split, const Vec2& position, int time )
: Boss( !split ? Vec2( Lib::WIDTH / 2.0f, -Lib::HEIGHT / 2.0f ) : position, z0Game::BOSS_1C, 1 + BASE_HP / pow( 2, split ), players, cycle )
, _onScreen( split != 0 )
, _move( false )
, _timer( time )
, _dir()
, _players( players )
, _cycle( cycle )
, _split( split )
{
    AddShape( new Polygram( Vec2(),  10 * pow( 1.5, MAX_SPLIT - _split ), 5, 0x3399ffff, 0, 0 ) );
    AddShape( new Polygram( Vec2(),  9 * pow( 1.5, MAX_SPLIT - _split ), 5, 0x3399ffff, 0, 0 ) );
    AddShape( new Polygram( Vec2(),  8 * pow( 1.5, MAX_SPLIT - _split ), 5, 0, 0, ENEMY | VULNERABLE ) );
    AddShape( new Polygram( Vec2(),  7 * pow( 1.5, MAX_SPLIT - _split ), 5, 0, 0, SHIELD ) );
    SetIgnoreDamageColourIndex( 2 );
    SetBoundingWidth( 10 * pow( 1.5, MAX_SPLIT - _split ) );
    if ( !_split ) {
        _sharedHp = 0;
    }
}

void ChaserBoss::Update()
{
    if ( IsOnScreen() )
        _onScreen = true;

    if ( _timer )
        _timer--;
    if ( _timer <= 0 ) {
        _timer = TIMER * ( _move + 1 );
        _move = !_move;
        if ( _move ) {
            Ship* p = GetNearestPlayer();
            _dir = p->GetPosition() - GetPosition();
            _dir.Normalise();
            _dir *= SPEED * pow( 1.1, _split );
        }
    }
    if ( _move ) {
        Move( _dir );
    }
    else {
        Rotate( 0.05f + float( _split ) * 0.05f / float( MAX_SPLIT ) );
    }
    _sharedHp = 0;
}

void ChaserBoss::Render()
{
    Boss::Render();
    int totalHp = 0;
    int hp = 0;
    for ( int i = 0; i < 8; i++ ) {
        float fhp = 1 + BASE_HP / pow( 2, 7 - i );

        fhp *= 1.0f + float( _cycle )       * HP_PER_EXTRA_CYCLE;
        fhp *= 1.0f + float( _players - 1 ) * HP_PER_EXTRA_PLAYER;

        totalHp = 2 * totalHp + int( fhp );
        if ( i < 7 - _split )
            hp = 2 * hp + int( fhp );
    }
    hp = 2 * hp + GetRemainingHP();
    _sharedHp    += hp;
    if ( _onScreen )
        Ship::RenderHPBar( float( _sharedHp ) / float( totalHp ) );
}

int ChaserBoss::GetDamage( int damage )
{
    return damage;
}

void ChaserBoss::OnDestroy()
{
    if ( _split < MAX_SPLIT ) {
        for ( int i = 0; i < 2; i++ ) {
            Vec2 d;
            d.SetPolar( i * M_PI + GetRotation(), 8 * pow( 1.5, MAX_SPLIT - 1 - _split ) );
            Ship* s = new ChaserBoss( _players, _cycle, _split + 1, GetPosition() + d, ( i + 1 ) * TIMER / 2 );
            Spawn( s );
            s->SetRotation( GetRotation() );
        }
    }
    else {
        z0Game::ShipList remaining = GetShipsInRadius( Vec2( Lib::WIDTH / 2.0f, Lib::HEIGHT / 2.0f ), Lib::WIDTH * Lib::HEIGHT );
        bool last = true;
        for ( unsigned int i = 0; i < remaining.size(); i++ ) {
            if ( remaining[ i ]->IsEnemy() && !remaining[ i ]->IsDestroyed() && remaining[ i ] != this ) {
                last = false;
                break;
            }
        }
        if ( last ) {
            SetKilled();
            for ( int i = 0; i < Lib::PLAYERS; i++ ) {
                GetLib().Rumble( i, 25 );
            }
            z0Game::ShipList players = GetPlayers();
            int n = 0;
            for ( unsigned int i = 0; i < players.size(); i++ ) {
                Player* p = ( Player* )players[ i ];
                if ( !p->IsKilled() )
                    n++;
            }
            for ( unsigned int i = 0; i < players.size(); i++ ) {
                Player* p = ( Player* )players[ i ];
                if ( !p->IsKilled() )
                    p->AddScore( GetScore() / n );
            }
        }
    }

    for ( int i = 0; i < Lib::PLAYERS; i++ ) {
        GetLib().Rumble( i, _split < 3 ? 10 : 3 );
    }

    Explosion();
    Explosion( 0xffffffff, 12 );
    Explosion( GetShape( 0 ).GetColour(), 24 );
    Explosion( 0xffffffff, 36 );
    Explosion( GetShape( 0 ).GetColour(), 48 );

    if ( _split < 3 )
        PlaySound( Lib::SOUND_EXPLOSION );
    else
        PlaySoundRandom( Lib::SOUND_EXPLOSION );
}

// Tractor beam boss
//------------------------------
TractorBoss::TractorBoss( int players, int cycle )
: Boss( Vec2( Lib::WIDTH * 1.5f, Lib::HEIGHT / 2.0f ), z0Game::BOSS_2A, BASE_HP, players, cycle )
, _willAttack( false )
, _stopped( false )
, _generating( false )
, _attacking( false )
, _continue( false )
{

    _s1 = new CompoundShape( Vec2( 0, -96 ), 0, ENEMY | VULNERABLE );

    _s1->AddShape( new Polygram( Vec2( 0, 0 ), 12, 6, 0xcc33ccff, 0, 0 ) );
    _s1->AddShape( new Polygon( Vec2( 0, 0 ), 36, 12, 0xcc33ccff, 0, 0 ) );
    _s1->AddShape( new Polygon( Vec2( 0, 0 ), 34, 12, 0xcc33ccff, 0, 0 ) );
    _s1->AddShape( new Polygon( Vec2( 0, 0 ), 32, 12, 0xcc33ccff, 0, 0 ) );
    for ( int i = 0; i < 8; i++ ) {
        Vec2 d( 24, 0 );
        d.Rotate( i * M_PI / 4.0f );
        _s1->AddShape( new Polygram( d, 12, 6, 0xcc33ccff, 0, 0 ) );
    }

    _s2 = new CompoundShape( Vec2( 0, 96 ), 0, ENEMY | VULNERABLE );

    _s2->AddShape( new Polygram( Vec2( 0, 0 ), 12, 6, 0xcc33ccff, 0, 0 ) );
    _s2->AddShape( new Polygon( Vec2( 0, 0 ), 36, 12, 0xcc33ccff, 0, 0 ) );
    _s2->AddShape( new Polygon( Vec2( 0, 0 ), 34, 12, 0xcc33ccff, 0, 0 ) );
    _s2->AddShape( new Polygon( Vec2( 0, 0 ), 32, 12, 0xcc33ccff, 0, 0 ) );
    for ( int i = 0; i < 8; i++ ) {
        Vec2 d( 24, 0 );
        d.Rotate( i * M_PI / 4.0f );
        _s2->AddShape( new Polygram( d, 12, 6, 0xcc33ccff, 0, 0 ) );
    }

    AddShape( _s1 );
    AddShape( _s2 );

    _sAttack = new Polygon( Vec2( 0, 0 ), 0, 16, 0x993399ff );
    AddShape( _sAttack );

    AddShape( new Line( Vec2( 0, 0 ), Vec2( -2, -96 ), Vec2( -2, 96 ), 0xcc33ccff, 0 ) );
    AddShape( new Line( Vec2( 0, 0 ), Vec2(  0, -96 ), Vec2(  0, 96 ), 0xcc33ccff, 0 ) );
    AddShape( new Line( Vec2( 0, 0 ), Vec2(  2, -96 ), Vec2(  2, 96 ), 0xcc33ccff, 0 ) );

    AddShape( new Polygon( Vec2( 0,  96 ), 30, 12, 0, 0, SHIELD ) );
    AddShape( new Polygon( Vec2( 0, -96 ), 30, 12, 0, 0, SHIELD ) );
}

void TractorBoss::Update()
{
    if ( GetPosition()._x <= Lib::WIDTH / 2.0f && _willAttack && !_stopped && !_continue ) {
        _stopped = true;
        _generating = true;
        _timer = 0;
    }

    if ( GetPosition()._x < -150.0f ) {
        SetPosition( Vec2( Lib::WIDTH + 150.0f, GetPosition()._y ) );
        _willAttack = !_willAttack;
        _continue = false;
    }

    _timer++;
    if ( !_stopped ) {
        Move( Vec2( -1, 0 ) * SPEED );
        if ( !_willAttack && _timer % ( 16 - ( CountPlayers() - Player::CountKilledPlayers() ) * 2 ) == 0 && IsOnScreen() ) {
            Player* p = GetNearestPlayer();
            Vec2 v = GetPosition();

            Vec2 d = p->GetPosition() - v;
            d.Normalise();
            Spawn( new SBBossShot( v, d * 5.0f, 0xcc33ccff ) );
            PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
        }
    } else {
        if ( _generating ) {
            if ( _timer >= TIMER * 5 ) {
                _timer = 0;
                _generating = false;
                _attacking  = false;
                _attackSize = 0;
                PlaySound( Lib::SOUND_BOSS_ATTACK );
            }

            if ( _timer % ( 10 - 2 * ( CountPlayers() - Player::CountKilledPlayers() ) ) == 0 && _timer < TIMER * 4 ) {
                Ship* s = new TBossShot( _s1->ConvertPoint( GetPosition(), GetRotation(), Vec2() ), GetRotation() + M_PI );
                Spawn( s );
                s = new TBossShot( _s2->ConvertPoint( GetPosition(), GetRotation(), Vec2() ), GetRotation() );
                Spawn( s );
                PlaySoundRandom( Lib::SOUND_ENEMY_SPAWN );
            }
        }
        else {
            if ( !_attacking ) {
                if ( _timer >= TIMER * 4 ) {
                    _timer     = 0;
                    _attacking = true;
                }
                _targets.clear();
                z0Game::ShipList t = GetShipsInRadius( GetPosition(), 640 );
                for ( unsigned int i = 0; i < t.size(); i++ ) {
                    if ( t[ i ] == this || ( t[ i ]->IsPlayer() && ( ( Player* )t[ i ] )->IsKilled() )
                         || !( t[ i ]->IsPlayer() || t[ i ]->IsEnemy() ) )
                        continue;
                    Vec2 pos = t[ i ]->GetPosition();
                    _targets.push_back( pos );
                    float speed = 0;
                    if ( t[ i ]->IsPlayer() )
                        speed = Tractor::TRACTOR_SPEED;
                    if ( t[ i ]->IsEnemy() )
                        speed = 4.5f;
                    Vec2 d = GetPosition() - pos;
                    d.Normalise();
                    t[ i ]->Move( d * speed );
                    if ( t[ i ]->IsEnemy() && ( t[ i ]->GetPosition() - GetPosition() ).Length() <= 40 ) {
                        t[ i ]->Destroy();
                        _attackSize++;
                        _sAttack->SetRadius( float( _attackSize ) / 1.5f );
                    }
                }

            }
            else {
                if ( _timer >= TIMER ) {
                    _timer   = 0;
                    _stopped = false;
                    _continue = true;
                    for ( int i = 0; i < _attackSize; i++ ) {
                        Vec2 d;
                        d.SetPolar( i * 2.0f * M_PI / float( _attackSize ), 5.0f );
                        Spawn( new SBBossShot( GetPosition(), d, 0xcc33ccff ) );
                    }
                    PlaySound( Lib::SOUND_BOSS_FIRE );
                    _attackSize = 0;
                    _sAttack->SetRadius( 0 );
                }
            }
        }
    }

    float r = 0;
    if ( !_willAttack || ( _stopped && !_generating && !_attacking ) )
        r = 0.01f;
    else if ( _stopped && _attacking && !_generating )
        r = 0;
    else
        r = 0.005;

    Rotate( r );

    _s1->Rotate(  0.05f );
    _s2->Rotate( -0.05f );
    for ( int i = 0; i < 8; i++ ) {
        _s1->GetShape( i + 4 ).Rotate( -0.1f );
        _s2->GetShape( i + 4 ).Rotate(  0.1f );
    }
}

void TractorBoss::Render()
{
    Boss::Render();
    if ( _stopped && !_generating && !_attacking ) {
        for ( unsigned int i = 0; i < _targets.size(); i++ ) {
            if ( ( ( _timer + i * 4 ) / 4 ) % 2 ) {
                GetLib().RenderLine( GetPosition(), _targets[ i ], 0xcc33ccff );
            }
        }
    }
}

int TractorBoss::GetDamage( int damage )
{
    return damage;
}

// Ghost boss
//------------------------------
GhostBoss::GhostBoss( int players, int cycle )
: Boss( Vec2( Lib::WIDTH / 2.0f, Lib::HEIGHT / 2.0f ), z0Game::BOSS_2B, BASE_HP, players, cycle )
, _visible( false )
, _vTime( 0 )
, _timer( 0 )
, _attackTime( ATTACK_TIME )
, _attack( 0 )
, _rDir( false )
, _startTime( 120 )
{
    AddShape( new Polygon( Vec2(), 32, 8, 0x9933ccff, 0, 0 ) );
    AddShape( new Polygon( Vec2(), 48, 8, 0xcc66ffff, 0, 0 ) );

    for ( int i = 0; i < 8; i++ ) {
        Vec2 c;
        c.SetPolar( i * M_PI / 4.0f, 48 );

        CompoundShape* s = new CompoundShape( c, 0, 0 );
        for ( int j = 0; j < 8; j++ ) {
            Vec2 d;
            d.SetPolar( j * M_PI / 4.0f, 1.0f );

            s->AddShape( new Line( Vec2(), d * 10.0f, d * 20.0f, 0x9933ccff, 0 ) );

        }

        AddShape( s );
    }

    AddShape( new Polygram( Vec2(), 24, 8, 0xcc66ffff, 0, 0 ) );

    for ( int n = 0; n < 4; n++ ) {
        CompoundShape* s = new CompoundShape( Vec2(), 0, 0 );
        for ( int i = 0; i < 16 + n * 6; i++ ) {
            Vec2 d;
            d.SetPolar( i * 2.0f * M_PI / ( 16 + n * 6 ), 100 + n * 60 );
            s->AddShape( new Polystar( d, 16, 8, n ? 0x330066ff : 0x9933ccff ) );
            s->AddShape( new Polygon ( d, 12, 8, n ? 0x330066ff : 0x9933ccff ) );
        }
        AddShape( s );
    }

    SetIgnoreDamageColourIndex( 12 );

}

void GhostBoss::Update()
{

    if ( !( _attack == 2 && _attackTime < ATTACK_TIME * 4 && _attackTime ) )
        Rotate( -0.04f );

    for ( int i = 0; i < 8; i++ ) {
        GetShape( i + 2 ).Rotate( 0.04f );
    }
    GetShape( 10 ).Rotate( 0.06f );
    for ( int n = 0; n < 4; n++ ) {
        CompoundShape* s = ( CompoundShape* )( &GetShape( 11 + n ) );
        if ( n % 2 )
            s->Rotate( 0.03f + 0.0015f * n );
        else
            s->Rotate( 0.05f - 0.0015f * n );
        for ( int i = 0; i < s->CountShapes(); i++ )
            s->GetShape( i ).Rotate( -0.1f );
    }

    if ( _startTime ) {
        _startTime--;
        return;
    }

    if ( !_attackTime ) {
        _timer++;
        if ( _timer == 9 * TIMER / 10 ) {
            for ( int i = 0; i < 8; i++ ) {
                Vec2 d;
                d.SetPolar( i * M_PI / 4.0f + GetRotation(), 5.0f );
                Spawn( new SBBossShot( GetPosition(), d, 0xcc66ffff ) );
            }
        }
        else if ( _timer >= TIMER / 10 && _timer < 9 * TIMER / 10 - 16 && _timer % 16 == 0 ) {
            Player* p = GetNearestPlayer();
            Vec2 d = p->GetPosition() - GetPosition();
            d.Normalise();

            if ( d.Length() > 0.5f ) {
                Spawn( new SBBossShot( GetPosition(), d *    5.0f, 0xcc66ffff ) );
                Spawn( new SBBossShot( GetPosition(), d * ( -5.0f ), 0xcc66ffff ) );
                PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
            }
        }
    }
    else {
        _attackTime--;

        if ( _attack == 2 ) {
            if ( _attackTime >= ATTACK_TIME * 4 && _attackTime % 8 == 0 ) {
                Vec2 pos( GetLib().RandInt( Lib::WIDTH + 1 ), GetLib().RandInt( Lib::HEIGHT + 1 ) );
                Spawn( new GhostMine( pos, this ) );
                PlaySoundRandom( Lib::SOUND_ENEMY_SPAWN );
            }
            else if ( _attackTime == ATTACK_TIME * 4 - 1 ) {
                _visible = true;
                _vTime = 60;
                AddShape( new Box( Vec2( Lib::WIDTH / 2.0f + 32.0f, 0 ), Lib::WIDTH / 2.0f, 10.0f, 0xcc66ffff, 0, 0 ) );
                AddShape( new Box( Vec2( Lib::WIDTH / 2.0f + 32.0f, 0 ), Lib::WIDTH / 2.0f, 7.0f,  0xcc66ffff, 0, 0 ) );
                AddShape( new Box( Vec2( Lib::WIDTH / 2.0f + 32.0f, 0 ), Lib::WIDTH / 2.0f, 4.0f,  0xcc66ffff, 0, 0 ) );

                AddShape( new Box( Vec2( -Lib::WIDTH / 2.0f - 32.0f, 0 ), Lib::WIDTH / 2.0f, 10.0f, 0xcc66ffff, 0, 0 ) );
                AddShape( new Box( Vec2( -Lib::WIDTH / 2.0f - 32.0f, 0 ), Lib::WIDTH / 2.0f, 7.0f,  0xcc66ffff, 0, 0 ) );
                AddShape( new Box( Vec2( -Lib::WIDTH / 2.0f - 32.0f, 0 ), Lib::WIDTH / 2.0f, 4.0f,  0xcc66ffff, 0, 0 ) );
                PlaySound( Lib::SOUND_BOSS_FIRE );
            }
            else if ( _attackTime < ATTACK_TIME * 3 ) {
                if ( _attackTime == ATTACK_TIME * 3 - 1 )
                    PlaySound( Lib::SOUND_BOSS_ATTACK );
                Rotate( ( _rDir ? 1 : -1 ) * 2.0f * M_PI / ( ATTACK_TIME * 6.0f ) );
                if ( !_attackTime ) {
                    for ( int i = 0; i < 6; i++ )
                        DestroyShape( 15 );
                }
            }
        }

        if ( _attack == 1 && _attackTime == ATTACK_TIME * 2 ) {
            _rDir = GetLib().RandInt( 2 );
            Spawn( new GhostWall( !_rDir, false, 0 ) );
        }
        if ( _attack == 1 && _attackTime == ATTACK_TIME * 1 ) {
            Spawn( new GhostWall( _rDir,  true,  0 ) );
        }
        if ( !_attackTime && !_visible ) {
            _visible = true;
            _vTime   = 60;
        }
    }

    if ( _timer >= TIMER ) {
        _timer   = 0;
        _visible = false;
        _vTime   = 60;
        _attack  = GetLib().RandInt( 3 );
        GetShape( 0  ).SetCategory( 0 );
        GetShape( 1  ).SetCategory( 0 );
        GetShape( 11 ).SetCategory( 0 );
        if ( CountShapes() >= 16 ) {
            GetShape( 15 ).SetCategory( 0 );
            GetShape( 18 ).SetCategory( 0 );
        }
        PlaySound( Lib::SOUND_BOSS_ATTACK );

        if ( _attack == 0 ) {
            _attackTime = ATTACK_TIME * 2;
            bool r = GetLib().RandInt( 2 );
            Spawn( new GhostWall( r,  true  ) );
            Spawn( new GhostWall( !r, false ) );
        }
        if ( _attack == 1 ) {
            _attackTime = ATTACK_TIME * 3;
            for ( int i = 0; i < CountPlayers(); i++ )
                Spawn( new MagicShield( GetPosition() ) );
        }
        if ( _attack == 2 ) {
            _attackTime = ATTACK_TIME * 6;
            _rDir = GetLib().RandInt( 2 );
        }
    }

    if ( _vTime ) {
        _vTime--;
        if ( !_vTime && _visible ) {
            GetShape( 0  ).SetCategory( SHIELD );
            GetShape( 1  ).SetCategory( ENEMY | VULNERABLE );
            GetShape( 11 ).SetCategory( ENEMY );
            if ( CountShapes() >= 16 ) {
                GetShape( 15 ).SetCategory( ENEMY );
                GetShape( 18 ).SetCategory( ENEMY );
            }
        }
    }
}

void GhostBoss::Render()
{
    if ( ( _startTime / 4 ) % 2 == 1 ) {
        RenderWithColour( 1 );
        return;
    }
    if ( ( _visible && ( ( _vTime / 4 ) % 2 == 0 ) ) || ( !_visible && ( ( _vTime / 4 ) % 2 == 1 ) ) ) {
        Boss::Render();
        return;
    }
    RenderWithColour( 0x330066ff );
    if ( !_startTime )
        RenderHPBar();
}

int GhostBoss::GetDamage( int damage )
{
    return damage;
}

// Death ray boss
//------------------------------
DeathRayBoss::DeathRayBoss( int players, int cycle )
: Boss( Vec2( Lib::WIDTH * 0.15f, -Lib::HEIGHT ), z0Game::BOSS_2C, BASE_HP, players, cycle )
, _timer( TIMER * 2 )
, _laser( false )
, _dir( true )
, _pos( 1 )
, _armTimer( 0 )
, _rayAttackTimer( 0 )
, _raySrc1()
, _raySrc2()
{
    AddShape( new Polygram( Vec2(), 70,  12, 0x33ff99ff, M_PI / 12.0f, 0 ) );
    AddShape( new Polygon ( Vec2(), 120, 12, 0x33ff99ff, M_PI / 12.0f, ENEMY | VULNERABLE ) );
    AddShape( new Polygon ( Vec2(), 115, 12, 0x33ff99ff, M_PI / 12.0f, 0 ) );
    AddShape( new Polygon ( Vec2(), 110, 12, 0x33ff99ff, M_PI / 12.0f, SHIELD ) );
    AddShape( new Polystar( Vec2(), 110, 12, 0x33ff99ff, M_PI / 12.0f, 0 ) );

    CompoundShape* s1 = new CompoundShape( Vec2(), 0, ENEMY );
    for ( int i = 1; i < 12; i++ ) {
        CompoundShape* s2 = new CompoundShape( Vec2(), i * M_PI / 6.0f, 0 );
        s2->AddShape( new Box( Vec2( 130, 0 ), 10, 24, 0x33ff99ff, 0, 0 ) );
        s1->AddShape( s2 );
    }
    AddShape( s1 );

    SetIgnoreDamageColourIndex( 5 );
}

void DeathRayBoss::Update()
{
    bool positioned = true;
    float d = _pos == 0 ? Lib::HEIGHT / 4.0f - GetPosition()._y :
              _pos == 1 ? Lib::HEIGHT / 2.0f - GetPosition()._y :
                   3.0f * Lib::HEIGHT / 4.0f - GetPosition()._y ;
    if ( abs( d ) > 3 ) {
        Move( Vec2( 0, d / abs( d ) ) * SPEED );
        positioned = false;
    }

    if ( _rayAttackTimer ) {
        _rayAttackTimer--;
        if ( _rayAttackTimer == 40 ) {
            _rayDest = GetNearestPlayer()->GetPosition();
        }
        if ( _rayAttackTimer < 40 ) {
            Vec2 d = _rayDest - GetPosition();
            d.Normalise();
            Spawn( new SBBossShot( GetPosition(), d * 10.0f, 0xccccccff ) );
            PlaySoundRandom( Lib::SOUND_BOSS_ATTACK );
            Explosion();
        }
    }

    if ( _laser ) {

        if ( _timer ) {
            if ( positioned )
                _timer--;
            if ( _timer < TIMER * 2 && !( _timer % 4 ) ) {
                Spawn( new DeathRay( GetPosition() + Vec2( 100, 0 ) ) );
                PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
            }
            if ( !_timer ) {
                _laser = false;
                _timer = TIMER * ( 2 + GetLib().RandInt( 3 ) );
            }
        }
        else {
            float r = GetRotation();
            if ( r == 0 )
                _timer = TIMER * 2;
            if ( r < 0.1f || r > 2.0f * M_PI - 0.1f )
                SetRotation( 0 );
            else
                Rotate( _dir ? 0.1f : -0.1f );
        }
    }
    else {
        Rotate( _dir ? 0.02f : -0.02f );
        if ( IsOnScreen() ) {
            _timer--;
            if ( _timer % TIMER == 0 && _timer != 0 && !GetLib().RandInt( 4 ) ) {
                _dir = GetLib().RandInt( 2 );
                _pos = GetLib().RandInt( 3 );
            }
            if ( _timer == TIMER * 2 + 50 && _arms.size() == 2 ) {
                _rayAttackTimer = RAY_TIMER;
                _raySrc1 = _arms[ 0 ]->GetPosition();
                _raySrc2 = _arms[ 1 ]->GetPosition();
                PlaySound( Lib::SOUND_ENEMY_SPAWN );
            }
        }
        if ( _timer <= 0 ) {
            _laser = true;
            _timer = 0;
            _pos = GetLib().RandInt( 3 );
        }
    }

    if ( !_arms.size() ) {
        _armTimer++;
        if ( _armTimer % 8 == 0 ) {
            int n = ( _armTimer / 8 ) % 12;
            Vec2 d( 1, 0 );
            d.Rotate( GetRotation() + n * M_PI / 6 );
            Ship* s = new SBBossShot( GetPosition() + d * 120.0f, d * 5.0f, 0x33ff99ff );
            Spawn( s );
            if ( s->IsOnScreen() )
                PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
        }
        if ( _armTimer >= ARM_RTIMER ) {
            _armTimer = 0;
            int hp = int( float( ARM_HP ) * ( 0.7f + 0.3f * ( GetLives() ? CountPlayers() : CountPlayers() - Player::CountKilledPlayers() ) ) );
            _arms.push_back( new DeathArm( this, true,  hp ) );
            _arms.push_back( new DeathArm( this, false, hp ) );
            PlaySound( Lib::SOUND_ENEMY_SPAWN );
            Spawn( _arms[ 0 ] );
            Spawn( _arms[ 1 ] );
        }
    }
}

void DeathRayBoss::Render()
{
    Boss::Render();
    for ( int i = _rayAttackTimer - 8; i <= _rayAttackTimer + 8; i++ ) {
        if ( i < 40 || i > RAY_TIMER )
            continue;

        Vec2 d = _raySrc1 - GetPosition();
        d *= float( i - 40 ) / float( RAY_TIMER - 40 );
        Polystar s( Vec2(), 10, 6, 0x999999ff, 0, 0 );
        s.Render( GetLib(), d + GetPosition(), 0 );

        d = _raySrc2 - GetPosition();
        d *= float( i - 40 ) / float( RAY_TIMER - 40 );
        Polystar s2( Vec2(), 10, 6, 0x999999ff, 0, 0 );
        s2.Render( GetLib(), d + GetPosition(), 0 );
    }
}

int DeathRayBoss::GetDamage( int damage )
{
    return _arms.size() ? 0 : damage;
}

void DeathRayBoss::OnArmDeath( Ship* arm )
{
    for ( unsigned int i = 0; i < _arms.size(); i++ ) {
        if ( _arms[ i ] == arm ) {
            _arms.erase( _arms.begin() + i );
            break;
        }
    }
}
