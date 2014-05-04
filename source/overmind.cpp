#include "overmind.h"
#include "enemy.h"
#include "boss.h"
#include "powerup.h"
#include "player.h"

int     Overmind::_tRow;
z0Game* Overmind::_tz0;

Overmind::Overmind( z0Game& z0 )
: _z0( z0 ),
_power( 0 ),
_timer( POWERUP_TIME ),
_count( 0 ),
_levelsMod( 0 ),
_groupsMod( 0 ),
_bossMod( 0 ),
_isBossNext( false ),
_isBossLevel( false )
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

void Overmind::Reset()
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
    _bossMod     = 0;
    _startTime   = time( 0 );
    _elapsedTime = 0;
    _timeStopped = false;

    if ( _z0.IsBossMode() ) {
        _power = 0;
        _timer = 0;
        return;
    }
    _power = INITIAL_POWER + 2 - _z0.CountPlayers() * 2;
    _timer = POWERUP_TIME;
}

void Overmind::AddFormation( SpawnFormationFunction function )
{
    _formations.push_back( Formation( function( true, 0 ), function ) );
    std::sort( _formations.begin(), _formations.end(), &SortFormations );
}

void Overmind::Update() {
    if ( _z0.IsBossMode() ) {
        if ( _count <= 0 ) {
            if ( _bossMod < 6 ) {
                if ( _bossMod )
                    SpawnBossReward();
                BossModeBoss();
            }
            if ( _bossMod < 7 ) {
                _bossMod++;
            }
        }
        return;
    }

    _timer++;
    if ( _timer == POWERUP_TIME && !_isBossLevel ) {
        SpawnPowerup();
    }

    int triggerVal = _groupsMod + _bossMod + INITIAL_TRIGGERVAL;
    if ( triggerVal < 0 || _isBossLevel || _isBossNext )
        triggerVal = 0;

    if ( ( _timer > TIMER && !_isBossLevel && !_isBossNext ) || _count <= triggerVal ) {
        if ( _timer < POWERUP_TIME && !_isBossLevel ) {
            SpawnPowerup();
        }

        _timer = 0;

        if ( _isBossLevel ) {
            _isBossLevel = false;
            _bossMod++;
            SpawnBossReward();
        }

        if ( _isBossNext ) {
            _isBossNext  = false;
            _isBossLevel = true;
            Boss();
        } else {
            Wave();
            _power++;
            _levelsMod++;
        }

        if ( _levelsMod >= LEVELS_PER_GROUP ) {
            _levelsMod = 0;
            _groupsMod++;
        }
        if ( _groupsMod >= BASE_GROUPS_PER_BOSS + _z0.CountPlayers() ) {
            _groupsMod  = 0;
            _isBossNext = true;
        }
    }
}

// Individual spawns
//------------------------------
void Overmind::SpawnPowerup()
{
    z0Game::ShipList existing = _z0.GetShipsInRadius( Vec2( Lib::WIDTH / 2.0f, Lib::HEIGHT / 2.0f ), Lib::WIDTH * Lib::HEIGHT );
    int count = 0;
    for ( unsigned int i = 0; i < existing.size(); i++ ) {
        if ( existing[ i ]->IsPowerup() )
            count++;
        if ( count >= 4 )
            break;
    }
    if ( count >= 4 )
        return;

    int r = RandInt( 4 );
    Vec2 v;

    if ( r == 0 )
        v.Set( -Lib::WIDTH,       Lib::HEIGHT / 2.0f );
    else if ( r == 1 )
        v.Set( Lib::WIDTH * 2.0f, Lib::HEIGHT / 2.0f );
    else if ( r == 2 )
        v.Set( Lib::WIDTH / 2.0f, -Lib::HEIGHT       );
    else
        v.Set( Lib::WIDTH / 2.0f, Lib::HEIGHT * 2.0f );

    int m = 4;
    for ( int i = 1; i <= Lib::PLAYERS; i++ ) {
        if ( _z0.GetLives() <= _z0.CountPlayers() - i )
            m++;
    }
    if ( !_z0.GetLives() )
        m += Player::CountKilledPlayers();
    r = RandInt( m );
    _tz0 = &_z0;

    if ( r == 0 )
        Spawn( new Bomb( v ) );
    else if ( r == 1 )
        Spawn( new MagicShots( v ) );
    else if ( r == 2 )
        Spawn( new MagicShield( v ) );
    else
        Spawn( new ExtraLife( v ) );

    _tz0 = 0;
}

