#include "overmind.h"
#include "enemy.h"
#include "boss.h"
#include "powerup.h"
#include "player.h"
#include "stars.h"
#include <algorithm>

int     Overmind::_tRow;
z0Game* Overmind::_tz0;
int     Overmind::_power = 0;
int     Overmind::_hardAlready = 0;

Overmind::Overmind( z0Game& z0 )
: _z0( z0 ),
_timer( POWERUP_TIME ),
_count( 0 ),
_nonWallCount( 0 ),
_levelsMod( 0 ),
_groupsMod( 0 ),
_bossModBosses( 0 ),
_bossModFights( 0 ),
_bossModSecret( 0 ),
_canFaceSecretBoss( false ),
_isBossNext( false ),
_isBossLevel( false ),
_bossesToGo( 0 )
{
    _tz0  = 0;
    _tRow = 0;
    AddFormations();
}

int Overmind::RandInt( int lessThan ) const
{
    return _z0.GetLib().RandInt( lessThan );
}

int Overmind::HackRandInt( int lessThan )
{
    if ( _tz0 )
        return _tz0->GetLib().RandInt( lessThan );
    return 0;
}

void Overmind::Reset( bool canFaceSecretBoss )
{
    int a, b, c;
    _boss1Queue.clear();
    _boss2Queue.clear();

    a = RandInt( 3 );
    b = RandInt( 2 );
    if ( a == 0 || ( a == 1 && b == 1 ) )
        b++;
    c = ( a + b == 3 ) ? 0 :
            ( a + b == 2 ) ? 1 : 2;

    _boss1Queue.push_back( a );
    _boss1Queue.push_back( b );
    _boss1Queue.push_back( c );

    a = RandInt( 3 );
    b = RandInt( 2 );
    if ( a == 0 || ( a == 1 && b == 1 ) )
        b++;
    c = ( a + b == 3 ) ? 0 :
            ( a + b == 2 ) ? 1 : 2;

    _boss2Queue.push_back( a );
    _boss2Queue.push_back( b );
    _boss2Queue.push_back( c );

    _isBossNext  = false;
    _isBossLevel = false;
    _levelsMod   = 0;
    _groupsMod   = 0;
    _bossModBosses = 0;
    _bossModFights = 0;
    _bossModSecret = 0;
    _canFaceSecretBoss = canFaceSecretBoss;
    _powerupMod  = 0;
    _livesTarget = 0;
    _startTime   = long( time( 0 ) );
    _elapsedTime = 0;
    _timeStopped = false;
    _bossesToGo  = 0;
    _wavesTotal  = 0;

    _starDir = Vec2f( 0, 1 );
    _starCount = 0;

    if ( _z0.IsBossMode() ) {
        _power = 0;
        _timer = 0;
        return;
    }
    _power = INITIAL_POWER + 2 - _z0.CountPlayers() * 2;
    if ( _z0.IsHardMode() ) {
        _power += 20;
        _wavesTotal = 15;
    }
    _timer = POWERUP_TIME;
    _bossRestTimer = BOSS_REST_TIME / 8;

    Star::SetDirection( _starDir );
}

void Overmind::AddFormation( SpawnFormationFunction function )
{
    _formations.push_back( Formation( function( true, 0 ), function ) );
    std::stable_sort( _formations.begin(), _formations.end(), &SortFormations );
}

void Overmind::Update() {

    ++_elapsedTime;
    Star::Update();
    int r = _starCount > 1 ? RandInt( _starCount ) : 0;
    for ( int i = 0; i < r; i++ )
        Star::CreateStar( _z0 );

    if ( _z0.IsBossMode() ) {
        if ( _count <= 0 ) {
            _starDir.Rotate( ( z_float( _z0.GetLib().RandFloat() ) - 0.5f ) * M_PIf );
            Star::SetDirection( _starDir );
            _starCount = RandInt( 3 ) + 2;
            if ( _bossModBosses < 6 ) {
                if ( _bossModBosses )
                    for ( int i = 0; i < _z0.CountPlayers(); ++i )
                        SpawnBossReward();
                BossModeBoss();
            }
            if ( _bossModBosses < 7 ) {
                _bossModBosses++;
            }
        }
        return;
    }

    if ( _bossRestTimer > 0 ) {
        --_bossRestTimer;
        return;
    }

    _timer++;
    if ( _timer == POWERUP_TIME && !_isBossLevel ) {
        SpawnPowerup();
    }

    int bossCycles = _bossModFights;
    int triggerStage = _groupsMod + bossCycles + 2 * ( _z0.IsHardMode() ? 1 : 0 );
    int triggerVal = INITIAL_TRIGGERVAL;
    int i = 0;
    while ( triggerStage > 0 ) {
        if ( i < 2 )
            triggerVal += 4;
        else if ( i < 7 - _z0.CountPlayers() )
            triggerVal += 3;
        else
            triggerVal += 2;
        --triggerStage;
        ++i;
    }
    if ( triggerVal < 0 || _isBossLevel || _isBossNext )
        triggerVal = 0;

    if ( ( _timer > TIMER && !_isBossLevel && !_isBossNext ) || _count <= triggerVal ) {
        if ( _timer < POWERUP_TIME && !_isBossLevel )
            SpawnPowerup();

        _timer = 0;
        _starDir.Rotate( ( z_float( _z0.GetLib().RandFloat() ) - 0.5f ) * M_PIf );
        Star::SetDirection( _starDir );
        _starCount = RandInt( 3 ) + 2;

        if ( _isBossLevel ) {
            ++_bossModBosses;
            _isBossLevel = false;
            if ( _bossesToGo <= 0 ) {
                SpawnBossReward();
                ++_bossModFights;
                _power += 2 - 2 * _z0.CountPlayers();
                _bossRestTimer = BOSS_REST_TIME;
                _bossesToGo = 0;
            }
            else
                _bossRestTimer = BOSS_REST_TIME / 4;
        }
        else {
            if ( _isBossNext ) {
                --_bossesToGo;
                _isBossNext  = _bossesToGo > 0;
                _isBossLevel = true;
                Boss();
            } else {
                Wave();
                if ( _wavesTotal < 5 )
                    _power += 2;
                else
                    ++_power;
                ++_wavesTotal;
                ++_levelsMod;
            }
        }

        if ( _levelsMod >= LEVELS_PER_GROUP ) {
            _levelsMod = 0;
            _groupsMod++;
        }
        if ( _groupsMod >= BASE_GROUPS_PER_BOSS + _z0.CountPlayers() ) {
            _groupsMod  = 0;
            _isBossNext = true;
            _bossesToGo = _bossModFights >= 4 ? 3 : _bossModFights >= 2 ? 2 : 1;
        }
    }
}

