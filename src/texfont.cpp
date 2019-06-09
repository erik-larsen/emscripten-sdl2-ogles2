//
// This is a fork of GLUT TexFont code, ported to C++ and OpenGLES.
//
// https://github.com/markkilgard/glut/tree/master/progs/texfont
// https://web.archive.org/web/20010616211947/http://reality.sgi.com/opengl/tips/TexFont/TexFont.html
//

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "texfont.h"

//#define TXF_DEBUG 1

// byte swap a 32-bit value 
inline void byteSwap32Bit(int* val)
{
    char *valBytes = (char *)val;
    char temp = valBytes[0];
    valBytes[0] = valBytes[3];
    valBytes[3] = temp;
    temp = valBytes[1];
    valBytes[1] = valBytes[2];
    valBytes[2] = temp; 
}

// byte swap a 16-bit value (short)
inline void byteSwap16Bit(short* val)
{
    char *valBytes = (char *)val;
    char temp = valBytes[0];
    valBytes[0] = valBytes[1];
    valBytes[1] = temp;
}

static char *lastError;

char *
txfErrorString(void)
{
    return lastError;
}

static TexGlyphVertexInfo *
getTCVI(TexFont * txf, int c)
{
    // Automatically substitute uppercase letters with lowercase if not
    // uppercase available (and vice versa). 
    if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) 
    {
        TexGlyphVertexInfo *tgvi = txf->lut[c - txf->min_glyph];
        if (tgvi) 
            return tgvi;

        if (islower(c)) 
        {
            c = toupper(c);
            if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range))
            {
                return txf->lut[c - txf->min_glyph];
            }
        }
        if (isupper(c)) 
        {
            c = tolower(c);
            if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) 
            {
                return txf->lut[c - txf->min_glyph];
            }
        }
    }
    printf("texfont: tried to access unavailable font character \"%c\" (%d)\n", isprint(c) ? c : ' ', c);
    return NULL;
}

void txfLoadFontError(const char* errorStr, TexFont *txf, FILE* file)
{
    lastError = (char*)errorStr;
    printf("%s\n", lastError);
    txfUnloadFont(txf);
    if (file)
        fclose(file);
}

