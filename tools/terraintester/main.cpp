//---------------------------------------------------------------------------
// terraintester: render Enemy Nations terrain from a hand-authored map, with the
// same enterrain library the game uses (tile set, tile rules, projection, worldgen
// shore + road facing rules, the live water layer, water/shore blend bands, feather,
// shade). Does not start the game.
//
//   terraintester [--data DIR] [--dir N] [--zoom N] [--shot OUT.png] [--dump]
//                 [--raw-shore] [--outline] MAP.txt
//
// DIR: data/terrain_gpu, or a game data dir that contains terrain_gpu/ (also env
// EN_TERRAIN_DATA). Default: the repo's enations_latest/data/terrain_gpu.
// --shot renders one frame, writes the PNG and exits; --dump prints the derived map
// (types + indices after the shore pass / road facings) and exits.
//
// Keys: arrows / left-drag pan; wheel or +/- zoom; , . rotate; R reload map;
// S save PNG; F feather; H shade; A animate water; M shore fix on/off (starts off
// with --raw-shore or a "shore manual" map); O outline the hexes the shore fix redraws (--outline); Esc quit.
//---------------------------------------------------------------------------
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "EnTerrain.h"
#include "EnTerrainShore.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#ifndef ENTERRAIN_DEFAULT_DATA
#define ENTERRAIN_DEFAULT_DATA "data/terrain_gpu"
#endif

using namespace enterrain;

//---------------------------------------------------------------------------
// Map file
//---------------------------------------------------------------------------
// Text format (examples: tools/terraintester/maps/*.txt):
//   size W H
//   shore auto|manual (optional, default auto) auto = run the game's worldgen shore pass
//                    (enterrain::AddCoastlines: water edges -> coastline + facings), so
//                    a map only needs land vs. water; manual = use the map's types as-is
//   makelakes        (optional) relabel lake shores to the lake art group, as worldgen's
//                    MakeLakes does for small enclosed oceans (river-fed lakes keep ocean art)
//   types            then H rows of W cells: <code>[index][/stage]
//   alts             (optional) then H rows of W ints (raw altitude; sea level 16)
// codes: C city  d desert  f forest  l lake  h hill  m mountain  o ocean  p plain
//        v river r road  u rough  s swamp  c coastline  F fields  R resources
// Road facings are computed from road neighbours (CHex::ChangeToRoad's rule). An index
// written on a road or coastline cell (r10, c12) overrides the computed one.
struct TestMap : public HexGrid, public ShoreGrid
{
    int w = 0, h = 0;
    bool autoShore = true, makeLakes = false;
    std::vector<int> type, index, stage, alt;
    std::vector<unsigned char> explicitIdx, mark;

    int Width() const override { return w; }
    int Height() const override { return h; }
    int Type( int x, int y ) const override { return type[y * w + x]; }
    int Index( int x, int y ) const override { return index[y * w + x]; }
    int GrowStage( int x, int y ) const override { return stage[y * w + x]; }
    int Alt( int x, int y ) const override { return alt[y * w + x]; }

    // ShoreGrid (worldgen shore rules). SetType is a plain store: the game's CHex::SetType
    // may also re-derive a LAND type from slope (reachable here only via the dry-coast ->
    // plain cleanup); the tester keeps plain.
    void SetType( int x, int y, int t ) override { int k = y * w + x; if ( type[k] != t ) { type[k] = t; index[k] = 0; } }
    void SetAlt( int x, int y, int a ) override { alt[y * w + x] = a; }
    bool Mark( int x, int y ) const override { return mark[y * w + x] != 0; }
    void SetMark( int x, int y ) override { mark[y * w + x] = 1; }
    void ClearMark( int x, int y ) override { mark[y * w + x] = 0; }
    int  CoastIndex( int x, int y ) const override { int k = y * w + x; return type[k] == TerrCoastline ? index[k] : -1; }
    bool SetCoastIndex( int x, int y, int i ) override
    {
        // theTerrain.GetSprite( coastline, i ) is non-strict: an index past the 39 coastline
        // sprites (0..38) falls back to offset 0. Mirror that, so out-of-range data shows
        // here exactly as in the game.
        if ( i < 0 || i > 38 ) i = 0;
        int k = y * w + x;
        bool changed = !( type[k] == TerrCoastline && index[k] == i );
        index[k] = i;
        return changed;
    }
};