void Overmind::OnEnemyDestroy( Ship* ship )
{
    _count -= ship->GetEnemyValue();
    if ( !ship->IsWall() )
        _nonWallCount--;
}
void Overmind::OnEnemyCreate( Ship* ship )
{ 
    _count += ship->GetEnemyValue();
    if ( !ship->IsWall() )
        _nonWallCount++;
}

// Individual spawns
//------------------------------
void Overmind::SpawnPowerup()
{
    z0Game::ShipList existing = _z0.GetShipsInRadius( Vec2( Lib::WIDTH / 2, Lib::HEIGHT / 2 ), Lib::WIDTH * Lib::HEIGHT );
    int count = 0;
    for ( unsigned int i = 0; i < existing.size(); i++ ) {
        if ( existing[ i ]->IsPowerup() )
            count++;
        if ( count >= 4 )
            return;
    }

    ++_powerupMod;
    if ( _powerupMod == 4 ) {
        _powerupMod = 0;
        ++_livesTarget;
    }

    int r = RandInt( 4 );
    Vec2 v;

    if ( r == 0 )
        v.Set( -Lib::WIDTH,    Lib::HEIGHT / 2 );
    else if ( r == 1 )
        v.Set( Lib::WIDTH * 2, Lib::HEIGHT / 2 );
    else if ( r == 2 )
        v.Set( Lib::WIDTH / 2, -Lib::HEIGHT    );
    else
        v.Set( Lib::WIDTH / 2, Lib::HEIGHT * 2 );

    int m = 4;
    for ( int i = 1; i <= Lib::PLAYERS; i++ ) {
        if ( _z0.GetLives() <= _z0.CountPlayers() - i )
            m++;
    }
    if ( !_z0.GetLives() )
        m += int( Player::CountKilledPlayers() );
    if ( _livesTarget > 0 )
        m += _livesTarget;
    if ( Player::CountKilledPlayers() == 0 && _livesTarget < -1 )
        m = 3;
    r = RandInt( m );
    _tz0 = &_z0;

    if ( r == 0 )
        Spawn( new Bomb( v ) );
    else if ( r == 1 )
        Spawn( new MagicShots( v ) );
    else if ( r == 2 )
        Spawn( new MagicShield( v ) );
    else {
        Spawn( new ExtraLife( v ) );
        --_livesTarget;
    }

    _tz0 = 0;
}

void Overmind::SpawnBossReward()
{
    _tz0 = &_z0;
    int r = RandInt( 4 );
    Vec2 v;

    if ( r == 0 )
        v.Set( -Lib::WIDTH / 4, Lib::HEIGHT / 2 );
    else if ( r == 1 )
        v.Set(  Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2 );
    else if ( r == 2 )
        v.Set( Lib::WIDTH / 2, -Lib::HEIGHT / 4 );
    else
        v.Set( Lib::WIDTH / 2,  Lib::HEIGHT + Lib::HEIGHT / 4 );

    Spawn( new ExtraLife( v ) );

    _tz0 = 0;
    if ( !_z0.IsBossMode() )
        SpawnPowerup();
}

void Overmind::SpawnFollow( int num, int div, int side )
{
    if ( side == 0 || side == 1 )
        Spawn( new Follow( SpawnPoint( false, _tRow, num, div ) ) );
    if ( side == 0 || side == 2 )
        Spawn( new Follow( SpawnPoint( true, _tRow, num, div ) ) );
}