TexFont *
txfLoadFont(const char *filename)
{    
    #define TXF_LOAD_ERROR(errorStr) { txfLoadFontError(errorStr, txf, file); return NULL; }
    #define TXF_LOAD_EXPECT_GOT(n) if (got != n) { txfLoadFontError("premature end of file.", txf, file); return NULL; }

    TexFont *txf = NULL;

    FILE *file = fopen(filename, "rb");
    if (file == NULL) 
        TXF_LOAD_ERROR("file open failed.");

    txf = new TexFont;
    if (txf == NULL) 
        TXF_LOAD_ERROR("out of memory.");

    txf->teximage = NULL;
    txf->tgi = NULL;
    txf->tgvi = NULL;
    txf->lut = NULL;

    char fileid[4];
    unsigned long got = fread(fileid, 1, 4, file);
    if (got != 4 || strncmp(fileid, "\377txf", 4)) 
        TXF_LOAD_ERROR("not a texture font file.");

    assert(sizeof(int) == 4);    // Ensure external file format size. 
    int endianness, swap;
    got = fread(&endianness, sizeof(int), 1, file);
    if (got == 1 && endianness == 0x12345678) 
        swap = 0;
    else if (got == 1 && endianness == 0x78563412)
        swap = 1;
    else 
        TXF_LOAD_ERROR("not a texture font file.");

    int format; 
    got = fread(&format, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);
    got = fread(&txf->tex_width, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);
    got = fread(&txf->tex_height, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);
    got = fread(&txf->max_ascent, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);
    got = fread(&txf->max_descent, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);
    got = fread(&txf->num_glyphs, sizeof(int), 1, file);
    TXF_LOAD_EXPECT_GOT(1);

    if (swap) 
    {
        byteSwap32Bit(&format);
        byteSwap32Bit(&txf->tex_width);
        byteSwap32Bit(&txf->tex_height);
        byteSwap32Bit(&txf->max_ascent);
        byteSwap32Bit(&txf->max_descent);
        byteSwap32Bit(&txf->num_glyphs);
    }
    txf->tgi = new TexGlyphInfo[txf->num_glyphs];
    if (txf->tgi == NULL)
        TXF_LOAD_ERROR("out of memory.");

    assert(sizeof(TexGlyphInfo) == 12);    // Ensure external file format size. 
    got = fread(txf->tgi, sizeof(TexGlyphInfo), txf->num_glyphs, file);
    TXF_LOAD_EXPECT_GOT(txf->num_glyphs);

    if (swap) 
    {
        for (int i = 0; i < txf->num_glyphs; i++) 
        {
            byteSwap16Bit((short*)&txf->tgi[i].c);
            byteSwap16Bit(&txf->tgi[i].x);
            byteSwap16Bit(&txf->tgi[i].y);
        }
    }
    txf->tgvi = new TexGlyphVertexInfo[txf->num_glyphs];
    if (txf->tgvi == NULL) 
        TXF_LOAD_ERROR("out of memory.");

    GLfloat w = txf->tex_width, h = txf->tex_height;
    GLfloat xstep = 0.5 / w, ystep = 0.5 / h;

    for (int i = 0; i < txf->num_glyphs; i++) 
    {
        TexGlyphInfo *tgi = &txf->tgi[i];
        txf->tgvi[i].t0[0] = tgi->x / w + xstep;
        txf->tgvi[i].t0[1] = tgi->y / h + ystep;
        txf->tgvi[i].v0[0] = tgi->xoffset;
        txf->tgvi[i].v0[1] = tgi->yoffset;
        txf->tgvi[i].t1[0] = (tgi->x + tgi->width) / w + xstep;
        txf->tgvi[i].t1[1] = tgi->y / h + ystep;
        txf->tgvi[i].v1[0] = tgi->xoffset + tgi->width;
        txf->tgvi[i].v1[1] = tgi->yoffset;
        txf->tgvi[i].t2[0] = (tgi->x + tgi->width) / w + xstep;
        txf->tgvi[i].t2[1] = (tgi->y + tgi->height) / h + ystep;
        txf->tgvi[i].v2[0] = tgi->xoffset + tgi->width;
        txf->tgvi[i].v2[1] = tgi->yoffset + tgi->height;
        txf->tgvi[i].t3[0] = tgi->x / w + xstep;
        txf->tgvi[i].t3[1] = (tgi->y + tgi->height) / h + ystep;
        txf->tgvi[i].v3[0] = tgi->xoffset;
        txf->tgvi[i].v3[1] = tgi->yoffset + tgi->height;
        txf->tgvi[i].advance = tgi->advance;

        // Build glyph vertex array quad, stored as tristrip (4 vertices)
        //
        TexGlyphVertexInfo& tgvi = txf->tgvi[i];
        #ifdef TXF_DEBUG        
            printf ("tgvi #%d '%c'\n", i, tgi->c);
            printf ("texCoord 0 %f,%f  ", tgvi.t0[0], tgvi.t0[1]);
            printf ("position 0 %d,%d\n", tgvi.v0[0], tgvi.v0[1]);
            printf ("texCoord 1 %f,%f  ", tgvi.t1[0], tgvi.t1[1]);
            printf ("position 1 %d,%d\n", tgvi.v1[0], tgvi.v1[1]);
            printf ("texCoord 2 %f,%f  ", tgvi.t2[0], tgvi.t2[1]);
            printf ("position 2 %d,%d\n", tgvi.v2[0], tgvi.v2[1]);
            printf ("texCoord 3 %f,%f  ", tgvi.t3[0], tgvi.t3[1]);
            printf ("position 3 %d,%d\n", tgvi.v3[0], tgvi.v3[1]);
        #endif

        GLfloat* va = tgvi.vertexArray;
        int offset = 0;
        va[offset+0] = tgvi.v3[0]; //0.0f;
        va[offset+1] = tgvi.v3[1]; //1.0f;
        va[offset+2] = 0.0f;
        va[offset+3] = tgvi.t3[0];
        va[offset+4] = tgvi.t3[1];

        offset += 5;
        va[offset+0] = tgvi.v2[0]; //1.0f;
        va[offset+1] = tgvi.v2[1]; //1.0f;
        va[offset+2] = 0.0f;
        va[offset+3] = tgvi.t2[0];
        va[offset+4] = tgvi.t2[1];

        offset += 5;
        va[offset+0] = tgvi.v0[0]; //0.0f;
        va[offset+1] = tgvi.v0[1]; //0.0f;
        va[offset+2] = 0.0f;
        va[offset+3] = tgvi.t0[0];
        va[offset+4] = tgvi.t0[1];

        offset += 5;
        va[offset+0] = tgvi.v1[0]; //1.0f;
        va[offset+1] = tgvi.v1[1]; //0.0f;
        va[offset+2] = 0.0f;
        va[offset+3] = tgvi.t1[0];
        va[offset+4] = tgvi.t1[1];       

        // Correct tgvi.advance read in from txf file
        // In rockfont.txf, advance = max x + min x, should be = max x - min x + letter spacing
        GLfloat minX = va[0];
        for (int i = 1; i < 4; ++i)
        {
            if (va[i * 5] < minX)
                minX = va[i * 5];
        }
        tgvi.advance -= (minX * 2.0f);
        const float letterSpacing = 3.0f;
        tgvi.advance += letterSpacing;
    }

    int min_glyph = txf->tgi[0].c;
    int max_glyph = min_glyph;
    for (int i = 1; i < txf->num_glyphs; i++) 
    {
        if (txf->tgi[i].c < min_glyph) 
            min_glyph = txf->tgi[i].c;
        if (txf->tgi[i].c > max_glyph) 
            max_glyph = txf->tgi[i].c;
    }
    txf->min_glyph = min_glyph;
    txf->range = max_glyph - min_glyph + 1;

    txf->lut = new TexGlyphVertexInfo*[txf->range];
    if (txf->lut == NULL)
        TXF_LOAD_ERROR("out of memory.");
    for (int i = 0; i < txf->range; ++i)
        txf->lut[i] = NULL;
    for (int i = 0; i < txf->num_glyphs; i++) 
        txf->lut[txf->tgi[i].c - txf->min_glyph] = &txf->tgvi[i];

    switch (format) 
    {
        case TXF_FORMAT_BYTE:
            {
                txf->teximage = new unsigned char[txf->tex_width * txf->tex_height];
                if (txf->teximage == NULL)
                    TXF_LOAD_ERROR("out of memory.");
                got = fread(txf->teximage, 1, txf->tex_width * txf->tex_height, file);

                TXF_LOAD_EXPECT_GOT(txf->tex_width * txf->tex_height);

                #ifdef TXF_DEBUG
                    printf("TXF_FORMAT_BYTE\n");
                    for (int i = 0; i < txf->tex_width * txf->tex_height; ++i)
                        printf("%x ", txf->teximage[i]);
                    printf("\n");
                #endif
            }
            break;
            
        case TXF_FORMAT_BITMAP:
            {
                int width = txf->tex_width;
                int height = txf->tex_height;
                int stride = (width + 7) >> 3;
                unsigned char *texbitmap = new unsigned char[stride * height];
                if (texbitmap == NULL)
                    TXF_LOAD_ERROR("out of memory.");

                got = fread(texbitmap, 1, stride * height, file);
                TXF_LOAD_EXPECT_GOT(stride * height);
                
                txf->teximage = new unsigned char[width * height];
                if (txf->teximage == NULL)
                    TXF_LOAD_ERROR("out of memory.");
                
                for (int i = 0; i < height; i++) 
                {
                    for (int j = 0; j < width; j++) 
                    {
                        if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) 
                            txf->teximage[i * width + j] = 255;
                        else
                            txf->teximage[i * width + j] = 0;
                    }
                }
                
                delete[] texbitmap;

                #ifdef TXF_DEBUG
                    printf("TXF_FORMAT_BITMAP\n");
                    for (int i = 0; i < width * height; ++i)
                        printf("%x ", txf->teximage[i]);
                    printf("\n");
                #endif
            }
            break;
    }

    fclose(file);
    return txf;
}

