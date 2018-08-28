//
// This is a fork of GLUT TexFont code, ported to C++ and OpenGLES.
//
// https://github.com/markkilgard/glut/tree/master/progs/texfont
// https://web.archive.org/web/20010616211947/http://reality.sgi.com/opengl/tips/TexFont/TexFont.html
//
#include <string>
#include <unordered_map>
#include <SDL_opengles2.h>

enum TxfFormat {TXF_FORMAT_BYTE, TXF_FORMAT_BITMAP};

typedef struct {
    unsigned short c;       // Potentially support 16-bit glyphs.
    unsigned char width;
    unsigned char height;
    signed char xoffset;
    signed char yoffset;
    signed char advance;
    char dummy;             // Space holder for alignment reasons.
    short x;
    short y;
} TexGlyphInfo;

typedef struct {
    GLfloat t0[2];
    GLshort v0[2];
    GLfloat t1[2];
    GLshort v1[2];
    GLfloat t2[2];
    GLshort v2[2];
    GLfloat t3[2];
    GLshort v3[2];
    GLfloat advance;
    GLfloat vertexArray[(3+2)*4];
} TexGlyphVertexInfo;

typedef struct {
    GLuint texobj;
    int tex_width;
    int tex_height;
    int max_ascent;
    int max_descent;
    int num_glyphs;
    int min_glyph;
    int range;
    unsigned char *teximage;
    TexGlyphInfo *tgi;
    TexGlyphVertexInfo *tgvi;
    TexGlyphVertexInfo **lut;
    std::unordered_map<std::string, GLuint> stringVBOs;
} TexFont;

extern char *txfErrorString(void);

extern TexFont *txfLoadFont(
    const char *filename);

extern void txfUnloadFont(
    TexFont * txf);

extern GLuint txfEstablishTexture(
    TexFont * txf,
    GLuint texobj);

extern void txfBindFontTexture(
    TexFont * txf);

extern void txfGetStringMetrics(
    TexFont * txf,
    const char *str,
    int len,
    int *width,
    int *max_ascent,
    int *max_descent);

extern void txfRenderString(
    TexFont * txf,
    const char *string,
    float x, float y);