void Overmind::SpawnChaser( int num, int div, int side )

{
    if ( side == 0 || side == 1 )
        Spawn( new Chaser( SpawnPoint( false, _tRow, num, div ) ) );
    if ( side == 0 || side == 2 )
        Spawn( new Chaser( SpawnPoint( true, _tRow, num, div ) ) );
}

void Overmind::SpawnSquare( int num, int div, int side )
{
    if ( side == 0 || side == 1 )
        Spawn( new Square( SpawnPoint( false, _tRow, num, div ) ) );
    if ( side == 0 || side == 2 )
        Spawn( new Square( SpawnPoint( true, _tRow, num, div ) ) );
}

void Overmind::SpawnWall( int num, int div, int side, bool dir )
{
    if ( side == 0 || side == 1 )
        Spawn( new Wall( SpawnPoint( false, _tRow, num, div ), dir ) );
    if ( side == 0 || side == 2 )
        Spawn( new Wall( SpawnPoint( true, _tRow, num, div ), dir ) );
}

void Overmind::SpawnFollowHub( int num, int div, int side )
{
    if ( side == 0 || side == 1 ) {
        bool p1 = HackRandInt( 64 ) < std::min( 32, _power - 32 ) - _hardAlready;
        if ( p1 )
            _hardAlready += 2;
        bool p2 = HackRandInt( 64 ) < std::min( 32, _power - 40 ) - _hardAlready;
        if ( p2 )
            _hardAlready += 2;
        Spawn( new FollowHub( SpawnPoint( false,  _tRow, num, div ), p1, p2 ) );
    }
    if ( side == 0 || side == 2 ) {
        bool p1 = HackRandInt( 64 ) < std::min( 32, _power - 32 ) - _hardAlready;
        if ( p1 )
            _hardAlready += 2;
        bool p2 = HackRandInt( 64 ) < std::min( 32, _power - 40 ) - _hardAlready;
        if ( p2 )
            _hardAlready += 2;
        Spawn( new FollowHub( SpawnPoint( true, _tRow, num, div ), p1, p2 ) );
    }
}

void Overmind::SpawnShielder( int num, int div, int side )
{
    if ( side == 0 || side == 1 ) {
        bool p = HackRandInt( 64 ) < std::min( 32, _power - 36 ) - _hardAlready;
        if ( p )
            _hardAlready += 3;
        Spawn( new Shielder( SpawnPoint( false,  _tRow, num, div ), p ) );
    }
    if ( side == 0 || side == 2 ) {
        bool p = HackRandInt( 64 ) < std::min( 32, _power - 36 ) - _hardAlready;
        if ( p )
            _hardAlready += 3;
        Spawn( new Shielder( SpawnPoint( true,  _tRow, num, div ), p ) );
    }
}

void Overmind::SpawnTractor( int num, int div, int side )
{
    if ( side == 0 || side == 1 ) {
        bool p = HackRandInt( 64 ) < std::min( 32, _power - 46 ) - _hardAlready;
        if ( p )
            _hardAlready += 4;
        Spawn( new Tractor( SpawnPoint( false, _tRow, num, div ), p ) );
    }
    if ( side == 0 || side == 2 ) {
        bool p = HackRandInt( 64 ) < std::min( 32, _power - 46 ) - _hardAlready;
        if ( p )
            _hardAlready += 4;
        Spawn( new Tractor( SpawnPoint( true, _tRow, num, div ), p ) );
    }
}

// Spawn formation coordinates
//------------------------------
void Overmind::Spawn( Ship* ship )
{
    if ( _tz0 )
        _tz0->AddShip( ship );
}

Vec2 Overmind::SpawnPoint( bool top, int row, int num, int div )
{
    if ( div < 2 ) div = 2;
    if ( num < 0 ) num = 0;
    if ( num >= div ) num = div - 1;
    fixed _x = fixed( top ? num : div - 1 - num ) * Lib::WIDTH / fixed( div - 1 );
    fixed _y = top ? -( row + 1 ) * ( M_PT_ZERO_ONE * 16 ) * Lib::HEIGHT : Lib::HEIGHT * ( 1 + ( row + 1 ) * ( M_PT_ZERO_ONE * 16 ) );
    return Vec2( _x, _y );
}