void Overmind::SpawnBossReward()
{
    _tz0 = &_z0;
    int r = RandInt( 4 );
    Vec2 v;

    if ( r == 0 )
        v.Set( -Lib::WIDTH * 0.25f, Lib::HEIGHT / 2.0f );
    else if ( r == 1 )
        v.Set(  Lib::WIDTH * 1.25f, Lib::HEIGHT / 2.0f );
    else if ( r == 2 )
        v.Set( Lib::WIDTH / 2.0f, -Lib::HEIGHT * 0.25f );
    else
        v.Set( Lib::WIDTH / 2.0f,  Lib::HEIGHT * 1.25f );

    Spawn( new ExtraLife( v ) );

    _tz0 = 0;
    if ( !_z0.IsBossMode() )
        SpawnPowerup();
}

void Overmind::SpawnFollow( int num, int div )
{
    Spawn( new Follow( SpawnPoint( true, _tRow, num, div ) ) );
    Spawn( new Follow( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnChaser( int num, int div )
{
    Spawn( new Chaser( SpawnPoint( true,  _tRow, num, div ) ) );
    Spawn( new Chaser( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnSquare( int num, int div )
{
    Spawn( new Square( SpawnPoint( true,  _tRow, num, div ) ) );
    Spawn( new Square( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnWall( int num, int div )
{
    Spawn( new Wall( SpawnPoint( true,  _tRow, num, div ) ) );
    Spawn( new Wall( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnFollowHub( int num, int div )
{
    Spawn( new FollowHub( SpawnPoint( true,  _tRow, num, div ) ) );
    Spawn( new FollowHub( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnShielder( int num, int div )
{
    Spawn( new Shielder( SpawnPoint( true,  _tRow, num, div ) ) );
    Spawn( new Shielder( SpawnPoint( false, _tRow, num, div ) ) );
}

void Overmind::SpawnTractor( bool top, int num, int div )
{
    Spawn( new Tractor( SpawnPoint( top, _tRow, num, div ) ) );
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
    float _x = float( top ? num : div - 1 - num ) * Lib::WIDTH / float( div - 1 );
    float _y = top ? -( row + 1 ) * 0.16f * Lib::HEIGHT : Lib::HEIGHT * ( 1.f + ( row + 1 ) * 0.16f );
    return Vec2( _x, _y );
}

// Spawn logic
//------------------------------
void Overmind::Wave()
{
    _tz0 = &_z0;
    int resources = GetPower();

    std::vector< std::pair< int, SpawnFormationFunction > > validFormations;
    for ( unsigned int i = 0; i < _formations.size(); i++ ) {
        if ( resources >= _formations[ i ].first.second )
            validFormations.push_back( std::pair< int, SpawnFormationFunction >( _formations[ i ].first.first, _formations[ i ].second ) );
    }

    int row = 0;
    while ( resources > 0 ) {

        unsigned int max = 0;
        while ( max < validFormations.size() && validFormations[ max ].first <= resources ) {
            max++;
        }

        int n;
        if ( max == 0 )
            break;
        else if ( max == 1 )
            n = 0;
        else
            n = RandInt( max );

        validFormations[ n ].second( false, row );
        resources -= validFormations[ n ].first;
        row++;

    }
    _tz0 = 0;
}

void Overmind::Boss()
{
    _tz0 = &_z0;
    _tRow = 0;
    if ( _bossMod % 2 == 0 ) {
        if ( _boss1Queue[ 0 ] == 0 )
            Spawn( new BigSquareBoss( _z0.CountPlayers(), _bossMod / 2 ) );
        if ( _boss1Queue[ 0 ] == 1 )
            Spawn( new ShieldBombBoss( _z0.CountPlayers(), _bossMod / 2 ) );
        if ( _boss1Queue[ 0 ] == 2 )
            Spawn( new ChaserBoss( _z0.CountPlayers(), _bossMod / 2 ) );

        _boss1Queue.push_back( _boss1Queue[ 0 ] );
        _boss1Queue.erase( _boss1Queue.begin() );
    }
    else {
        if ( _boss2Queue[ 0 ] == 0 )
            Spawn( new TractorBoss( _z0.CountPlayers(), _bossMod / 2 ) );
        if ( _boss2Queue[ 0 ] == 1 )
            Spawn( new GhostBoss( _z0.CountPlayers(), _bossMod / 2 ) );
        if ( _boss2Queue[ 0 ] == 2 )
            Spawn( new DeathRayBoss( _z0.CountPlayers(), _bossMod / 2 ) );

        _boss2Queue.push_back( _boss2Queue[ 0 ] );
        _boss2Queue.erase( _boss2Queue.begin() );
    }
    _tz0 = 0;
}

void Overmind::BossModeBoss()
{
    _tz0 = &_z0;
    _tRow = 0;

    if ( _bossMod < 3 ) {
        if ( _boss1Queue[ _bossMod ] == 0 )
            Spawn( new BigSquareBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss1Queue[ _bossMod ] == 1 )
            Spawn( new ShieldBombBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss1Queue[ _bossMod ] == 2 )
            Spawn( new ChaserBoss( _z0.CountPlayers(), 0 ) );
    }
    else {
        if ( _boss2Queue[ _bossMod - 3 ] == 0 )
            Spawn( new TractorBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss2Queue[ _bossMod - 3 ] == 1 )
            Spawn( new GhostBoss( _z0.CountPlayers(), 0 ) );
        if ( _boss2Queue[ _bossMod - 3 ] == 2 )
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
    FORM_USE( Wall1 );
    FORM_USE( Wall2 );
    FORM_USE( Wall3 );
    FORM_USE( Follow1 );
    FORM_USE( Follow2 );
    FORM_USE( Follow3 );
    FORM_USE( Chaser1 );
    FORM_USE( Chaser2 );
    FORM_USE( Chaser3 );
    FORM_USE( Chaser4 );
    FORM_USE( Hub1 );
    FORM_USE( Hub2 );
    FORM_USE( Mixed1 );
    FORM_USE( Mixed2 );
    FORM_USE( Mixed3 );
    FORM_USE( Mixed4 );
    FORM_USE( Mixed5 );
    FORM_USE( Mixed6 );
    FORM_USE( Mixed7 );
    FORM_USE( Tractor1 );
    FORM_USE( Tractor2 );
    FORM_USE( Shielder1 );
}

FORM_DEF( Square1, 4, 0 ) {
    for ( int i = 1; i < 5; i++ )
        SpawnSquare( i, 6 );
} FORM_END;

FORM_DEF( Square2, 11, 0 ) {
    for ( int i = 1; i < 11; i++ )
        SpawnSquare( i, 12 );
} FORM_END;

FORM_DEF( Square3, 19, 0 ) {
    for ( int i = 0; i < 18; i++ )
        SpawnSquare( i, 18 );
} FORM_END;

FORM_DEF( Wall1, 5, 0 ) {
    for ( int i = 1; i < 3; i++ )
        SpawnWall( i, 4 );
} FORM_END;

FORM_DEF( Wall2, 12, 0 ) {
    for ( int i = 1; i < 8; i++ )
        SpawnWall( i, 9 );
} FORM_END;

FORM_DEF( Wall3, 21, 0 ) {
    for ( int i = 0; i < 12; i++ )
        SpawnWall( i, 12 );
} FORM_END;

FORM_DEF( Follow1, 3, 0 ) {
    SpawnFollow( 0, 3 );
    SpawnFollow( 2, 3 );
} FORM_END;

FORM_DEF( Follow2, 7, 0 ) {
    for ( int i = 0; i < 8; i++ )
        SpawnFollow( i, 8 );
} FORM_END;

FORM_DEF( Follow3, 14, 0 ) {
    for ( int i = 0; i < 16; i++ )
        SpawnFollow( i, 16 );
} FORM_END;

FORM_DEF( Chaser1, 4, 0 ) {
    SpawnChaser( 0, 3 );
    SpawnChaser( 2, 3 );
} FORM_END;

FORM_DEF( Chaser2, 8, 0 ) {
    for ( int i = 0; i < 8; i++ )
        SpawnChaser( i, 8 );
} FORM_END;

FORM_DEF( Chaser3, 16, 0 ) {
    for ( int i = 0; i < 16; i++ )
        SpawnChaser( i, 16 );
} FORM_END;

FORM_DEF( Chaser4, 20, 0 ) {
    for ( int i = 0; i < 22; i++ )
        SpawnChaser( i, 22 );
} FORM_END;

FORM_DEF( Hub1, 6, 0 ) {
    SpawnFollowHub( 1, 3 );
} FORM_END;

FORM_DEF( Hub2, 12, 0 ) {
    SpawnFollowHub( 1, 5 );
    SpawnFollowHub( 3, 5 );
} FORM_END;

FORM_DEF( Mixed1, 6, 0 ) {
    SpawnFollow( 0, 4 );
    SpawnFollow( 1, 4 );
    SpawnChaser( 2, 4 );
    SpawnChaser( 3, 4 );
} FORM_END;

FORM_DEF( Mixed2, 12, 0 ) {
    for ( int i = 0; i < 13; i++ ) {
        if ( i % 2 ) SpawnFollow( i, 13 );
        else SpawnChaser( i, 13 );
    }
} FORM_END;

FORM_DEF( Mixed3, 16, 0 ) {
    SpawnWall( 3, 7 );
    SpawnFollowHub( 1, 7 );
    SpawnFollowHub( 5, 7 );
    SpawnChaser( 2, 7 );
    SpawnChaser( 4, 7 );
} FORM_END;

FORM_DEF( Mixed4, 18, 0 ) {
    SpawnSquare( 1, 7 );
    SpawnSquare( 5, 7 );
    SpawnFollowHub( 3, 7 );
    SpawnChaser( 2, 9 );
    SpawnChaser( 3, 9 );
    SpawnChaser( 5, 9 );
    SpawnChaser( 6, 9 );
} FORM_END;

FORM_DEF( Mixed5, 22, 28 ) {
    SpawnFollowHub( 1, 7 );
    SpawnTractor( HackRandInt( 2 ), 3, 7 );
} FORM_END;

FORM_DEF( Mixed6, 16, 20 ) {
    SpawnFollowHub( 1, 5 );
    SpawnShielder( 3, 5 );
} FORM_END;

FORM_DEF( Mixed7, 18, 0 ) {
    SpawnShielder( 2, 5 );
    SpawnWall( 1, 10 );
    SpawnWall( 8, 10 );
    SpawnChaser( 2, 10 );
    SpawnChaser( 3, 10 );
    SpawnChaser( 4, 10 );
    SpawnChaser( 5, 10 );
} FORM_END;

FORM_DEF( Tractor1, 16, 20 ) {
    SpawnTractor( HackRandInt( 2 ), HackRandInt( 3 ) + 1, 5 );
} FORM_END;

FORM_DEF( Tractor2, 28, 36 ) {
    SpawnTractor( true,  HackRandInt( 3 ) + 1, 5 );
    SpawnTractor( false, HackRandInt( 3 ) + 1, 5 );
} FORM_END;

FORM_DEF( Shielder1, 10, 18 ) {
    SpawnShielder( HackRandInt( 3 ) + 1, 5 );
} FORM_END;