// Derived terrain, as the game builds it: shore pass, optional MakeLakes relabel, road
// facings; then explicit indices from the file win.
static void DeriveTerrain( TestMap& m )
{
    std::vector<int> fileIdx = m.index, fileType = m.type;
    if ( m.autoShore )
        AddCoastlines( m );
    if ( m.makeLakes )
        for ( int y = 0; y < m.h; ++y )
            for ( int x = 0; x < m.w; ++x )
                if ( m.type[y * m.w + x] == TerrLake )
                    RelabelLakeCoasts( m, x, y );
    auto isRoad = [&]( int x, int y ) {
        x = ( ( x % m.w ) + m.w ) % m.w; y = ( ( y % m.h ) + m.h ) % m.h;
        return m.type[y * m.w + x] == TerrRoad;
    };
    for ( int y = 0; y < m.h; ++y )
        for ( int x = 0; x < m.w; ++x )
            if ( m.type[y * m.w + x] == TerrRoad )
                m.index[y * m.w + x] = RoadFacing( ( isRoad( x, y - 1 ) ? 1 : 0 ) | ( isRoad( x - 1, y ) ? 2 : 0 ) |
                                                   ( isRoad( x + 1, y ) ? 4 : 0 ) | ( isRoad( x, y + 1 ) ? 8 : 0 ) );
    for ( size_t k = 0; k < fileIdx.size(); ++k )
        // only where the FILE wrote a road/coastline: a land cell's variant digit must not
        // become the facing of a hex the shore pass turned into coastline
        if ( m.explicitIdx[k] && m.type[k] == fileType[k] && ( m.type[k] == TerrRoad || m.type[k] == TerrCoastline ) )
            m.index[k] = fileIdx[k];
}

static int CodeToType( char c )
{
    switch ( c )
    {
    case 'C': return TerrCity;     case 'd': return TerrDesert;   case 'f': return TerrForest;
    case 'l': return TerrLake;     case 'h': return TerrHill;     case 'm': return TerrMountain;
    case 'o': return TerrOcean;    case 'p': return TerrPlain;    case 'v': return TerrRiver;
    case 'r': return TerrRoad;     case 'u': return TerrRough;    case 's': return TerrSwamp;
    case 'c': return TerrCoastline;case 'F': return TerrFields;   case 'R': return TerrResources;
    default:  return -1;
    }
}

static int DefaultAlt( int type )
{
    switch ( type )
    {
    case TerrOcean: return 8;
    case TerrLake: case TerrRiver: case TerrSwamp: case TerrCoastline: return SeaLevel;
    case TerrHill: return 28;
    case TerrMountain: return 40;
    default: return 20;
    }
}

static bool LoadMap( const std::string& path, TestMap& m, std::string& err )
{
    std::ifstream f( path.c_str() );
    if ( !f ) { err = "cannot open " + path; return false; }
    TestMap out;
    std::string line, section;
    int row = 0;
    bool haveAlts = false;
    int lineNo = 0;
    while ( std::getline( f, line ) )
    {
        ++lineNo;
        size_t hash = line.find( '#' );
        if ( hash != std::string::npos ) line.erase( hash );
        std::istringstream ss( line );
        std::string tok;
        if ( !( ss >> tok ) ) continue;
        if ( tok == "size" )
        {
            ss >> out.w >> out.h;
            if ( out.w <= 0 || out.h <= 0 || out.w > 1024 || out.h > 1024 ) { err = "bad size"; return false; }
            size_t n = (size_t)out.w * out.h;
            out.type.assign( n, TerrPlain ); out.index.assign( n, 0 );
            out.stage.assign( n, 0 );        out.alt.assign( n, -1 );
            out.explicitIdx.assign( n, 0 );  out.mark.assign( n, 0 );
            continue;
        }
        if ( tok == "shore" ) { std::string v; ss >> v; out.autoShore = ( v != "manual" ); continue; }
        if ( tok == "makelakes" ) { out.makeLakes = true; continue; }
        if ( tok == "types" || tok == "alts" )
        {
            if ( !out.w ) { err = "'size' must come first"; return false; }
            section = tok; row = 0;
            haveAlts = haveAlts || tok == "alts";
            continue;
        }
        if ( section.empty() ) { err = "line " + std::to_string( lineNo ) + ": expected size/types/alts"; return false; }
        if ( row >= out.h ) { err = "line " + std::to_string( lineNo ) + ": too many rows in " + section; return false; }
        for ( int x = 0; x < out.w; ++x )
        {
            if ( x > 0 && !( ss >> tok ) )
            { err = "line " + std::to_string( lineNo ) + ": row has fewer than " + std::to_string( out.w ) + " cells"; return false; }
            size_t k = (size_t)row * out.w + x;
            if ( section == "alts" ) { out.alt[k] = atoi( tok.c_str() ); continue; }
            int t = CodeToType( tok[0] );
            if ( t < 0 ) { err = "line " + std::to_string( lineNo ) + ": unknown terrain code '" + tok + "'"; return false; }
            out.type[k]  = t;
            out.explicitIdx[k] = ( tok.size() > 1 && isdigit( (unsigned char)tok[1] ) ) ? 1 : 0;
            out.index[k] = out.explicitIdx[k] ? atoi( tok.c_str() + 1 ) : 0;
            size_t sl = tok.find( '/' );
            out.stage[k] = ( sl != std::string::npos ) ? atoi( tok.c_str() + sl + 1 ) : 0;
        }
        ++row;
    }
    if ( !out.w ) { err = "no 'size' line"; return false; }
    for ( size_t k = 0; k < out.alt.size(); ++k )
        if ( out.alt[k] < 0 ) out.alt[k] = DefaultAlt( out.type[k] );
    (void)haveAlts;
    DeriveTerrain( out );
    m = out;
    return true;
}