// Spawn logic
//------------------------------
void Overmind::Wave()
{
    if ( _z0.IsFastMode() )
        for ( int i = 0; i < RandInt( 7 ); ++i )
            RandInt( 1 );
    if ( _z0.IsWhatMode() )
        for ( int i = 0; i < RandInt( 11 ); ++i )
            RandInt( 1 );

    _tz0 = &_z0;
    int resources = GetPower();

    std::vector< std::pair< int, SpawnFormationFunction > > validFormations;
    for ( unsigned int i = 0; i < _formations.size(); i++ ) {
        if ( resources >= _formations[ i ].first.second )
            validFormations.push_back( std::pair< int, SpawnFormationFunction >( _formations[ i ].first.first, _formations[ i ].second ) );
    }

    std::vector< std::pair< int, SpawnFormationFunction > > chosenFormations;
    while ( resources > 0 ) {

        unsigned int max = 0;
        while ( max < validFormations.size() && validFormations[ max ].first <= resources )
            max++;

        int n;
        if ( max == 0 )
            break;
        else if ( max == 1 )
            n = 0;
        else
            n = RandInt( max );

        chosenFormations.insert( chosenFormations.begin() + RandInt( int( chosenFormations.size() ) + 1 ), validFormations[ n ] );
        resources -= validFormations[ n ].first;

    }
    std::vector< std::size_t > perm;
    for ( std::size_t i = 0; i < chosenFormations.size(); ++i )
        perm.push_back( i );
    for ( std::size_t i = 0; i < chosenFormations.size() - 1; ++i )
        std::swap( perm[ i ], perm[ i + RandInt( int( chosenFormations.size() - i ) ) ] );
    _hardAlready = 0;
    for ( std::size_t row = 0; row < chosenFormations.size(); ++row ) {
        chosenFormations[ perm[ row ] ].second( false, int( perm[ row ] ) );
    }
    _tz0 = 0;
}

void Overmind::Boss()
{
    _tz0 = &_z0;
    _tRow = 0;
    int cycle = ( _z0.IsHardMode() ? 1 : 0 ) + _bossModBosses / 2;
    bool secretChance = ( _z0.IsFastMode() || _z0.IsHardMode() || _z0.IsWhatMode() ) ?
                        ( _bossModFights > 1 ? RandInt( 4 ) == 0 : _bossModFights > 0 ? RandInt( 8 ) == 0 : false ) :
                        ( _bossModFights > 2 ? RandInt( 4 ) == 0 : _bossModFights > 1 ? RandInt( 6 ) == 0 : false );
    if ( _canFaceSecretBoss && _bossesToGo == 0 && _bossModSecret == 0 && secretChance ) {
        int secretCycle = std::max( 0, ( _bossModBosses + ( _z0.IsHardMode() ? 1 : 0 ) - 2 ) / 2 );
        Spawn( new SuperBoss( _z0.CountPlayers(), secretCycle ) );
        _bossModSecret = 2;
    }
    else if ( _bossModBosses % 2 == 0 ) {
        if ( _boss1Queue[ 0 ] == 0 )
            Spawn( new BigSquareBoss( _z0.CountPlayers(), cycle ) );
        if ( _boss1Queue[ 0 ] == 1 )
            Spawn( new ShieldBombBoss( _z0.CountPlayers(), cycle ) );
        if ( _boss1Queue[ 0 ] == 2 )
            Spawn( new ChaserBoss( _z0.CountPlayers(), cycle ) );

        _boss1Queue.push_back( _boss1Queue[ 0 ] );
        _boss1Queue.erase( _boss1Queue.begin() );
    }
    else {
        if ( _bossModSecret > 0 )
            --_bossModSecret;

        if ( _boss2Queue[ 0 ] == 0 )
            Spawn( new TractorBoss( _z0.CountPlayers(), cycle ) );
        if ( _boss2Queue[ 0 ] == 1 )
            Spawn( new GhostBoss( _z0.CountPlayers(), cycle ) );
        if ( _boss2Queue[ 0 ] == 2 )
            Spawn( new DeathRayBoss( _z0.CountPlayers(), cycle ) );

        _boss2Queue.push_back( _boss2Queue[ 0 ] );
        _boss2Queue.erase( _boss2Queue.begin() );
    }
    _tz0 = 0;
}

void Overmind::BossModeBoss()
{
    _tz0 = &_z0;
    _tRow = 0;
    if ( _bossModBosses < 3 ) {
        if ( _boss1Queue[ _bossModBosses ] == 0 )
            Spawn( new BigSquareBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss1Queue[ _bossModBosses ] == 1 )
            Spawn( new ShieldBombBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss1Queue[ _bossModBosses ] == 2 )
            Spawn( new ChaserBoss( _z0.CountPlayers(), 0 ) );
    }
    else {
        if ( _boss2Queue[ _bossModBosses - 3 ] == 0 )
            Spawn( new TractorBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss2Queue[ _bossModBosses - 3 ] == 1 )
            Spawn( new GhostBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss2Queue[ _bossModBosses - 3 ] == 2 )
            Spawn( new DeathRayBoss( _z0.CountPlayers(), 0 ) );
    }
    _tz0 = 0;
}

