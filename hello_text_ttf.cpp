//
// Emscripten/SDL2/OpenGLES2 sample that displays TrueType text by loading a font and building a string texture 
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc hello_text_ttf.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 -s FULL_ES2=1 -s WASM=0 --preload-file media/LiberationSansBold.ttf -o hello_text_ttf_debug.html
// 
// Run:
//     emrun hello_text_ttf_debug.html
//
// Result:
//     A TrueType text quad and colorful triangle.  Left mouse pans, mouse wheel zooms in/out.
//
#include <exception>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_opengles2.h>

// Mouse input
const float MOUSE_WHEEL_ZOOM_DELTA = 0.05f;
bool mouseButtonDown = false;
int mouseButtonDownX = 0, mouseButtonDownY = 0;
int mousePositionX = 0, mousePositionY = 0;

// Finger input
bool fingerDown = false;
float fingerDownX = 0.0f, fingerDownY = 0.0f;
long long fingerDownId = 0;

// Pinch input
const float PINCH_ZOOM_THRESHOLD = 0.001f;
const float PINCH_SCALE = 8.0f;
bool pinch = false;

// Window
SDL_Window* window = nullptr;
Uint32 windowID = 0;
int windowWidth = 640, windowHeight = 480;

// Geometry
GLfloat triangleVertices[] = 
{
    0.0f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f
};
GLuint triangleVbo = 0;

GLfloat quadVertices[] = 
{
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f
};
GLuint quadVbo = 0;

// Texture
GLuint textureObj = 0;

// Text
const char* FONT_NAME = "media/LiberationSansBold.ttf";
const int FONT_POINT_SIZE = 64;
const char* message = "Hello Text";

// Shader vars
const GLint positionAttrib = 0;
GLint shaderPan, shaderZoom, shaderAspect, shaderViewport, shaderTextSize, shaderTexSize;
GLfloat pan[2] = {0.0f, 0.0f}, zoom = 1.0f, aspect = 1.0f, viewport[2] = {640.0f, 480.0f}, textSize[2] = {0.0f, 0.0f}, texSize[2] = {0.0f, 0.0f};

const GLfloat ZOOM_MIN = 0.1f, ZOOM_MAX = 10.0f;
GLfloat basePan[2] = {0.0f, 0.0f};

//  Quad vertex & fragment shaders
GLuint quadShaderProgram = 0;
const GLchar* quadVertexSource =
    "attribute vec4 position;                                   \n"
    "varying vec2 texCoord;                                     \n"
    "uniform vec2 viewport;                                     \n"
    "uniform vec2 textSize;                                     \n"
    "uniform vec2 texSize;                                      \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_Position = vec4(position.xyz, 1.0);                 \n"
    "    gl_Position.x *= textSize.x;                           \n"
    "    gl_Position.y *= textSize.y;                           \n"
    "                                                           \n"
    "    // Translate to lower left viewport                    \n"
    "    gl_Position.x -= viewport.x / 2.0;                     \n"
    "    gl_Position.y -= viewport.y / 2.0;                     \n"
    "                                                           \n"
    "    // Ortho projection                                    \n"
    "    gl_Position.x += 1.0;                                  \n"
    "    gl_Position.x *= 2.0 / viewport.x;                     \n"
    "    gl_Position.y += 1.0;                                  \n"
    "    gl_Position.y *= 2.0 / viewport.y;                     \n"
    "                                                           \n"
    "    // Text subrectangle from overall texture              \n"
    "    texCoord.x = position.x * textSize.x / texSize.x;      \n"
    "    texCoord.y = -position.y * textSize.y / texSize.y;     \n"
    "}                                                          \n";

const GLchar* quadFragmentSource =
    "precision mediump float;                                   \n"
    "varying vec2 texCoord;                                     \n"
    "uniform sampler2D texSampler;                              \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_FragColor = texture2D(texSampler, texCoord);        \n"
    "}                                                          \n";