//---------------------------------------------------------------------------
// Minimal PNG writer (stored deflate blocks; no zlib dependency)
//---------------------------------------------------------------------------
static unsigned Crc32( const unsigned char* p, size_t n, unsigned c = 0xFFFFFFFFu )
{
    for ( size_t i = 0; i < n; ++i )
    {
        c ^= p[i];
        for ( int k = 0; k < 8; ++k ) c = ( c >> 1 ) ^ ( 0xEDB88320u & ( 0u - ( c & 1u ) ) );
    }
    return c;
}

static void Put32( std::vector<unsigned char>& v, unsigned x )
{
    v.push_back( (unsigned char)( x >> 24 ) ); v.push_back( (unsigned char)( x >> 16 ) );
    v.push_back( (unsigned char)( x >> 8 ) );  v.push_back( (unsigned char)x );
}

static void Chunk( std::vector<unsigned char>& out, const char* type, const std::vector<unsigned char>& data )
{
    Put32( out, (unsigned)data.size() );
    size_t start = out.size();
    out.insert( out.end(), type, type + 4 );
    out.insert( out.end(), data.begin(), data.end() );
    Put32( out, Crc32( &out[start], out.size() - start ) ^ 0xFFFFFFFFu );
}

// rgba: w*h*4 bytes, R,G,B,A order.
static bool WritePng( const std::string& path, const unsigned char* rgba, int w, int h )
{
    std::vector<unsigned char> raw;
    raw.reserve( (size_t)( w * 4 + 1 ) * h );
    for ( int y = 0; y < h; ++y )
    {
        raw.push_back( 0 );   // filter: none
        raw.insert( raw.end(), rgba + (size_t)y * w * 4, rgba + (size_t)( y + 1 ) * w * 4 );
    }
    std::vector<unsigned char> z;
    z.push_back( 0x78 ); z.push_back( 0x01 );
    unsigned a = 1, b = 0;
    for ( size_t i = 0; i < raw.size(); ++i ) { a = ( a + raw[i] ) % 65521u; b = ( b + a ) % 65521u; }
    for ( size_t off = 0; off < raw.size() || off == 0; )
    {
        size_t n = raw.size() - off; if ( n > 65535 ) n = 65535;
        bool last = off + n >= raw.size();
        z.push_back( last ? 1 : 0 );
        z.push_back( (unsigned char)( n & 0xFF ) ); z.push_back( (unsigned char)( n >> 8 ) );
        z.push_back( (unsigned char)( ~n & 0xFF ) ); z.push_back( (unsigned char)( ( ~n >> 8 ) & 0xFF ) );
        z.insert( z.end(), raw.begin() + off, raw.begin() + off + n );
        off += n;
        if ( last ) break;
    }
    Put32( z, ( b << 16 ) | a );

    std::vector<unsigned char> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    std::vector<unsigned char> ihdr;
    Put32( ihdr, (unsigned)w ); Put32( ihdr, (unsigned)h );
    ihdr.push_back( 8 ); ihdr.push_back( 6 ); ihdr.push_back( 0 ); ihdr.push_back( 0 ); ihdr.push_back( 0 );
    Chunk( png, "IHDR", ihdr );
    Chunk( png, "IDAT", z );
    Chunk( png, "IEND", std::vector<unsigned char>() );
    FILE* f = fopen( path.c_str(), "wb" );
    if ( !f ) return false;
    bool ok = fwrite( png.data(), 1, png.size(), f ) == png.size();
    fclose( f );
    return ok;
}