// Formations
//------------------------------
void Overmind::AddFormations()
{
    FORM_USE( Square1 );
    FORM_USE( Square2 );
    FORM_USE( Square3 );
    FORM_USE( Square1Side );
    FORM_USE( Square2Side );
    FORM_USE( Square3Side );
    FORM_USE( Wall1 );
    FORM_USE( Wall2 );
    FORM_USE( Wall3 );
    FORM_USE( Wall1Side );
    FORM_USE( Wall2Side );
    FORM_USE( Wall3Side );
    FORM_USE( Follow1 );
    FORM_USE( Follow2 );
    FORM_USE( Follow3 );
    FORM_USE( Follow1Side );
    FORM_USE( Follow2Side );
    FORM_USE( Follow3Side );
    FORM_USE( Chaser1 );
    FORM_USE( Chaser2 );
    FORM_USE( Chaser3 );
    FORM_USE( Chaser4 );
    FORM_USE( Chaser1Side );
    FORM_USE( Chaser2Side );
    FORM_USE( Chaser3Side );
    FORM_USE( Chaser4Side );
    FORM_USE( Hub1 );
    FORM_USE( Hub2 );
    FORM_USE( Hub1Side );
    FORM_USE( Hub2Side );
    FORM_USE( Mixed1 );
    FORM_USE( Mixed2 );
    FORM_USE( Mixed3 );
    FORM_USE( Mixed4 );
    FORM_USE( Mixed5 );
    FORM_USE( Mixed6 );
    FORM_USE( Mixed7 );
    FORM_USE( Mixed1Side );
    FORM_USE( Mixed2Side );
    FORM_USE( Mixed3Side );
    FORM_USE( Mixed4Side );
    FORM_USE( Mixed5Side );
    FORM_USE( Mixed6Side );
    FORM_USE( Mixed7Side );
    FORM_USE( Tractor1 );
    FORM_USE( Tractor2 );
    FORM_USE( Tractor1Side );
    FORM_USE( Tractor2Side );
    FORM_USE( Shielder1 );
    FORM_USE( Shielder1Side );
}

FORM_DEF( Square1, 4, 0 ) {
    for ( int i = 1; i < 5; i++ )
        SpawnSquare( i, 6, 0 );
} FORM_END;

FORM_DEF( Square2, 11, 0 ) {
    int r = HackRandInt( 4 );
    int p1 = 2 + HackRandInt( 8 );
    int p2 = 2 + HackRandInt( 8 );
    for ( int i = 1; i < 11; ++i ) {
        if ( r < 2 || i != p1 )
            SpawnSquare( i, 12, 1 );
        if ( r < 2 || ( r == 2 && i != 11 - p1 ) || ( r == 3 && i != p2 ) )
            SpawnSquare( i, 12, 2 );
    }
} FORM_END;

FORM_DEF( Square3, 20, 24 ) {
    int r1 = HackRandInt( 4 );
    int r2 = HackRandInt( 4 );
    int p11 = 2 + HackRandInt( 14 );
    int p12 = 2 + HackRandInt( 14 );
    int p21 = 2 + HackRandInt( 14 );
    int p22 = 2 + HackRandInt( 14 );
    for ( int i = 0; i < 18; i++ ) {
        if ( r1 < 2 || i != p11 )
            if ( r2 < 2 || i != p21 )
                SpawnSquare( i, 18, 1 );
        if ( r1 < 2 || ( r1 == 2 && i != 17 - p11 ) || ( r1 == 3 && i != p12 ) )
            if ( r2 < 2 || ( r2 == 2 && i != 17 - p21 ) || ( r2 == 3 && i != p22 ) )
                SpawnSquare( i, 18, 2 );
    }
} FORM_END;

FORM_DEF( Square1Side, 2, 0 ) {
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    if ( p < 2 ) {
        for ( int i = 1; i < 5; ++i )
            SpawnSquare( i, 6, 1 + r );
    }
    else if ( p == 2 ) {
        for ( int i = 1; i < 5; ++i )
            SpawnSquare( ( i + r ) % 2 == 0 ? i : 5 - i, 6, 1 + ( ( i + r ) % 2 ) );
    }
    else for ( int i = 1; i < 3; ++i )
        SpawnSquare( r == 0 ? i : 5 - i, 6, 0 );
} FORM_END;

FORM_DEF( Square2Side, 5, 0 ) {
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    if ( p < 2 ) {
        for ( int i = 1; i < 11; ++i )
            SpawnSquare( i, 12, 1 + r );
    }
    else if ( p == 2 ) {
        for ( int i = 1; i < 11; ++i )
            SpawnSquare( ( i + r ) % 2 == 0 ? i : 11 - i, 12, 1 + ( ( i + r ) % 2 ) );
    }
    else for ( int i = 1; i < 6; ++i )
        SpawnSquare( r == 0 ? i : 11 - i, 12, 0 );
} FORM_END;

FORM_DEF( Square3Side, 10, 12 ) {
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    int r2 = HackRandInt( 2 );
    int p2 = 1 + HackRandInt( 16 );
    if ( p < 2 ) {
        for ( int i = 0; i < 18; ++i )
            if ( r2 == 0 || i != p2 )
                SpawnSquare( i, 18, 1 + r );
    }
    else if ( p == 2 ) {
        for ( int i = 0; i < 18; ++i )
            SpawnSquare(( i + r ) % 2 == 0 ? i : 17 - i, 18, 1 + ( ( i + r ) % 2 ) );
    }
    else for ( int i = 0; i < 9; ++i )
        SpawnSquare( r == 0 ? i : 17 - i, 18, 0 );
} FORM_END;

FORM_DEF( Wall1, 5, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    for ( int i = 1; i < 3; ++i )
        SpawnWall( i, 4, 0, dir );
} FORM_END;