// Triangle vertex & fragment shaders
GLuint triShaderProgram = 0;
const GLchar* triVertexSource =
    "uniform vec2 pan;                             \n"
    "uniform float zoom;                           \n"
    "uniform float aspect;                         \n"
    "attribute vec4 position;                      \n"
    "varying vec3 color;                           \n"
    "void main()                                   \n"
    "{                                             \n"
    "    gl_Position = vec4(position.xyz, 1.0);    \n"
    "    gl_Position.xy += pan;                    \n"
    "    gl_Position.xy *= zoom;                   \n"
    "    gl_Position.y *= aspect;                  \n"
    "    color = gl_Position.xyz + vec3(0.5);      \n"
    "}                                             \n";

const GLchar* triFragmentSource =
    "precision mediump float;                     \n"
    "varying vec3 color;                          \n"
    "void main()                                  \n"
    "{                                            \n"
    "    gl_FragColor = vec4 ( color, 1.0 );      \n"
    "}                                            \n";


float clamp (float val, float lo, float hi)
{
    return std::max(lo, std::min(val, hi));
}

int nextPowerOfTwo(int val)
{
    int power = 1;
    while (power < val)
        power *= 2;
    return power;
}

void updateShaderUniforms()
{
    glUseProgram(quadShaderProgram);
    glUniform2fv(shaderViewport, 1, viewport);
    glUniform2fv(shaderTextSize, 1, textSize);
    glUniform2fv(shaderTexSize, 1, texSize);

    glUseProgram(triShaderProgram);
    glUniform2fv(shaderPan, 1, pan);
    glUniform1f(shaderZoom, zoom); 
    glUniform1f(shaderAspect, aspect);
}

GLuint initShader(const GLchar* vertexSource, const GLchar* fragmentSource)
{
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link vertex and fragment shader into shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindAttribLocation(shaderProgram, positionAttrib, "position");
    glEnableVertexAttribArray(positionAttrib);
    glLinkProgram(shaderProgram);

    return shaderProgram;
}

void initShaders()
{
    // Compile & link shaders
    quadShaderProgram = initShader(quadVertexSource, quadFragmentSource);
    triShaderProgram = initShader(triVertexSource, triFragmentSource);

    // Get shader variables and initalize them
    shaderViewport = glGetUniformLocation(quadShaderProgram, "viewport");
    shaderTextSize = glGetUniformLocation(quadShaderProgram, "textSize");
    shaderTexSize = glGetUniformLocation(quadShaderProgram, "texSize");

    shaderPan = glGetUniformLocation(triShaderProgram, "pan");
    shaderZoom = glGetUniformLocation(triShaderProgram, "zoom");    
    shaderAspect = glGetUniformLocation(triShaderProgram, "aspect");
    
    updateShaderUniforms();
}

void initGeometry()
{
   // Create vertex buffer objects and copy vertex data into them
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);  
 }

void debugPrintSurface(SDL_Surface* surface, const char* name, bool dumpPixels)
{
    printf ("%s dimensions %dx%d, %d bits per pixel\n", name, surface->w, surface->h, surface->format->BitsPerPixel);
    if (dumpPixels)
    {
        for (int i = 0; i < surface->w * surface->h; ++i)
            printf("%x ", ((unsigned int*)surface->pixels)[i]);
        printf("\n");
    }
}