static bool SaveShot( SDL_Renderer* r, const std::string& path )
{
    int w = 0, h = 0;
    SDL_GetRendererOutputSize( r, &w, &h );
    std::vector<unsigned char> px( (size_t)w * h * 4 );
    if ( SDL_RenderReadPixels( r, nullptr, SDL_PIXELFORMAT_ABGR8888, px.data(), w * 4 ) != 0 )
    {
        fprintf( stderr, "ReadPixels failed: %s\n", SDL_GetError() );
        return false;
    }
    for ( size_t i = 3; i < px.size(); i += 4 ) px[i] = 255;   // opaque
    bool ok = WritePng( path, px.data(), w, h );
    printf( ok ? "saved %s (%dx%d)\n" : "FAILED to write %s\n", path.c_str(), w, h );
    return ok;
}

//---------------------------------------------------------------------------
// Data dir
//---------------------------------------------------------------------------
static bool IsFile( const std::string& p )
{
    FILE* f = fopen( p.c_str(), "rb" );
    if ( f ) fclose( f );
    return f != nullptr;
}

static std::string ResolveDataDir( std::string d )
{
    const char* probe = "/plain_0_aa010000_z0.png";
    if ( d.empty() ) { const char* e = getenv( "EN_TERRAIN_DATA" ); if ( e ) d = e; }
    std::vector<std::string> cands;
    if ( !d.empty() ) { cands.push_back( d ); cands.push_back( d + "/terrain_gpu" ); cands.push_back( d + "/data/terrain_gpu" ); }
    else
    {
        cands.push_back( "data/terrain_gpu" );
        char* base = SDL_GetBasePath();
        if ( base ) { cands.push_back( std::string( base ) + "data/terrain_gpu" ); SDL_free( base ); }
        cands.push_back( ENTERRAIN_DEFAULT_DATA );
    }
    for ( const std::string& c : cands )
        if ( IsFile( c + probe ) ) return c;
    return std::string();
}

//---------------------------------------------------------------------------
// Camera: centre in world map units (doubles), so rotate/zoom keep the view centred.
//---------------------------------------------------------------------------
static void ContentOf( double x, double y, int dir, double& cx, double& cy )
{
    switch ( dir & 3 )
    {
    case 0: cx =  x + y; cy = ( -x + y ) / 2; break;
    case 1: cx = -x + y; cy = ( -x - y ) / 2; break;
    case 2: cx = -x - y; cy = (  x - y ) / 2; break;
    default:cx =  x - y; cy = (  x + y ) / 2; break;
    }
}

static void WorldOfContentDelta( double dcx, double dcy, int dir, double& dx, double& dy )
{
    // invert ContentOf (linear): solve per dir.
    double s = dcx, t = 2 * dcy;
    switch ( dir & 3 )
    {
    case 0: dx = ( s - t ) / 2; dy = ( s + t ) / 2; break;   // s=x+y, t=y-x
    case 1: dx = ( -s - t ) / 2; dy = ( s - t ) / 2; break;  // s=y-x, t=-x-y
    case 2: dx = ( -s + t ) / 2; dy = ( -s - t ) / 2; break; // s=-x-y, t=x-y
    default:dx = ( s + t ) / 2; dy = ( t - s ) / 2; break;   // s=x-y, t=x+y
    }
}

//---------------------------------------------------------------------------
// Shore display: what each hex is DRAWN as, via the game's render-only rule
// (enterrain::CoastDisplay: floating land fragments re-picked or drawn as shoals).
// The map's stored types/indices are untouched. M toggles the rule off (the raw
// stored facings, as before the fix); O outlines the hexes the rule redraws.
//---------------------------------------------------------------------------
struct ShoreView : public HexGrid
{
    const TestMap* m = nullptr;
    std::vector<int> t, i;                 // displayed type / index
    std::vector<unsigned char> changed;    // drawn differently from the stored facing
    int nChanged = 0, nShoal = 0;

