#ifndef LIBWII_H
#define LIBWII_H

#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include "GRRLIB.h"

class LibWii : public Lib {
public:

    LibWii();
    virtual ~LibWii();

    // General
    //------------------------------
    virtual void Init();
    virtual void Exit( ExitType t );
    virtual ExitType GetExitType() const;
    virtual void SystemExit( bool powerOff ) const;

    virtual void BeginFrame();
    virtual void EndFrame();

    virtual int   RandInt( int lessThan );
    virtual fixed RandFloat();

    virtual Settings LoadSettings();
    virtual bool     Connect();
    virtual void     Disconnect();
    virtual void     SendHighScores( const HighScoreTable& table );

    virtual std::string SavePath() const
    {
        return "sd:/wiispace.sav";
    }
    virtual std::string SettingsPath() const
    {
        return "sd:/wiispace.txt";
    }
    virtual std::string ScreenshotPath() const
    {
        return "sd:/wiispace.png";
    }

    // Input
    //------------------------------
    virtual PadType IsPadConnected( int player );

    virtual bool IsKeyPressed ( int player, Key k );
    virtual bool IsKeyReleased( int player, Key k );
    virtual bool IsKeyHeld    ( int player, Key k );

    virtual Vec2 GetMoveVelocity ( int player );
    virtual Vec2 GetFireTarget   ( int player, const Vec2& position );

    // Output
    //------------------------------
	virtual void ClearScreen();
    virtual void RenderLine( const Vec2& a, const Vec2& b, Colour c );
    virtual void RenderText( const Vec2& v, const std::string& text, Colour c );
    virtual void RenderRect( const Vec2& low, const Vec2& hi, Colour c, int lineWidth = 0 );
    virtual void Render();

    virtual void Rumble( int player, int time );
    virtual void StopRumble();
    virtual bool PlaySound( Sound sound, float volume = 1.0f, float pan = 0.0f, fixed float = 0.0f );

    virtual void TakeScreenShot();

private:

    // Internal
    //------------------------------
    static void        OnReset();
    static void        OnPower();
    static void        OnPadPower( s32 c );
    static ExitType    _exitType;

    static bool        HasWKey( u32 wPad, Key k );
    static bool        HasGKey( u16 gPad, Key k );
    static bool        HasNKey( u32 nPad, Key k );
    static bool        HasCKey( u32 cPad, Key k );

    void LoadSounds();

    // Data
    //------------------------------
    typedef ir_t              PadIR;
    typedef expansion_t       Expansion;
    std::vector< PadIR* >     _padIR;
    std::vector< Expansion* > _expansion;
    std::vector< Vec2 >       _lastSubStick;
    std::vector< int >        _rumble;
    u32                       _wConnectedPads;
    u32                       _gConnectedPads;
    u32                       _cConnectedPads;

    typedef GRRLIB_texImg     Texture;
    Texture                   _consoleFont;

    Settings                  _settings;

    typedef std::pair< u8*, int >           SoundBuffer;
    typedef std::pair< int, SoundBuffer >   NamedSound;
    typedef std::pair< int, NamedSound >    SoundResource;
    typedef std::vector< SoundResource >    SoundList;
    SoundList                               _sounds;

    s32 _server;

};

#endif
