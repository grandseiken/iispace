#ifndef LIBREPSCORE_H
#define LIBREPSCORE_H

#include "lib.h"

class LibRepScore : public Lib {
public:

    LibRepScore();
    virtual ~LibRepScore();

    virtual void OnScore( long seed, int players, bool bossMode, long score, bool hardMode, bool fastMode, bool whatMode );

    // General
    //------------------------------
    virtual void Init();
    virtual void BeginFrame();
    virtual void EndFrame();

    virtual void Exit( ExitType t );
    virtual ExitType GetExitType() const;
    virtual void SystemExit( bool powerOff ) const;
    virtual void SetWorkingDirectory( bool original ) { }

    virtual int   RandInt( int lessThan );
    virtual fixed RandFloat();

    virtual Settings LoadSettings() const;
    virtual bool     Connect();
    virtual void     Disconnect();
    virtual void     SendHighScores( const HighScoreTable& table );

    virtual void     SetVolume( int volume ) { }

    virtual std::string SavePath() const
    {
        return "";
    }
    virtual std::string SettingsPath() const
    {
        return "";
    }
    virtual std::string ScreenshotPath() const
    {
        return "";
    }

    // Input
    //------------------------------
    virtual PadType IsPadConnected( int player ) const;

    virtual bool IsKeyPressed ( int player, Key k ) const;
    virtual bool IsKeyReleased( int player, Key k ) const;
    virtual bool IsKeyHeld    ( int player, Key k ) const;

    virtual Vec2 GetMoveVelocity ( int player ) const;
    virtual Vec2 GetFireTarget   ( int player, const Vec2& position ) const;

    // Output
    //------------------------------
	virtual void ClearScreen() const;
    virtual void RenderLine( const Vec2f& a, const Vec2f& b, Colour c ) const;
    virtual void RenderText( const Vec2f& v, const std::string& text, Colour c ) const;
    virtual void RenderRect( const Vec2f& low, const Vec2f& hi, Colour c, int lineWidth = 0 ) const;
    virtual void Render() const;

    virtual void Rumble( int player, int time );
    virtual void StopRumble();
    virtual bool PlaySound( Sound sound, float volume = 0.f, float pan = 0.f, float repitch = 0.f );

    virtual void TakeScreenShot();

private:

    int      _frame;
    ExitType _exitType;

};

#endif