    int Width() const override { return m->w; }
    int Height() const override { return m->h; }
    int Type( int x, int y ) const override { return t[y * m->w + x]; }
    int Index( int x, int y ) const override { return i[y * m->w + x]; }
    int GrowStage( int x, int y ) const override { return m->GrowStage( x, y ); }
    int Alt( int x, int y ) const override { return m->Alt( x, y ); }
    int W( int x ) const { return ( ( x % m->w ) + m->w ) % m->w; }
    int H( int y ) const { return ( ( y % m->h ) + m->h ) % m->h; }

    void Build( const TestMap& map, bool fix )
    {
        m = &map;
        t = map.type; i = map.index;
        changed.assign( map.w * map.h, 0 );
        nChanged = nShoal = 0;
        if ( !fix ) return;
        for ( int y = 0; y < map.h; ++y )
            for ( int x = 0; x < map.w; ++x )
            {
                if ( map.Type( x, y ) != TerrCoastline ) continue;
                auto at = [&]( int dx, int dy, int& ty, int& ix ) -> bool {
                    ty = map.Type( W( x + dx ), H( y + dy ) ); ix = map.Index( W( x + dx ), H( y + dy ) );
                    return true;
                };
                int dt, di;
                if ( !CoastDisplay( map.Index( x, y ), at, dt, di ) ) continue;
                const int k = y * map.w + x;
                t[k] = dt; i[k] = di; changed[k] = 1; ++nChanged;
                if ( dt != TerrCoastline ) ++nShoal;
            }
    }
};

// outline hexes (screen space): the hexes the shore rule redraws
static void OutlineHexes( SDL_Renderer* r, const ShoreView& sv, const std::vector<unsigned char>& which,
                          const Camera& cam, Uint8 R, Uint8 G, Uint8 B )
{
    SDL_SetRenderDrawColor( r, R, G, B, 255 );
    const TestMap& m = *sv.m;
    auto ad = [&]( int x, int y ) { int a = m.Alt( sv.W( x ), sv.H( y ) ); return a > SeaLevel ? a : SeaLevel; };
    for ( int y = 0; y < m.h; ++y )
        for ( int x = 0; x < m.w; ++x )
        {
            if ( !which[y * m.w + x] ) continue;
            int alt[4] = { ad( x, y ), ad( x + 1, y ), ad( x + 1, y + 1 ), ad( x, y + 1 ) };
            Pt c[4];
            HexContentCorners( x, y, alt, cam.dir, c );
            SDL_Point p[5];
            for ( int k = 0; k < 4; ++k ) { p[k].x = ( c[k].x >> cam.zoom ) - cam.ulX; p[k].y = ( c[k].y >> cam.zoom ) - cam.ulY; }
            p[4] = p[0];
            for ( int o = 0; o < 2; ++o )   // 2px
            {
                SDL_Point q[5];
                for ( int k = 0; k < 5; ++k ) { q[k] = p[k]; q[k].y += o; }
                SDL_RenderDrawLines( r, q, 5 );
            }
        }
}

struct View
{
    double wx = 0, wy = 0;   // centre, world map units
    double liftPx = 0;       // mean terrain lift (alt << TERRAIN_HT_SHIFT), keeps the map centred
    int    dir = 0, zoom = 1;
};

static double MeanLiftPx( const TestMap& m )
{
    double sum = 0;
    for ( int a : m.alt ) sum += ( a > SeaLevel ? a : SeaLevel ) << TerrainHtShift;
    return m.alt.empty() ? 0 : sum / m.alt.size();
}

static Camera MakeCamera( const View& v, int outW, int outH )
{
    double cx, cy;
    ContentOf( v.wx, v.wy, v.dir, cx, cy );
    cy -= v.liftPx;
    Camera c;
    c.dir = v.dir; c.zoom = v.zoom;
    c.ulX = (int)std::floor( cx / ( 1 << v.zoom ) ) - outW / 2;
    c.ulY = (int)std::floor( cy / ( 1 << v.zoom ) ) - outH / 2;
    return c;
}

