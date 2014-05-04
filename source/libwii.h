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

    virtual void BeginFrame();
    virtual void EndFrame();

    virtual void Exit( ExitType t );
    virtual ExitType GetExitType() const;

    virtual int   RandInt( int lessThan ) const;
    virtual float RandFloat() const;

    virtual SaveData LoadSaveData();
    virtual void     SaveSaveData( const SaveData& data );

    // Input
    //------------------------------
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

    virtual void Rumble( int player, int time );
    virtual void StopRumble();
    virtual bool PlaySound( Sound sound, float volume = 1.0f, float pan = 0.0f, float repitch = 0.0f );

    virtual void TakeScreenShot();

private:

    // Save path
    //------------------------------
    #define SAVE_PATH "sd:/wiispace.sav"
    #define ENCRYPTION_KEY "WiiSPACE Encryption Key"
    #define SCREENSHOT_PATH "sd:/wiispace.png"

    // Internal
    //------------------------------
    static void        OnReset();
    static void        OnPower();
    static void        OnPadPower( s32 c );
    static ExitType    _exitType;

    static bool        HasWKey( u32 wPad, Key k );
    static bool        HasGKey( u16 gPad, Key k );

    static std::string Crypt( const std::string& text );

    void LoadSounds();
    #define USE_SOUND( sound, data ) _sounds.push_back( SoundResource( 0, NamedSound( sound , SoundBuffer( data , data##_len ) ) ) );

    // Data
    //------------------------------
    typedef ir_t              PadIR;
    typedef expansion_t       Expansion;
    std::vector< PadIR* >     _padIR;
    std::vector< Expansion* > _expansion;
    std::vector< Vec2 >       _lastSubStick;
    std::vector< int >        _rumble;

    typedef GRRLIB_texImg     Texture;
    Texture                   _consoleFont;

    typedef std::pair< u8*, int >           SoundBuffer;
    typedef std::pair< int, SoundBuffer >   NamedSound;
    typedef std::pair< int, NamedSound >    SoundResource;
    typedef std::vector< SoundResource >    SoundList;
    SoundList                               _sounds;

};

#endif
