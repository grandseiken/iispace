#include "z0.h"
#ifdef PLATFORM_WII

#include <iostream>
#include <time.h>
#include <fat.h>
#include <fstream.h>
#include <network.h>
#include "libwii.h"
#include <errno.h>

#define SERVER_ENCRYPTION_KEY "<>"
#define SERVER "www.seiken.co.uk"
#define SERVER_BUFFER_SIZE 4096

typedef s32 (* TransferType)( s32 s, void* mem, s32 len );
inline bool Transfer( s32 s, char *buf, s32 length, TransferType transferrer )
{
    s32 bytes_transferred = 0;
    s32 remaining = length;
    while ( remaining ) {
        if ( ( bytes_transferred = transferrer( s, buf, remaining > SERVER_BUFFER_SIZE ? SERVER_BUFFER_SIZE : remaining ) ) > 0 ) {
            remaining -= bytes_transferred;
            buf += bytes_transferred;
        }
        else if ( bytes_transferred < 0 ) {
            return bytes_transferred;
        }
        else {
            return false;
        }
    }
    return true;
}

std::string UrlEncode( const std::string& s )
{
    std::stringstream ss;
    for ( unsigned int i = 0; i < s.length(); i++ ) {
        int n = char( s[ i ] );
        int r = n % 16;
        int l = n / 16;
        ss << "%";
        if ( l < 10 ) ss << l;
        else if ( l == 10 ) ss << "A";
        else if ( l == 11 ) ss << "B";
        else if ( l == 12 ) ss << "C";
        else if ( l == 13 ) ss << "D";
        else if ( l == 14 ) ss << "E";
        else if ( l == 15 ) ss << "F";
        if ( r < 10 ) ss << r;
        else if ( r == 10 ) ss << "A";
        else if ( r == 11 ) ss << "B";
        else if ( r == 12 ) ss << "C";
        else if ( r == 13 ) ss << "D";
        else if ( r == 14 ) ss << "E";
        else if ( r == 15 ) ss << "F";
    }
    return ss.str();
}

// High score server
//------------------------------
bool LibWii::Connect()
{
    s32 r;
    while ( ( r = net_init() ) == -EAGAIN );
    if ( r >= 0 ) {
        char ip[16];
        if ( if_config( ip, NULL, NULL, true ) < 0 )
            return false;
    } else {
        return false;
    }

	sockaddr_in connect;
	
	_server = net_socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
    if ( _server < 0 ) return false;

    hostent* host = net_gethostbyname( ( char* )( SERVER ) );
	if( !host )
        return false;

	memset( &connect, 0, sizeof( connect ) );
	connect.sin_port = 80;
    connect.sin_family = AF_INET;
    connect.sin_addr = *( in_addr* )( host->h_addr_list[ 0 ] );
	
	if ( net_connect( _server, ( sockaddr* )( &connect ), sizeof( connect ) ) == -1 ) {
		net_close( _server );
		return false;
	}
	
	return true;
}

void LibWii::Disconnect()
{
    net_close( _server );
}

void LibWii::SendHighScores( const HighScoreTable& table )
{
    std::stringstream message;
    unsigned int i = 0;
    bool one = false;

    while ( true ) {

        int type = i / NUM_HIGH_SCORES;
        if ( type >= 4 )
            type = 4 + i - ( PLAYERS * NUM_HIGH_SCORES );
        if ( type >= 8 ) {
            break;
        }

        HighScore s = type < 4 ? table[ type ][ i % NUM_HIGH_SCORES ]
                               : table[ PLAYERS ][ type - 4 ];
        if ( s.second > 0 ) {
            if ( one )
                message << "#";
            one = true;
            message << type << " " << s.second
                    << " " << ( s.second % 17 )
                    << " " << ( s.second % 701 )
                    << " " << ( s.second % 1171 )
                    << " " << ( s.second % 1777 )
                    << " " << s.first << "#";
        }
        i++;
    }

    std::stringstream ss;
    ss << "GET http://" << SERVER << "/WiiSPACE/index.php?a=send&s=";
    ss << UrlEncode( Crypt( message.str(), SERVER_ENCRYPTION_KEY ) );
    ss << " HTTP/1.0\r\n\r\n";

	char obuf[ SERVER_BUFFER_SIZE ];
	sprintf( obuf, "%s", ss.str().c_str() );
    Transfer( _server, obuf, ss.str().length(), ( TransferType )net_write );

}

#endif
