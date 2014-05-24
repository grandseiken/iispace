#ifndef IISPACE_SRC_GAMEPAD_GAMEPAD_H
#define IISPACE_SRC_GAMEPAD_GAMEPAD_H

#include <fstream>
#include <sstream>
#include <vector>

struct PadConfig {
    struct Stick {
        int _axis1;
        int _axis2;
        bool _axis1r;
        bool _axis2r;
    };
    typedef std::vector< Stick > Sticks;
    typedef std::vector< int >   Buttons;

    std::string _name;

    Sticks  _moveSticks;
    Buttons _moveDpads;
    Sticks  _aimSticks;
    Buttons _aimDpads;
    Buttons _startButtons;
    Buttons _fireButtons;
    Buttons _bombButtons;
    Buttons _moveUpButtons;
    Buttons _moveDownButtons;
    Buttons _moveLeftButtons;
    Buttons _moveRightButtons;

    void SetDefault()
    {
        _moveSticks.clear();
        _aimSticks.clear();
        _moveDpads.clear();
        _aimDpads.clear();
        _startButtons.clear();
        _fireButtons.clear();
        _moveUpButtons.clear();
        _moveDownButtons.clear();
        _moveLeftButtons.clear();
        _moveRightButtons.clear();
        Stick stick;
        stick._axis1 = 2;
        stick._axis1r = false;
        stick._axis2 = 3;
        stick._axis2r = false;
        _moveSticks.push_back( stick );
        stick._axis1 = 0;
        stick._axis2 = 1;
        _aimSticks.push_back( stick );
        _moveDpads.push_back( 0 );
        _aimDpads.push_back( 1 );
        _startButtons.push_back( 9 );
        _startButtons.push_back( 9 );
        _fireButtons.push_back( 1 );
        _fireButtons.push_back( 2 );
        _fireButtons.push_back( 5 );
        _bombButtons.push_back( 0 );
        _bombButtons.push_back( 3 );
        _bombButtons.push_back( 4 );
        _bombButtons.push_back( 6 );
        _bombButtons.push_back( 7 );
    }

    void Write( std::ofstream& write )
    {
        for ( std::size_t t = 0; t < _name.length(); ++t ) {
            if ( _name[ t ] == '\n' || _name[ t ] == '\r' )
                _name[ t ] = ' ';
        }
        write << _name << "\n";
        for ( std::size_t i = 0; i < _moveSticks.size(); ++i )
            write << _moveSticks[ i ]._axis1 << " " << _moveSticks[ i ]._axis1r << " " << _moveSticks[ i ]._axis2 << " " << _moveSticks[ i ]._axis2r << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _moveDpads.size(); ++i )
            write << _moveDpads[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _aimSticks.size(); ++i )
            write << _aimSticks[ i ]._axis1 << " " << _aimSticks[ i ]._axis1r << " " << _aimSticks[ i ]._axis2 << " " << _aimSticks[ i ]._axis2r << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _aimDpads.size(); ++i )
            write << _aimDpads[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _startButtons.size(); ++i )
            write << _startButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _fireButtons.size(); ++i )
            write << _fireButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _bombButtons.size(); ++i )
            write << _bombButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _moveUpButtons.size(); ++i )
            write << _moveUpButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _moveDownButtons.size(); ++i )
            write << _moveDownButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _moveLeftButtons.size(); ++i )
            write << _moveLeftButtons[ i ] << " ";
        write << "\n";
        for ( std::size_t i = 0; i < _moveRightButtons.size(); ++i )
            write << _moveRightButtons[ i ] << " ";
        write << "\n";
    }

    void Read( std::ifstream& read )
    {
        _moveSticks.clear();
        _aimSticks.clear();
        _moveDpads.clear();
        _aimDpads.clear();
        _startButtons.clear();
        _fireButtons.clear();
        _moveUpButtons.clear();
        _moveDownButtons.clear();
        _moveLeftButtons.clear();
        _moveRightButtons.clear();
        std::string line;
        int a = 0;
        int b = 0;
        int c = 0;
        int d = 0;
        std::getline( read, line );
        std::stringstream ss1( line );
        while ( ss1 >> a ) {
            ss1 >> c;
            ss1 >> b;
            ss1 >> d;
            Stick stick;
            stick._axis1 = a;
            stick._axis1r = c != 0;
            stick._axis2 = b;
            stick._axis2r = d != 0;
            _moveSticks.push_back( stick );
        }
        std::getline( read, line );
        std::stringstream ss2( line );
        while ( ss2 >> a )
            _moveDpads.push_back( a );
        std::getline( read, line );
        std::stringstream ss3( line );
        while ( ss3 >> a ) {
            ss3 >> c;
            ss3 >> b;
            ss3 >> d;
            Stick stick;
            stick._axis1 = a;
            stick._axis1r = c != 0;
            stick._axis2 = b;
            stick._axis2r = d != 0;
            _aimSticks.push_back( stick );
        }
        std::getline( read, line );
        std::stringstream ss4( line );
        while ( ss4 >> a )
            _aimDpads.push_back( a );
        std::getline( read, line );
        std::stringstream ss5( line );
        while ( ss5 >> a )
            _startButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss6( line );
        while ( ss6 >> a )
            _fireButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss7( line );
        while ( ss7 >> a )
            _bombButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss8( line );
        while ( ss8 >> a )
            _moveUpButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss9( line );
        while ( ss9 >> a )
            _moveDownButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss10( line );
        while ( ss10 >> a )
            _moveLeftButtons.push_back( a );
        std::getline( read, line );
        std::stringstream ss11( line );
        while ( ss11 >> a )
            _moveRightButtons.push_back( a );
    }
};

#endif