FORM_DEF( Wall2, 12, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = HackRandInt( 4 );
    int p1 = 2 + HackRandInt( 5 );
    int p2 = 2 + HackRandInt( 5 );
    for ( int i = 1; i < 8; ++i ) {
        if ( r < 2 || i != p1 )
            SpawnWall( i, 9, 1, dir );
        if ( r < 2 || ( r == 2 && i != 8 - p1 ) || ( r == 3 && i != p2 ) )
            SpawnWall( i, 9, 2, dir );
    }
} FORM_END;

FORM_DEF( Wall3, 22, 26 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r1 = HackRandInt( 4 );
    int r2 = HackRandInt( 4 );
    int p11 = 1 + HackRandInt( 10 );
    int p12 = 1 + HackRandInt( 10 );
    int p21 = 1 + HackRandInt( 10 );
    int p22 = 1 + HackRandInt( 10 );
    for ( int i = 0; i < 12; ++i ) {
        if ( r1 < 2 || i != p11 )
            if ( r2 < 2 || i != p21 )
                SpawnWall( i, 12, 1, dir );
        if ( r1 < 2 || ( r1 == 2 && i != 11 - p11 ) || ( r1 == 3 && i != p12 ) )
            if ( r2 < 2 || ( r2 == 2 && i != 11 - p21 ) || ( r2 == 3 && i != p22 ) )
                SpawnWall( i, 12, 2, dir );
    }
} FORM_END;

FORM_DEF( Wall1Side, 3, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    if ( p < 2 ) {
        for ( int i = 1; i < 3; ++i )
            SpawnWall( i, 4, 1 + r, dir );
    }
    else if ( p == 2 ) {
        for ( int i = 1; i < 3; ++i )
            SpawnWall( ( i + r ) % 2 == 0 ? i : 3 - i, 4, 1 + ( ( i + r ) % 2 ), dir );
    }
    else
        SpawnWall( 1 + r, 4, 0, dir );
} FORM_END;

FORM_DEF( Wall2Side, 6, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    if ( p < 2 ) {
        for ( int i = 1; i < 8; ++i )
            SpawnWall( i, 9, 1 + r, dir );
    }
    else if ( p == 2 ) {
        for ( int i = 1; i < 8; ++i )
            SpawnWall( i, 9, 1 + ( ( i + r ) % 2 ), dir );
    }
    else for ( int i = 0; i < 4; ++i )
        SpawnWall( r == 0 ? i : 8 - i, 9, 0, dir );
} FORM_END;

FORM_DEF( Wall3Side, 11, 13 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = HackRandInt( 2 );
    int p = HackRandInt( 4 );
    int r2 = HackRandInt( 2 );
    int p2 = 1 + HackRandInt( 10 );
    if ( p < 2 ) {
        for ( int i = 0; i < 12; ++i )
            if ( r2 == 0 || i != p2 )
                SpawnWall( i, 12, 1 + r, dir );
    }
    else if ( p == 2 ) {
        for ( int i = 0; i < 12; ++i )
            SpawnWall( ( i + r ) % 2 == 0 ? i : 11 - i, 12, 1 + ( ( i + r ) % 2 ), dir );
    }
    else for ( int i = 0; i < 6; ++i )
        SpawnWall( r == 0 ? i : 11 - i, 12, 0, dir );
} FORM_END;

FORM_DEF( Follow1, 3, 0 ) {
    int p = HackRandInt( 3 );
    if ( p == 0 ) {
        SpawnFollow( 0, 3, 0 );
        SpawnFollow( 2, 3, 0 );
    }
    else if ( p == 1 ) {
        SpawnFollow( 1, 4, 0 );
        SpawnFollow( 2, 4, 0 );
    }
    else {
        SpawnFollow( 0, 5, 0 );
        SpawnFollow( 4, 5, 0 );
    }
} FORM_END;

FORM_DEF( Follow2, 7, 0 ) {
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 8; i++ )
            SpawnFollow( i, 8, 0 );
    }
    else for ( int i = 0; i < 8; i++ )
        SpawnFollow( 4 + i, 16, 0 );
} FORM_END;

FORM_DEF( Follow3, 14, 0 ) {
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 16; ++i )
            SpawnFollow( i, 16, 0 );
    }
    else for ( int i = 0; i < 8; ++i ) {
        SpawnFollow( i, 28, 0 );
        SpawnFollow( 27 - i, 28, 0 );
    }
} FORM_END;

FORM_DEF( Follow1Side, 2, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 3 );
    if ( p == 0 ) {
        SpawnFollow( 0, 3, r );
        SpawnFollow( 2, 3, r );
    }
    else if ( p == 1 ) {
        SpawnFollow( 1, 4, r );
        SpawnFollow( 2, 4, r );
    }
    else {
        SpawnFollow( 0, 5, r );
        SpawnFollow( 4, 5, r );
    }
} FORM_END;

FORM_DEF( Follow2Side, 3, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 8; i++ )
            SpawnFollow( i, 8, r );
    }
    else for ( int i = 0; i < 8; i++ )
        SpawnFollow( 4 + i, 16, r );
} FORM_END;