void initTextTexture()
{
    TTF_Init();

    // Load the font
    TTF_Font *font = TTF_OpenFont(FONT_NAME, FONT_POINT_SIZE);
    if (font) 
    {
        // Render text to surface
        SDL_Color foregroundColor = {255,255,255,255};
        SDL_Surface* textImage8Bit = TTF_RenderText_Solid(font, message, foregroundColor);
        
        if (textImage8Bit)
        {
            // Convert surface from 8 to 32 bit for GL
            SDL_Surface* textImage = SDL_ConvertSurfaceFormat(textImage8Bit, SDL_PIXELFORMAT_RGBA8888, 0);
            debugPrintSurface(textImage, "textImage", false);

            // Create power of 2 dimensioned texture for GL with 1 texel border, clear it, and copy text image into it
            SDL_Surface* texture = SDL_CreateRGBSurface(0, nextPowerOfTwo(textImage->w + 2), nextPowerOfTwo(textImage->h + 2), 
                                                        textImage->format->BitsPerPixel, 0, 0, 0, 0);
            memset(texture->pixels, 0x0, texture->w * texture->h * texture->format->BytesPerPixel);
            SDL_Rect destRect = {1, texture->h - textImage->h - 1, textImage->w + 1, texture->h - 1};
            SDL_SetSurfaceBlendMode(textImage, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(textImage, NULL, texture, &destRect);
 
            // Emscripten/SDL bug? SDL_BlitSurface should copy source alpha when source surface is set to 
            // SDL_BLENDMODE_NONE, however this is not happening, so fix it up here
            unsigned int* pixels = (unsigned int*)texture->pixels;
            for (int i = 0; i < texture->w*texture->h; ++i)
            {
                if (pixels[i] != 0)
                    pixels[i] |= 0xff000000;
                else
                    pixels[i] = 0x80808080;
            }
            debugPrintSurface(texture, "texture", false);

            // Determine GL texture format
            GLint format = -1;
            if (texture->format->BitsPerPixel == 24)
                format = GL_RGB;
            else if (texture->format->BitsPerPixel == 32)
                format = GL_RGBA;

            if (format != -1)
            {
                // Enable blending for texture alpha component
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // Generate a GL texture object
                glGenTextures(1, &textureObj);

                // Bind GL texture
                glBindTexture(GL_TEXTURE_2D, textureObj);

                // Set the GL texture's wrapping and stretching properties
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                // Copy SDL surface image to GL texture
                glTexImage2D(GL_TEXTURE_2D, 0, format, 
                             texture->w, texture->h, 
                             0, format, GL_UNSIGNED_BYTE, texture->pixels);

                // Update quad shader
                texSize[0] = (GLfloat)texture->w;
                texSize[1] = (GLfloat)texture->h;
                textSize[0] = (GLfloat)textImage->w + 2;
                textSize[1] = (GLfloat)textImage->h + 2;
                updateShaderUniforms();
            }
                                    
            SDL_FreeSurface (textImage);        
            SDL_FreeSurface (texture);        
        }  
        TTF_CloseFont(font);
    }
    else
        printf("Failed to load font %s, due to %s\n", FONT_NAME, TTF_GetError());
}

void redraw()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle VBO with a colorful shader
    glUseProgram(triShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Draw the quad VBO with a text texture shader
    glUseProgram(quadShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Swap front/back framebuffers
    SDL_GL_SwapWindow(window);
}

// Convert from normalized viewport coords (x,y) in ([0.0, 1.0], [1.0, 0.0]) to device coords ([-1.0, 1.0], [-1.0,1.0])
void normWindowToDeviceCoords (float normWinX, float normWinY, float& deviceX, float& deviceY)
{
    deviceX = (normWinX - 0.5f) * 2.0f;
    deviceY = (1.0f - normWinY - 0.5f) * 2.0f;
}

// Convert from viewport coords (x,y) in ([0, windowWidth], [windowHeight, 0]) to device coords ([-1.0, 1.0], [-1.0,1.0])
void windowToDeviceCoords (int winX, int winY, float& deviceX, float& deviceY)
{
    normWindowToDeviceCoords(winX / (float)windowWidth,  winY / (float)windowHeight, deviceX, deviceY);
}

// Convert from device coords ([-1.0, 1.0], [-1.0,1.0]) to world coords ([-inf, inf], [-inf, inf])
void deviceToWorldCoords (float deviceX, float deviceY, float& worldX, float& worldY)
{
    worldX = deviceX / zoom - pan[0];
    worldY = deviceY / aspect / zoom - pan[1];
}

// Convert from viewport coords (x,y) in ([0, windowWidth], [windowHeight, 0]) to world coords ([-inf, inf], [-inf, inf])
void windowToWorldCoords(int winX, int winY, float& worldX, float& worldY)
{
    float deviceX, deviceY;
    windowToDeviceCoords(winX, winY, deviceX, deviceY);   
    deviceToWorldCoords(deviceX, deviceY, worldX, worldY);
}

// Convert from normalized viewport coords (x,y) in in ([0.0, 1.0], [1.0, 0.0]) to world coords ([-inf, inf], [-inf, inf])
void normWindowToWorldCoords(float normWinX, float normWinY, float& worldX, float& worldY)
{
    float deviceX, deviceY;
    normWindowToDeviceCoords(normWinX, normWinY, deviceX, deviceY);
    deviceToWorldCoords(deviceX, deviceY, worldX, worldY);
}

void windowResizeEvent(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    // Update viewport and aspect ratio
    glViewport(0, 0, windowWidth, windowHeight);
    aspect = windowWidth / (float)windowHeight; 
    viewport[0] = (float)windowWidth;
    viewport[1] = (float)windowHeight;

    updateShaderUniforms();
}

void zoomEventMouse(bool mouseWheelDown, int x, int y)
{                
    float preZoomWorldX, preZoomWorldY;
    windowToWorldCoords(mousePositionX, mousePositionY, preZoomWorldX, preZoomWorldY);

    // Zoom by scaling up/down in 0.05 increments 
    float zoomDelta = mouseWheelDown ? -MOUSE_WHEEL_ZOOM_DELTA : MOUSE_WHEEL_ZOOM_DELTA;
    zoom += zoomDelta;

    // Limit zooming to finite range
    zoom = clamp(zoom, ZOOM_MIN, ZOOM_MAX);

    float postZoomWorldX, postZoomWorldY;
    windowToWorldCoords(mousePositionX, mousePositionY, postZoomWorldX, postZoomWorldY);

    // Zoom to point: Keep the world coords under mouse position the same before and after the zoom
    float deltaWorldX = postZoomWorldX - preZoomWorldX, deltaWorldY = postZoomWorldY - preZoomWorldY;
    pan[0] += deltaWorldX;
    pan[1] += deltaWorldY;

    updateShaderUniforms();
}

void zoomEventPinch (float pinchDist, float pinchX, float pinchY)
{
    float preZoomWorldX, preZoomWorldY;
    normWindowToWorldCoords(pinchX, pinchY, preZoomWorldX, preZoomWorldY);

    // Zoom in/out by positive/negative pinch distance
    float zoomDelta = pinchDist * PINCH_SCALE;
    zoom += zoomDelta;

    // Limit zooming to finite range
    zoom = clamp(zoom, ZOOM_MIN, ZOOM_MAX);

    float postZoomWorldX, postZoomWorldY;
    normWindowToWorldCoords(pinchX, pinchY, postZoomWorldX, postZoomWorldY);

    // Zoom to point: Keep the world coords under pinch position the same before and after the zoom
    float deltaWorldX = postZoomWorldX - preZoomWorldX, deltaWorldY = postZoomWorldY - preZoomWorldY;
    pan[0] += deltaWorldX;
    pan[1] += deltaWorldY;

    updateShaderUniforms();
}

void panEventMouse(int x, int y)
{ 
     int deltaX = windowWidth / 2 + (x - mouseButtonDownX),
         deltaY = windowHeight / 2 + (y - mouseButtonDownY);

    float deviceX, deviceY;
    windowToDeviceCoords(deltaX,  deltaY, deviceX, deviceY);

    pan[0] = basePan[0] + deviceX / zoom;
    pan[1] = basePan[1] + deviceY / zoom / aspect;
    
    updateShaderUniforms();
}

void panEventFinger(float x, float y)
{ 
    float deltaX = 0.5f + (x - fingerDownX),
          deltaY = 0.5f + (y - fingerDownY);

    float deviceX, deviceY;
    normWindowToDeviceCoords(deltaX,  deltaY, deviceX, deviceY);

    pan[0] = basePan[0] + deviceX / zoom;
    pan[1] = basePan[1] + deviceY / zoom / aspect;

    updateShaderUniforms();
}

void handleEvents()
{
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                std::terminate();
                break;

            case SDL_WINDOWEVENT:
            {
                if (event.window.windowID == windowID
                    && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    int width = event.window.data1, height = event.window.data2;
                    windowResizeEvent(width, height);
                }
                break;
            }

            case SDL_MOUSEWHEEL: 
            {
                SDL_MouseWheelEvent *m = (SDL_MouseWheelEvent*)&event;
                bool mouseWheelDown = (m->y < 0);
                zoomEventMouse(mouseWheelDown, mousePositionX, mousePositionY);
                break;
            }
            
            case SDL_MOUSEMOTION: 
            {
                SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
                mousePositionX = m->x;
                mousePositionY = m->y;
                if (mouseButtonDown && !fingerDown && !pinch)
                    panEventMouse(mousePositionX, mousePositionY);
                break;
            }

            case SDL_MOUSEBUTTONDOWN: 
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                if (m->button == SDL_BUTTON_LEFT && !fingerDown && !pinch)
                {
                    mouseButtonDown = true;
                    mouseButtonDownX = m->x;
                    mouseButtonDownY = m->y;
                    basePan[0] = pan[0]; 
                    basePan[1] = pan[1];
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: 
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                if (m->button == SDL_BUTTON_LEFT)
                    mouseButtonDown = false;
                break;
            }

            case SDL_FINGERMOTION:
                if (fingerDown)
                {
                    SDL_TouchFingerEvent *m = (SDL_TouchFingerEvent*)&event;

                    // Finger down and finger moving must match
                    if (m->fingerId == fingerDownId)
                        panEventFinger(m->x, m->y);
                }
                break;

            case SDL_FINGERDOWN:
                if (!pinch)
                {
                    // Finger already down means multiple fingers, which is handled by multigesture event
                    if (fingerDown)
                        fingerDown = false;
                    else
                    {
                        SDL_TouchFingerEvent *m = (SDL_TouchFingerEvent*)&event;

                        fingerDown = true;
                        fingerDownX = m->x;
                        fingerDownY = m->y;
                        fingerDownId = m->fingerId;
                        basePan[0] = pan[0]; 
                        basePan[1] = pan[1];
                    }
                }
                break;

            case SDL_MULTIGESTURE:
            {
                SDL_MultiGestureEvent *m = (SDL_MultiGestureEvent*)&event;
                if (m->numFingers == 2 && fabs(m->dDist) >= PINCH_ZOOM_THRESHOLD)
                {
                    pinch = true;
                    fingerDown = false;
                    mouseButtonDown = false;
                    zoomEventPinch(m->dDist, m->x, m->y);
                }
                break;
            }

            case SDL_FINGERUP:
                fingerDown = false;
                pinch = false;
                break;
        }
    }
}

void mainLoop() 
{    
    handleEvents();
    redraw();
}

int main(int argc, char** argv)
{
    // Create SDL window
    window = SDL_CreateWindow("hello_text_ttf", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE| SDL_WINDOW_SHOWN);
    windowID = SDL_GetWindowID(window);

    // Create OpenGLES2 context on SDL window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GLContext glc = SDL_GL_CreateContext(window);

    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize graphics
    initShaders();
    initGeometry();
    initTextTexture();

    // Start the main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, true);
#else
    while(true) 
        mainLoop();
#endif

    glDeleteTextures(1, &textureObj);

    return 0;
}