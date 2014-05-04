#ifndef SHAPE_H
#define SHAPE_H

#include "z0.h"
#include "lib.h"

// Basic shape
//------------------------------
class Shape {
public:

    Shape( const Vec2& centre, Colour colour, int category )
    : _centre( centre ), _colour( colour ), _category( category ) { }
    virtual ~Shape() { }

    Vec2 GetCentre() const
    { return _centre; }
    void SetCentre( const Vec2& centre )
    { _centre = centre; }

    int GetCategory() const
    { return _category; }
    void SetCategory( int category )
    { _category = category; }

    Colour GetColour( Colour colour = 0 ) const
    { return colour ? colour : _colour; }
    void SetColour( Colour colour )
    { _colour = colour; }

    bool CheckPoint( const Vec2& v ) const
    { return CheckLocalPoint( v - GetCentre() ); }

    virtual Vec2 ConvertPoint( const Vec2& position, float rotation, const Vec2& v ) const
    { Vec2 a = GetCentre() + v; a.Rotate( rotation ); return position + a; }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const = 0;

    virtual float GetRotation() const { return 0; }
    virtual void SetRotation( float rotation ) { }
    void Rotate( float rotation )
    { SetRotation( GetRotation() + rotation ); }

private:

    virtual bool CheckLocalPoint( const Vec2& v ) const = 0;

    Vec2 _centre;
    Colour _colour;
    int _category;

};

// Shape with independent rotation
//------------------------------
class RotateShape : public Shape {
public:

    RotateShape( const Vec2& centre, float rotation, Colour colour, int category )
    : Shape( centre, colour, category ), _rotation( rotation ) { }
    virtual ~RotateShape() { }

    virtual float GetRotation() const
    { return _rotation; }
    virtual void SetRotation( float rotation )
    { _rotation = ( ( rotation > 2.0f * M_PI ) ? ( rotation - 2.0f * M_PI ) : ( rotation < 0 ? rotation + 2.0f * M_PI : rotation ) ); }

    virtual Vec2 ConvertPoint( const Vec2& position, float rotation, const Vec2& v ) const
    { Vec2 a = v; a.Rotate( GetRotation() ); return Shape::ConvertPoint( position, rotation, a ); }

private:

    virtual bool CheckLocalPoint( const Vec2& v ) const
    { Vec2 a = v; a.Rotate( -GetRotation() ); return CheckRotatedPoint( a ); }
    virtual bool CheckRotatedPoint( const Vec2& v ) const = 0;

    float _rotation;

};

// Filled rectangle
//------------------------------
class Fill : public Shape {
public:

    Fill( const Vec2& centre, float width, float height, Colour colour, int category = 0 )
    : Shape( centre, colour, category ), _width( width ), _height( height ) { }
    virtual ~Fill() { }

    float GetWidth() const
    { return _width; }
    float GetHeight() const
    { return _height; }
    void SetWidth( float width )
    { _width = width; }
    void SetHeight( float height )
    { _height = height; }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        Vec2 c = ConvertPoint( position, rotation, Vec2() );
        Vec2 a = c + Vec2( GetWidth(), GetHeight() );
        Vec2 b = c - Vec2( GetWidth(), GetHeight() );
        lib.RenderRect( a, b, GetColour( colour ) );
    }

private:

    virtual bool CheckLocalPoint( const Vec2& v ) const
    {
        return abs( v._x ) < _width && abs( v._y ) < _height;
    }

    float _width;
    float _height;

};

// Line between two points
//------------------------------
class Line : public RotateShape {
public:

    Line( const Vec2& centre, const Vec2& a, const Vec2& b, Colour colour, float rotation = 0 )
    : RotateShape( centre, rotation, colour, 0 ), _a( a ), _b( b ) { }
    virtual ~Line() { }

    Vec2 GetA() const
    { return _a; }
    Vec2 GetB() const
    { return _b; }
    void SetA( const Vec2& a )
    { _a = a; }
    void SetB( const Vec2& b )
    { _b = b; }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        Vec2 a = ConvertPoint( position, rotation, _a );
        Vec2 b = ConvertPoint( position, rotation, _b );
        lib.RenderLine( a, b, GetColour( colour ) );
    }

private:

    virtual bool CheckRotatedPoint( const Vec2& v ) const
    { return false; }

    Vec2 _a;
    Vec2 _b;

};

// Box with width and height
//------------------------------
class Box : public RotateShape {
public:

    Box( const Vec2& centre, float width, float height, Colour colour, float rotation = 0, int category = 0 )
    : RotateShape( centre, rotation, colour, category ), _width( width ), _height( height ) { }
    virtual ~Box() { }