FORM_DEF( Follow3Side, 7, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 16; ++i )
            SpawnFollow( i, 16, r );
    }
    else for ( int i = 0; i < 8; ++i ) {
        SpawnFollow( i, 28, r );
        SpawnFollow( 27 - i, 28, r );
    }
} FORM_END;

FORM_DEF( Chaser1, 4, 0 ) {
    int p = HackRandInt( 3 );
    if ( p == 0 ) {
        SpawnChaser( 0, 3, 0 );
        SpawnChaser( 2, 3, 0 );
    }
    else if ( p == 1 ) {
        SpawnChaser( 1, 4, 0 );
        SpawnChaser( 2, 4, 0 );
    }
    else {
        SpawnChaser( 0, 5, 0 );
        SpawnChaser( 4, 5, 0 );
    }
} FORM_END;

FORM_DEF( Chaser2, 8, 0 ) {
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 8; i++ )
            SpawnChaser( i, 8, 0 );
    }
    else for ( int i = 0; i < 8; i++ )
        SpawnChaser( 4 + i, 16, 0 );
} FORM_END;

FORM_DEF( Chaser3, 16, 0 ) {
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 16; ++i )
            SpawnChaser( i, 16, 0 );
    }
    else for ( int i = 0; i < 8; ++i ) {
        SpawnChaser( i, 28, 0 );
        SpawnChaser( 27 - i, 28, 0 );
    }
} FORM_END;

FORM_DEF( Chaser4, 20, 0 ) {
    for ( int i = 0; i < 22; i++ )
        SpawnChaser( i, 22, 0 );
} FORM_END;

FORM_DEF( Chaser1Side, 2, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 3 );
    if ( p == 0 ) {
        SpawnChaser( 0, 3, r );
        SpawnChaser( 2, 3, r );
    }
    else if ( p == 1 ) {
        SpawnChaser( 1, 4, r );
        SpawnChaser( 2, 4, r );
    }
    else {
        SpawnChaser( 0, 5, r );
        SpawnChaser( 4, 5, r );
    }
} FORM_END;

FORM_DEF( Chaser2Side, 4, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 2 );
    if ( p == 0 ) {
        for ( int i = 0; i < 8; i++ )
            SpawnChaser( i, 8, r );
    }
    else for ( int i = 0; i < 8; i++ )
        SpawnChaser( 4 + i, 16, r );
} FORM_END;

FORM_DEF( Chaser3Side, 8, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 2 );
    if ( p == 0 )
        for ( int i = 0; i < 16; ++i )
            SpawnChaser( i, 16, r );
    else for ( int i = 0; i < 8; ++i ) {
        SpawnChaser( i, 28, r );
        SpawnChaser( 27 - i, 28, r );
    }
} FORM_END;

FORM_DEF( Chaser4Side, 10, 0 ) {
    int r = 1 + HackRandInt( 2 );
    for ( int i = 0; i < 22; i++ )
        SpawnChaser( i, 22, r );
} FORM_END;

FORM_DEF( Hub1, 6, 0 ) {
    SpawnFollowHub( 1 + HackRandInt( 3 ), 5, 0 );
} FORM_END;

FORM_DEF( Hub2, 12, 0 ) {
    int p = HackRandInt( 3 );
    SpawnFollowHub( p == 1 ? 2 : 1, 5, 0 );
    SpawnFollowHub( p == 2 ? 2 : 3, 5, 0 );
} FORM_END;

FORM_DEF( Hub1Side, 3, 0 ) {
    SpawnFollowHub( 1 + HackRandInt( 3 ), 5, 1 + HackRandInt( 2 ) );
} FORM_END;

FORM_DEF( Hub2Side, 6, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 3 );
    SpawnFollowHub( p == 1 ? 2 : 1, 5, r );
    SpawnFollowHub( p == 2 ? 2 : 3, 5, r );
} FORM_END;

FORM_DEF( Mixed1, 6, 0 ) {
    int p = HackRandInt( 2 );
    SpawnFollow( p == 0 ? 0 : 2, 4, 0 );
    SpawnFollow( p == 0 ? 1 : 3, 4, 0 );
    SpawnChaser( p == 0 ? 2 : 0, 4, 0 );
    SpawnChaser( p == 0 ? 3 : 1, 4, 0 );
} FORM_END;

FORM_DEF( Mixed2, 12, 0 ) {
    for ( int i = 0; i < 13; i++ ) {
        if ( i % 2 )
            SpawnFollow( i, 13, 0 );
        else
            SpawnChaser( i, 13, 0 );
    }
} FORM_END;

FORM_DEF( Mixed3, 16, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    SpawnWall( 3, 7, 0, dir );
    SpawnFollowHub( 1, 7, 0 );
    SpawnFollowHub( 5, 7, 0 );
    SpawnChaser( 2, 7, 0 );
    SpawnChaser( 4, 7, 0 );
} FORM_END;

