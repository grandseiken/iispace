#include "fix32.h"
#include <iomanip>
#include <sstream>

std::ostream& operator<<( std::ostream& o, const Fix32& f )
{
    if ( f._value < 0 )
        o << "-";
    uint64_t v = f._value < 0 ? -f._value : f._value;
    o << ( v >> 32 ) << ".";
    double d = 0;
    for ( int i = 0; i < 32; ++i ) {
        if ( v & ( int64_t( 1 ) << ( 31 - i ) ) )
            d += pow( 2.0, -( i + 1 ) );
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision( 16 ) << d;
    o << ss.str().substr( 2 );
    return o;
}

Fix32 Fix32::sqrt() const
{
    if ( _value <= 0 )
        return 0;
    if ( *this < 1 )
        return 1 / ( 1 / *this ).sqrt();
    const Fix32 a = *this / 2;
    static const Fix32 half = Fix32( 1 ) / 2;
    static const Fix32 bound = Fix32( 1 ) / 1024;
    Fix32 f = from_internal( _value >> ( ( 32 - clz( _value ) ) / 2 ) );
    for ( uint8_t n = 0; n < 8 && f != 0; ++n ) {
        f = f * half + a / f;
        if ( from_internal( fixedabs( ( f * f )._value - _value ) ) < bound )
            break;
    }
    return f;
}

Fix32 Fix32::sin() const
{
    bool negative = _value < 0;
    Fix32 angle = from_internal( ( _value < 0 ? -_value : _value ) % ( F32PI * 2 )._value );

	if ( angle > F32PI )
		angle -= 2 * F32PI;

	Fix32 angle2 = Fix32( angle ) * Fix32( angle );
	Fix32 out = angle;
    static const Fix32 r6          = F32ONE / 6;
    static const Fix32 r120        = F32ONE / 120;
    static const Fix32 r5040       = F32ONE / 5040;
    static const Fix32 r362880     = F32ONE / 362880;
    static const Fix32 r39916800   = F32ONE / 39916800;

	angle *= angle2;
	out -= angle * r6;
	angle *= angle2;
	out += angle * r120;
	angle *= angle2;
	out -= angle * r5040;
	angle *= angle2;
	out += angle * r362880;
	angle *= angle2;
	out -= angle * r39916800;

    return negative ? -out : out;
}

Fix32 Fix32::cos() const
{
    return ( *this + F32PI / 2 ).sin();
}

Fix32 Fix32::atan2( const Fix32& x ) const
{
    Fix32 y = from_internal( _value < 0 ? -_value : _value );
    Fix32 angle;
    static const Fix32 PId4  = F32PI / 4;
    static const Fix32 PI3d4 = 3 * F32PI / 4;

    if ( x._value >= 0 ) {
        if ( x + y == 0 )
            return -PId4;
        Fix32 r = ( x - y ) / ( x + y );
        Fix32 r3 = r * r * r;
        angle = from_internal( 0x32400000 ) * r3 - from_internal( 0xfb500000 ) * r + PId4;
    }
    else {
        if ( x - y == 0 )
            return -PI3d4;
        Fix32 r = ( x + y ) / ( y - x );
        Fix32 r3 = r * r * r;
        angle = from_internal( 0x32400000 ) * r3 - from_internal( 0xfb500000 ) * r + PI3d4;
    }
    return _value < 0 ? -angle : angle;
}