static void PanPixels( View& v, int dxPx, int dyPx )
{
    double dx, dy;
    WorldOfContentDelta( (double)dxPx * ( 1 << v.zoom ), (double)dyPx * ( 1 << v.zoom ), v.dir, dx, dy );
    v.wx += dx; v.wy += dy;
}

//---------------------------------------------------------------------------
int main( int argc, char** argv )
{
    std::string mapPath, dataArg, shotPath;
    int startDir = 0, startZoom = 1;
    bool dump = false, outline = false;
    bool shoreFix = true;
    for ( int i = 1; i < argc; ++i )
    {
        std::string a = argv[i];
        if ( a == "--data" && i + 1 < argc ) dataArg = argv[++i];
        else if ( a == "--shot" && i + 1 < argc ) shotPath = argv[++i];
        else if ( a == "--dir" && i + 1 < argc ) startDir = atoi( argv[++i] ) & 3;
        else if ( a == "--zoom" && i + 1 < argc ) startZoom = atoi( argv[++i] );
        else if ( a == "--dump" ) dump = true;
        else if ( a == "--raw-shore" ) shoreFix = false;
        else if ( a == "--outline" ) outline = true;
        else if ( a == "-h" || a == "--help" ) { mapPath.clear(); break; }
        else mapPath = a;
    }
    if ( mapPath.empty() )
    {
        printf( "usage: terraintester [--data DIR] [--dir 0-3] [--zoom 0-3] [--shot OUT.png] [--dump] [--raw-shore] [--outline] MAP.txt\n" );
        return 2;
    }
    if ( startZoom < 0 ) startZoom = 0;
    if ( startZoom > NUM_ZOOMS - 1 ) startZoom = NUM_ZOOMS - 1;

    TestMap map;
    std::string err;
    if ( !LoadMap( mapPath, map, err ) ) { fprintf( stderr, "%s: %s\n", mapPath.c_str(), err.c_str() ); return 1; }
    if ( !map.autoShore ) shoreFix = false;   // a manual map (e.g. the coast art sheet) draws as written
    if ( dump )   // the DERIVED map (after the shore pass / road facings): code+index per cell
    {
        static const char kCode[] = "CdflhmopvrusCFR";
        for ( int y = 0; y < map.h; ++y )
        {
            for ( int x = 0; x < map.w; ++x )
            {
                int t = map.type[y * map.w + x];
                char c = ( t == TerrCoastline ) ? 'c' : ( t == TerrFields ? 'F' : ( t >= 0 && t < TerrNumTypes ? kCode[t] : '?' ) );
                printf( "%c%-3d", c, map.index[y * map.w + x] );
            }
            printf( "\n" );
        }
        return 0;
    }

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) { fprintf( stderr, "SDL_Init: %s\n", SDL_GetError() ); return 1; }
    std::string dataDir = ResolveDataDir( dataArg );
    if ( dataDir.empty() ) { fprintf( stderr, "terrain_gpu PNG set not found (use --data DIR)\n" ); return 1; }

    SDL_Window* win = SDL_CreateWindow( "terraintester", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        1280, 800, SDL_WINDOW_RESIZABLE );
    SDL_Renderer* ren = win ? SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE ) : nullptr;
    if ( !ren ) { fprintf( stderr, "renderer: %s\n", SDL_GetError() ); return 1; }

    TileSet tiles;
    int files = tiles.LoadDir( ren, dataDir, []( const std::string& m ) { fprintf( stderr, "%s\n", m.c_str() ); } );
    printf( "loaded %d PNGs (%d tiles) from %s\n", files, (int)tiles.Size(), dataDir.c_str() );

    View view;
    view.dir = startDir; view.zoom = startZoom;
    view.wx = map.w * HexHt / 2.0; view.wy = map.h * HexHt / 2.0;
    view.liftPx = MeanLiftPx( map );

    RenderOptions opt;
    RenderScratch scratch;
    bool animate = true, dragging = false, running = true;
    unsigned frameBase = 0, pausedFrame = 0;
    int shotNo = 0;
    const bool shotMode = !shotPath.empty();   // --shot: save one frame, then exit
    std::string pendingShot;
    if ( shotMode ) { pendingShot = shotPath; shotPath.clear(); }
    int frameCount = 0;

    while ( running )
    {
        SDL_Event e;
        while ( SDL_PollEvent( &e ) )
        {
            if ( e.type == SDL_QUIT ) running = false;
            else if ( e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT ) dragging = true;
            else if ( e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT ) dragging = false;
            else if ( e.type == SDL_MOUSEMOTION && dragging ) PanPixels( view, -e.motion.xrel, -e.motion.yrel );
            else if ( e.type == SDL_MOUSEWHEEL )
            {
                if ( e.wheel.y > 0 && view.zoom > 0 ) --view.zoom;
                if ( e.wheel.y < 0 && view.zoom < NUM_ZOOMS - 1 ) ++view.zoom;
            }
            else if ( e.type == SDL_KEYDOWN )
            {
                SDL_Keycode k = e.key.keysym.sym;
                const int step = 64;
                if ( k == SDLK_ESCAPE ) running = false;
                else if ( k == SDLK_LEFT )  PanPixels( view, -step, 0 );
                else if ( k == SDLK_RIGHT ) PanPixels( view,  step, 0 );
                else if ( k == SDLK_UP )    PanPixels( view, 0, -step );
                else if ( k == SDLK_DOWN )  PanPixels( view, 0,  step );
                else if ( ( k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS ) && view.zoom > 0 ) --view.zoom;
                else if ( ( k == SDLK_MINUS || k == SDLK_KP_MINUS ) && view.zoom < NUM_ZOOMS - 1 ) ++view.zoom;
                else if ( k == SDLK_COMMA )  view.dir = ( view.dir + 3 ) & 3;
                else if ( k == SDLK_PERIOD ) view.dir = ( view.dir + 1 ) & 3;
                else if ( k == SDLK_f ) opt.feather = !opt.feather;
                else if ( k == SDLK_h ) opt.shade = !opt.shade;
                else if ( k == SDLK_a ) { animate = !animate; pausedFrame = opt.frame; frameBase = SDL_GetTicks(); }
                else if ( k == SDLK_r )
                {
                    TestMap m2;
                    if ( LoadMap( mapPath, m2, err ) ) { map = m2; view.liftPx = MeanLiftPx( map ); printf( "reloaded %s (%dx%d)\n", mapPath.c_str(), map.w, map.h ); }
                    else fprintf( stderr, "reload failed: %s\n", err.c_str() );
                }
                else if ( k == SDLK_m ) shoreFix = !shoreFix;
                else if ( k == SDLK_o ) outline = !outline;
                else if ( k == SDLK_s ) shotPath = "terraintester_" + std::to_string( shotNo++ ) + ".png";
            }
        }

        // Game clock: 24 sim frames/s.
        if ( animate ) opt.frame = pausedFrame + ( SDL_GetTicks() - frameBase ) * 24u / 1000u;

        int outW = 0, outH = 0;
        SDL_GetRendererOutputSize( ren, &outW, &outH );
        Camera cam = MakeCamera( view, outW, outH );
        SDL_SetRenderDrawColor( ren, 0, 0, 0, 255 );
        SDL_RenderClear( ren );
        ShoreView sv;
        sv.Build( map, shoreFix );
        int drawn = RenderGrid( ren, tiles, sv, cam, opt, scratch );
        if ( outline )   // yellow = hexes the shore rule redraws
            OutlineHexes( ren, sv, sv.changed, cam, 255, 230, 0 );
        if ( frameCount == 0 && shoreFix )
            printf( "shore fix: %d hexes redrawn (%d as shoal)\n", sv.nChanged, sv.nShoal );

        char title[256];
        snprintf( title, sizeof title, "terraintester - %s  %dx%d  dir %d  zoom %d  hexes %d  feather %s  shade %s  shore fix[M] %s",
                  mapPath.c_str(), map.w, map.h, view.dir, view.zoom, drawn,
                  opt.feather ? "on" : "off", opt.shade ? "on" : "off", shoreFix ? "on" : "off" );
        SDL_SetWindowTitle( win, title );

        ++frameCount;
        if ( shotMode && frameCount == 3 )   // a few frames in, so the window is up
        {
            SaveShot( ren, pendingShot );
            running = false;
        }
        else if ( !shotPath.empty() )
        {
            SaveShot( ren, shotPath );
            shotPath.clear();
        }
        SDL_RenderPresent( ren );
        SDL_Delay( 10 );
    }

    scratch.Release();
    tiles.Unload();
    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    SDL_Quit();
    return 0;
}