FORM_DEF( Mixed4, 18, 0 ) {
    SpawnSquare( 1, 7, 0 );
    SpawnSquare( 5, 7, 0 );
    SpawnFollowHub( 3, 7, 0 );
    SpawnChaser( 2, 9, 0 );
    SpawnChaser( 3, 9, 0 );
    SpawnChaser( 5, 9, 0 );
    SpawnChaser( 6, 9, 0 );
} FORM_END;

FORM_DEF( Mixed5, 22, 38 ) {
    SpawnFollowHub( 1, 7, 0 );
    SpawnTractor( 3, 7, 1 + HackRandInt( 2 ) );
} FORM_END;

FORM_DEF( Mixed6, 16, 30 ) {
    SpawnFollowHub( 1, 5, 0 );
    SpawnShielder( 3, 5, 0 );
} FORM_END;

FORM_DEF( Mixed7, 18, 16 ) {
    bool dir = HackRandInt( 2 ) != 0;
    SpawnShielder( 2, 5, 0 );
    SpawnWall( 1, 10, 0, dir );
    SpawnWall( 8, 10, 0, dir );
    SpawnChaser( 2, 10, 0 );
    SpawnChaser( 3, 10, 0 );
    SpawnChaser( 4, 10, 0 );
    SpawnChaser( 5, 10, 0 );
} FORM_END;

FORM_DEF( Mixed1Side, 3, 0 ) {
    int r = 1 + HackRandInt( 2 );
    int p = HackRandInt( 2 );
    SpawnFollow( p == 0 ? 0 : 2, 4, r );
    SpawnFollow( p == 0 ? 1 : 3, 4, r );
    SpawnChaser( p == 0 ? 2 : 0, 4, r );
    SpawnChaser( p == 0 ? 3 : 1, 4, r );
} FORM_END;

FORM_DEF( Mixed2Side, 6, 0 ) {
    int r = HackRandInt( 2 );
    int p = HackRandInt( 2 );
    for ( int i = 0; i < 13; i++ ) {
        if ( i % 2 )
            SpawnFollow( i, 13, 1 + r );
        else
            SpawnChaser( i, 13, p == 0 ? 1 + r : 2 - r );
    }
} FORM_END;

FORM_DEF( Mixed3Side, 8, 0 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = 1 + HackRandInt( 2 );
    SpawnWall( 3, 7, r, dir );
    SpawnFollowHub( 1, 7, r );
    SpawnFollowHub( 5, 7, r );
    SpawnChaser( 2, 7, r );
    SpawnChaser( 4, 7, r );
} FORM_END;

FORM_DEF( Mixed4Side, 9, 0 ) {
    int r = 1 + HackRandInt( 2 );
    SpawnSquare( 1, 7, r );
    SpawnSquare( 5, 7, r );
    SpawnFollowHub( 3, 7, r );
    SpawnChaser( 2, 9, r );
    SpawnChaser( 3, 9, r );
    SpawnChaser( 5, 9, r );
    SpawnChaser( 6, 9, r );
} FORM_END;

FORM_DEF( Mixed5Side, 19, 36 ) {
    int r = 1 + HackRandInt( 2 );
    SpawnFollowHub( 1, 7, r );
    SpawnTractor( 3, 7, r );
} FORM_END;

FORM_DEF( Mixed6Side, 8, 20 ) {
    int r = 1 + HackRandInt( 2 );
    SpawnFollowHub( 1, 5, r );
    SpawnShielder( 3, 5, r );
} FORM_END;

FORM_DEF( Mixed7Side, 9, 16 ) {
    bool dir = HackRandInt( 2 ) != 0;
    int r = 1 + HackRandInt( 2 );
    SpawnShielder( 2, 5, r );
    SpawnWall( 1, 10, r, dir );
    SpawnWall( 8, 10, r, dir );
    SpawnChaser( 2, 10, r );
    SpawnChaser( 3, 10, r );
    SpawnChaser( 4, 10, r );
    SpawnChaser( 5, 10, r );
} FORM_END;

FORM_DEF( Tractor1, 16, 30 ) {
    SpawnTractor( HackRandInt( 3 ) + 1, 5, 1 + HackRandInt( 2 ) );
} FORM_END;

FORM_DEF( Tractor2, 28, 46 ) {
    SpawnTractor( HackRandInt( 3 ) + 1, 5, 2 );
    SpawnTractor( HackRandInt( 3 ) + 1, 5, 1 );
} FORM_END;

FORM_DEF( Tractor1Side, 16, 36 ) {
    SpawnTractor( HackRandInt( 7 ) + 1, 9, 1 + HackRandInt( 2 ) );
} FORM_END;

FORM_DEF( Tractor2Side, 14, 32 ) {
    SpawnTractor( HackRandInt( 5 ) + 1, 7, 1 + HackRandInt( 2 ) );
} FORM_END;

FORM_DEF( Shielder1, 10, 28 ) {
    SpawnShielder( HackRandInt( 3 ) + 1, 5, 0 );
} FORM_END;

FORM_DEF( Shielder1Side, 5, 22 ) {
    SpawnShielder( HackRandInt( 3 ) + 1, 5, 1 + HackRandInt( 2 ) );
} FORM_END;
