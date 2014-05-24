#include "z0.h"
#include <iostream>
#include "lib_score.h"

LibRepScore::LibRepScore()
: _frame( 0 )
, _exitType( NO_EXIT )
{
}

LibRepScore::~LibRepScore()
{
}

void LibRepScore::OnScore( long seed, int players, bool bossMode, long score, bool hardMode, bool fastMode, bool whatMode )
{
    std::cout << seed << "\n" << players << "\n" << bossMode << "\n" << hardMode << "\n" << fastMode << "\n" << whatMode << "\n" << score << "\n" << std::flush;
    exit( 0 );
}

// General
//------------------------------
void LibRepScore::Init()
{
}

void LibRepScore::BeginFrame()
{
    if ( _frame < 5 )
        ++_frame;
    SetFrameCount( 16384 );
}

void LibRepScore::EndFrame()
{
    SetFrameCount( 16384 );
}

void LibRepScore::Exit( ExitType t )
{
    _exitType = t;
}

Lib::ExitType LibRepScore::GetExitType() const
{
    return _exitType;
}

void LibRepScore::SystemExit( bool powerOff ) const
{
}

int LibRepScore::RandInt( int lessThan )
{
    return z_rand() % lessThan;
}

fixed LibRepScore::RandFloat()
{
    return fixed( z_rand() ) / fixed( Z_RAND_MAX );
}

void LibRepScore::TakeScreenShot()
{
}

LibRepScore::Settings LibRepScore::LoadSettings() const
{
    return LibRepScore::Settings();
}

// Input
//------------------------------
LibRepScore::PadType LibRepScore::IsPadConnected( int player ) const
{
    return PAD_NONE;
}

bool LibRepScore::IsKeyPressed( int player, Key k ) const
{
    if ( player != 0 )
        return false;
    if ( k == KEY_DOWN && _frame < 4 )
        return true;
    if ( k == KEY_ACCEPT && _frame == 5 )
        return true;
    return false;
}

bool LibRepScore::IsKeyReleased( int player, Key k ) const
{
    return false;
}

bool LibRepScore::IsKeyHeld( int player, Key k ) const
{
    return false;
}

Vec2 LibRepScore::GetMoveVelocity( int player ) const
{
    return Vec2();
}

Vec2 LibRepScore::GetFireTarget( int player, const Vec2& position ) const
{
    return Vec2();
}

// Output
//------------------------------
void LibRepScore::ClearScreen() const
{
}

void LibRepScore::RenderLine( const Vec2f& a, const Vec2f& b, Colour c ) const
{
}

void LibRepScore::RenderText( const Vec2f& v, const std::string& text, Colour c ) const
{
}

void LibRepScore::RenderRect( const Vec2f& low, const Vec2f& hi, Colour c, int lineWidth ) const
{
}

void LibRepScore::Render() const
{
}

void LibRepScore::Rumble( int player, int time )
{
}

void LibRepScore::StopRumble()
{
}

bool LibRepScore::PlaySound( Sound sound, float volume, float pan, float repitch )
{
    return false;
}

// Save
//------------------------------
bool LibRepScore::Connect()
{
	return true;
}

void LibRepScore::Disconnect()
{
}

void LibRepScore::SendHighScores( const HighScoreTable& table )
{
}