GLuint
txfEstablishTexture(TexFont * txf, GLuint texobj)
{
    if (txf->texobj == 0) 
    {
        if (texobj == 0)
            glGenTextures(1, &txf->texobj);
        else
            txf->texobj = texobj;
    }
 
    glBindTexture(GL_TEXTURE_2D, txf->texobj);
    const GLenum format = GL_ALPHA; // r,g,b = 0,0,0; a = teximage
    glTexImage2D(GL_TEXTURE_2D, 0, format,
        txf->tex_width, txf->tex_height, 0,
        format, GL_UNSIGNED_BYTE, txf->teximage);

    return txf->texobj;
}

void
txfBindFontTexture(TexFont * txf)
{
    glBindTexture(GL_TEXTURE_2D, txf->texobj);
}

void
txfGetStringMetrics(
    TexFont * txf,
    const char *string,
    int len,
    int *width,
    int *max_ascent,
    int *max_descent)
{
    TexGlyphVertexInfo *tgvi;

    int w = 0;
    for (int i = 0; i < len; i++) 
    {
        if (string[i] == 27) 
        {
            switch (string[i + 1]) 
            {
                case 'M': i += 4; break;
                case 'T': i += 7; break;
                case 'L': i += 7; break;
                case 'F': i += 13; break;
            }
        } 
        else 
        {
            tgvi = getTCVI(txf, string[i]);
            w += tgvi->advance;
        }
    }
    *width = w;
    *max_ascent = txf->max_ascent;
    *max_descent = txf->max_descent;
}

