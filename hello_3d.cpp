//
// Emscripten/SDL2/OpenGLES2 sample that demonstrates 3D view manipulation using the Stanford bunny
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc -std=c++11 hello_3d.cpp events.cpp camera.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=1 -o hello_3d.html
//
// Run:
//     emrun hello_3d.html
//
// Result:
//     A colorful bunny with a white wireframe overlay.
//     Controls:
//      - Orbit with left mouse/finger drag
//      - Zoom with mouse wheel/pinch gesture
//      - Pan with right mouse/two-finger drag
//      - R to reset view
//     Window is resizable.
//

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_opengles2.h>
#include <vector>

#include "events.h"
#include "bunny_mesh.h"

//
// Full modelViewProj transform for 3D view manipulation, and a single
// shader pass colors the triangle interiors with rainbow shading
// (color derived from object space rather than screen space in hello
// triangle), and colors the triangle edges in white using a barycentric
// coordinate approach.
//

// Vertex shader
GLint shaderModelViewProj = 0;
const GLchar* vertexSource =
    "uniform mat4 modelViewProj;                                \n"
    "attribute vec4 position;                                   \n"
    "attribute vec3 barycentric;                                \n"
    "varying vec3 color;                                        \n"
    "varying vec3 bary;                                         \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_Position = modelViewProj * vec4(position.xyz, 1.0); \n"
    "    color = position.xyz * 0.3 + vec3(0.5);                \n"
    "    bary = barycentric;                                    \n"
    "}                                                          \n";

// Fragment/pixel shader
const GLchar* fragmentSource =
    "#extension GL_OES_standard_derivatives : enable                        \n"
    "precision mediump float;                                               \n"
    "varying vec3 color;                                                    \n"
    "varying vec3 bary;                                                     \n"
    "void main()                                                            \n"
    "{                                                                      \n"
    "    // Distance to nearest edge in pixels, via screen-space derivatives\n"
    "    vec3 d = fwidth(bary);                                             \n"
    "    vec3 edgeDist = bary / max(d, vec3(0.0001));                       \n"
    "    float nearestEdge = min(min(edgeDist.x, edgeDist.y), edgeDist.z);  \n"
    "                                                                       \n"
    "    // Blend from the rainbow interior to a white edge                 \n"
    "    float edge = 1.0 - smoothstep(0.5, 1.5, nearestEdge);              \n"
    "    vec3 rgb = mix(color, vec3(1.0), edge);                            \n"
    "    gl_FragColor = vec4(rgb, 1.0);                                     \n"
    "}                                                                      \n";

//
// Bunny geometry - the pre-triangulated mesh in bunny_mesh.h
//
GLuint bunnyVBO = 0;
GLsizei bunnyVertexCount = 0;
GLint posAttrib = -1, baryAttrib = -1;

void initGeometry(GLuint shaderProgram)
{
    const float scale = 1.4f;   // fit bunny nicely in view

    // Each triangle corner carries a barycentric coordinate so the shader can
    // find the triangle edges.  These can't be shared between triangles, so the
    // geometry is non-indexed: every triangle emits its 3 corners explicitly,
    // with 6 floats per vertex (position xyz + barycentric).
    const GLfloat barycentrics[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };

    std::vector<GLfloat> bunnyVerts;
    bunnyVerts.reserve(kBunnyNumTriangles * 3 * 6);

    for (int t = 0; t < kBunnyNumTriangles; ++t)
        for (int corner = 0; corner < 3; ++corner)
        {
            const float* pos = kBunnyTriangles[t * 3 + corner];
            const GLfloat* bary = barycentrics[corner];
            bunnyVerts.insert(bunnyVerts.end(),
                { pos[0] * scale, pos[1] * scale, pos[2] * scale, bary[0], bary[1], bary[2] });
        }
    bunnyVertexCount = (GLsizei)(bunnyVerts.size() / 6);

    // Create the vertex buffer object and copy the geometry into it
    glGenBuffers(1, &bunnyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
    glBufferData(GL_ARRAY_BUFFER, bunnyVerts.size() * sizeof(GLfloat), bunnyVerts.data(), GL_STATIC_DRAW);

    posAttrib = glGetAttribLocation(shaderProgram, "position");
    baryAttrib = glGetAttribLocation(shaderProgram, "barycentric");
}

void updateShader(EventHandler& eventHandler, GLuint shaderProgram)
{
    mat4x4 modelViewProj;
    eventHandler.camera().modelViewProj(modelViewProj);

    glUniformMatrix4fv(shaderModelViewProj, 1, GL_FALSE, (const GLfloat*)modelViewProj);
}

GLuint initShader()
{
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link vertex and fragment shader into shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    shaderModelViewProj = glGetUniformLocation(shaderProgram, "modelViewProj");

    return shaderProgram;
}

void redraw(GLuint shaderProgram)
{
    // Clear screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the bunny: one pass colors triangle interiors with the rainbow and
    // their edges white (see the fragment shader)
    glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
    const GLsizei stride = 6 * sizeof(GLfloat);          // position (3 floats) + barycentric (3 floats)
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    glEnableVertexAttribArray(baryAttrib);
    glVertexAttribPointer(baryAttrib, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(3 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLES, 0, bunnyVertexCount);

    glDisableVertexAttribArray(posAttrib);
    glDisableVertexAttribArray(baryAttrib);
}

GLuint gShader = 0;

void mainLoop(void* mainLoopArg)
{
    EventHandler& eventHandler = *((EventHandler*)mainLoopArg);
    eventHandler.processEvents();

    // Update shader if camera changed
    if (eventHandler.camera().updated())
        updateShader(eventHandler, gShader);

    redraw(gShader);
    eventHandler.swapWindow();
}

int main(int argc, char** argv)
{
    EventHandler eventHandler("Hello 3D", MANIP_3D);

    // Depth testing for 3D
    glEnable(GL_DEPTH_TEST);

    // Initialize shader and geometry
    gShader = initShader();
    initGeometry(gShader);
    updateShader(eventHandler, gShader);

    // Start the main loop
    void* mainLoopArg = &eventHandler;

#ifdef __EMSCRIPTEN__
    int fps = 0; // Use browser's requestAnimationFrame
    emscripten_set_main_loop_arg(mainLoop, mainLoopArg, fps, true);
#else
    while(true)
        mainLoop(mainLoopArg);
#endif

    return 0;
}