    float GetWidth() const
    { return _width; }
    float GetHeight() const
    { return _height; }
    void SetWidth( float width )
    { _width = width; }
    void SetHeight( float height )
    { _height = height; }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        Vec2 a = ConvertPoint( position, rotation, Vec2(  GetWidth(),  GetHeight() ) );
        Vec2 b = ConvertPoint( position, rotation, Vec2( -GetWidth(),  GetHeight() ) );
        Vec2 c = ConvertPoint( position, rotation, Vec2( -GetWidth(), -GetHeight() ) );
        Vec2 d = ConvertPoint( position, rotation, Vec2(  GetWidth(), -GetHeight() ) );
        lib.RenderLine( a, b, GetColour( colour ) );
        lib.RenderLine( b, c, GetColour( colour ) );
        lib.RenderLine( c, d, GetColour( colour ) );
        lib.RenderLine( d, a, GetColour( colour ) );
    }

private:

    virtual bool CheckRotatedPoint( const Vec2& v ) const
    {
        return abs( v._x ) < _width && abs( v._y ) < _height;
    }

    float _width;
    float _height;

};

// Outlined regular polygon
//------------------------------
class Polygon : public RotateShape {
public:
    
    Polygon( const Vec2& centre, float radius, int sides, Colour colour, float rotation = 0, int category = 0 )
    : RotateShape( centre, rotation, colour, category ), _radius( radius ), _sides( sides ) { }
    virtual ~Polygon() { }

    float GetRadius() const
    { return _radius; }
    void SetRadius( float radius )
    { _radius = radius; }

    int GetSides() const
    { return _sides; }
    void SetSides( int sides )
    { _sides = sides; }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        if ( GetSides() < 2 ) return;
        for ( int i = 0; i < GetSides(); i++ ) {
            Vec2 a, b;
            a.SetPolar(     float( i ) * 2.0f * M_PI / float( GetSides() ), GetRadius() );
            b.SetPolar( float( i + 1 ) * 2.0f * M_PI / float( GetSides() ), GetRadius() );
            lib.RenderLine( ConvertPoint( position, rotation, a ), ConvertPoint( position, rotation, b ), GetColour( colour ) );
        }
    }

private:

    virtual bool CheckRotatedPoint( const Vec2& v ) const
    {
        return v.Length() < GetRadius();
    }

    float _radius;
    int _sides;

};

// Complete regular polygon
//------------------------------
class Polygram : public Polygon {
public:

    Polygram( const Vec2& centre, float radius, int sides, Colour colour, float rotation = 0, int category = 0 )
    : Polygon( centre, radius, sides, colour, rotation, category ) { }
    virtual ~Polygram() { }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        if ( GetSides() < 2 ) return;
        std::vector< Vec2 > list;
        for ( int i = 0; i < GetSides(); i++ ) {
            Vec2 v;
            v.SetPolar(     float( i ) * 2.0f * M_PI / float( GetSides() ), GetRadius() );
            list.push_back( v );
        }
        for ( unsigned int i = 0; i < list.size(); i++ ) {
            for ( unsigned int j = i + 1; j < list.size(); j++ ) {
                lib.RenderLine( ConvertPoint( position, rotation, list[ i ] ), ConvertPoint( position, rotation, list[ j ] ), GetColour( colour ) );
            }
        }
    }

};

// Spoked regular polygon
//------------------------------
class Polystar : public Polygon {
public:

    Polystar( const Vec2& centre, float radius, int sides, Colour colour, float rotation = 0, int category = 0 )
    : Polygon( centre, radius, sides, colour, rotation, category ) { }
    virtual ~Polystar() { }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        if ( GetSides() < 2 ) return;
        for ( int i = 0; i < GetSides(); i++ ) {
            Vec2 v;
            v.SetPolar(     float( i ) * 2.0f * M_PI / float( GetSides() ), GetRadius() );
            lib.RenderLine( ConvertPoint( position, rotation, Vec2() ), ConvertPoint( position, rotation, v ), GetColour( colour ) );
        }
    }

};

// Group of lesser shapes
//------------------------------
class CompoundShape : public RotateShape {
public:

    // Child shapes take the top-level category
    CompoundShape( const Vec2& centre, float rotation = 0, int category = 0 )
    : RotateShape( centre, rotation, 0, category ) { }
    virtual ~CompoundShape()
    {
        for ( unsigned int i = 0; i < _children.size(); i++ )
            delete _children[ i ];
    }

    void AddShape( Shape* shape )
    {
        _children.push_back( shape );
    }

    virtual void Render( Lib& lib, const Vec2& position, float rotation, Colour colour = 0 ) const
    {
        Vec2 c = ConvertPoint( position, rotation, Vec2() );
        for ( unsigned int i = 0; i < _children.size(); i++ )
            _children[ i ]->Render( lib, c, GetRotation() + rotation, colour );
    }

    int CountShapes() const
    { return _children.size(); }
    Shape& GetShape( int i ) const
    { return *_children[ i ]; }

private:

    virtual bool CheckRotatedPoint( const Vec2& v ) const
    {
        for ( unsigned int i = 0; i < _children.size(); i++ )
            if ( _children[ i ]->CheckPoint( v ) )
                return true;
        return false;
    }

    typedef std::vector< Shape* > ShapeList;
    ShapeList _children;

};


#endif