void
txfRenderString(TexFont * txf, const char *str, float x, float y)
{
    size_t numChars = strlen(str);
    if (txf && numChars > 0)
    {
        const GLint vertexPositionFloats = 3, // x,y,z
                    vertexTexCoordFloats = 2, // u,v
                    vertexFloats = vertexPositionFloats + vertexTexCoordFloats, // x,y,z + u,v = 5
                    vertexBytes = vertexFloats * sizeof(GLfloat),
                    quadVertices = 6, // two independent triangles = 6 vertices
                    quadFloats = vertexFloats * quadVertices, // 5 * 6 = 30 floats
                    vertexArrayVertices = quadVertices * numChars, // 6 * numchars
                    vertexArrayFloats = vertexFloats * vertexArrayVertices, // 5 * 6 * numchars
                    vertexArrayBytes = vertexBytes * vertexArrayVertices;

        GLuint quadsVboId = 0;

        auto stringVBO = txf->stringVBOs.find(str);
        if (stringVBO == txf->stringVBOs.end())
        {
            // Not found - build VBO and add to map
            GLfloat* stringVertexArray = new GLfloat[vertexArrayFloats];

            // Start advance at caller specified x
            GLfloat advance = x;

            for (int i = 0; i < numChars; ++i)
            {
                char c = str[i];
                int quadIndex = i * quadFloats;

                TexGlyphVertexInfo *tgvi = getTCVI(txf, c);
                if (tgvi)
                {
                    // Copy char's vertexArray into stringVertexArray
                    // Triangle #1
                    for (int j = 0; j < 3; ++j) // vertices
                        for (int k = 0; k < 5; ++k) // floats x,y,z,u,v
                            stringVertexArray[i * 5 * 6 + j * 5 + k] = tgvi->vertexArray[j * 5 + k];

                    // Triangle #2
                    for (int j = 3; j < 6; ++j) // vertices
                        for (int k = 0; k < 5; ++k) // floats x,y,z,u,v
                            stringVertexArray[i * 5 * 6 + j * 5 + k] = tgvi->vertexArray[(j - 2) * 5 + k];


                    // Translate x positions by accumulated advance
                    // Translate y positions by caller specified y
                    for (int j = 0; j < 6; ++j)
                    {
                        stringVertexArray[i * 5 * 6 + j * 5] += advance;
                        stringVertexArray[i * 5 * 6 + j * 5 + 1] += y;
                    }

                    advance += tgvi->advance;

                    #ifdef TXF_DEBUG
                        for (int i = 0; i < 4; ++i)
                        {
                            GLfloat* va = tgvi->vertexArray;
                            printf("tgvi va pos[%d] (%f,%f,%f) tex[%d] (%f,%f)\n",
                                    i, va[i*5], va[i*5+1], va[i*5+2],
                                    i, va[i*5+3], va[i*5+4]);
                        }
                        printf ("tgvi advance %f\n", tgvi->advance);

                        for (int i = 0; i < numVertices; ++i)
                        {
                            GLfloat* va = stringVertexArray;
                            printf("strva pos[%d] (%f,%f,%f) tex[%d] (%f,%f)\n",
                                    i, va[i*5], va[i*5+1], va[i*5+2],
                                    i, va[i*5+3], va[i*5+4]);
                        }
                    #endif
                }
            }

            // Build VBO
            glGenBuffers(1, &quadsVboId);
            glBindBuffer(GL_ARRAY_BUFFER, quadsVboId);
            glBufferData(GL_ARRAY_BUFFER, 5 * 6 * numChars * sizeof(GLfloat), stringVertexArray, GL_STATIC_DRAW);
            delete[] stringVertexArray;

            // Cache the string/VBO pair
            txf->stringVBOs.insert({str, quadsVboId});
        }
        else 
        {
            // Found - bind VBO
            quadsVboId = stringVBO->second;
            glBindBuffer(GL_ARRAY_BUFFER, quadsVboId);
        }

        // Draw the string VBO
        if (quadsVboId != 0)
        {
            const GLuint vertexPositionIndex = 0, 
                         vertexTexCoordIndex = 1;            
            GLuint offset = 0;
            glVertexAttribPointer(vertexPositionIndex, vertexPositionFloats, GL_FLOAT, GL_FALSE, vertexFloats * sizeof(GLfloat), (const void*)offset);
            offset += vertexPositionFloats * sizeof(GLfloat);
            glVertexAttribPointer(vertexTexCoordIndex, vertexTexCoordFloats, GL_FLOAT, GL_FALSE, vertexFloats * sizeof(GLfloat), (const void*)offset);

            glDrawArrays(GL_TRIANGLES, 0, vertexArrayVertices);
        }
    }
}

void
txfUnloadFont(TexFont * txf)
{
    if (txf)
    {
        if (txf->texobj != 0)
            glDeleteTextures(1, &txf->texobj);

        for (auto stringVBO = txf->stringVBOs.begin(); stringVBO != txf->stringVBOs.end(); ++stringVBO)
            glDeleteBuffers(1, &stringVBO->second);

        delete[] txf->teximage;
        delete[] txf->tgi;
        delete[] txf->tgvi;
        delete[] txf->lut;
        delete txf;
    }
}