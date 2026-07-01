//
// Camera - pan, zoom, and window resizing
//
#include <algorithm>
#include <SDL.h>
#include <SDL_opengles2.h>
#include "camera.h"

bool Camera::updated()
{
    bool updated = mCameraUpdated;
    mCameraUpdated = false;
    return updated;
}

void Camera::reset()
{
    mat4x4_identity(mOrbitMat);
    mPan = { 0.0f, 0.0f };
    mZoom = 1.0f;
    mCameraUpdated = true;
}

bool Camera::windowResized()
{
    bool resized = mWindowResized;
    mWindowResized = false;
    return resized;
}

void Camera::setWindowSize(int width, int height)
{
    if (mWindowSize.width != width || mWindowSize.height != height)
    {
        mWindowResized = true;
        mWindowSize = {width, height};
        mViewport = {(float)width, (float)height};
        setAspect(width / (float)height);
    }
}

// Clamp val between lo and hi
float Camera::clamp (float val, float lo, float hi)
{
    return std::max(lo, std::min(val, hi));
}

// Convert from normalized window coords (x,y) in ([0.0, 1.0], [1.0, 0.0]) to device coords ([-1.0, 1.0], [-1.0,1.0])
void Camera::normWindowToDeviceCoords (float normWinX, float normWinY, float& deviceX, float& deviceY)
{
    deviceX = (normWinX - 0.5f) * 2.0f;
    deviceY = (1.0f - normWinY - 0.5f) * 2.0f;
}

// Convert from window coords (x,y) in ([0, mWindowWidth], [mWindowHeight, 0]) to device coords ([-1.0, 1.0], [-1.0,1.0])
void Camera::windowToDeviceCoords (int winX, int winY, float& deviceX, float& deviceY)
{
    normWindowToDeviceCoords(winX / (float)mWindowSize.width,  winY / (float)mWindowSize.height, deviceX, deviceY);
}

// Convert from device coords ([-1.0, 1.0], [-1.0,1.0]) to world coords ([-inf, inf], [-inf, inf])
void Camera::deviceToWorldCoords (float deviceX, float deviceY, float& worldX, float& worldY)
{
    worldX = deviceX / mZoom - mPan.x;
    worldY = deviceY / mAspect / mZoom - mPan.y;
}

// Convert from window coords (x,y) in ([0, windowWidth], [windowHeight, 0]) to world coords ([-inf, inf], [-inf, inf])
void Camera::windowToWorldCoords(int winX, int winY, float& worldX, float& worldY)
{
    float deviceX, deviceY;
    windowToDeviceCoords(winX, winY, deviceX, deviceY);
    deviceToWorldCoords(deviceX, deviceY, worldX, worldY);
}

// Convert from normalized window coords (x,y) in in ([0.0, 1.0], [1.0, 0.0]) to world coords ([-inf, inf], [-inf, inf])
void Camera::normWindowToWorldCoords(float normWinX, float normWinY, float& worldX, float& worldY)
{
    float deviceX, deviceY;
    normWindowToDeviceCoords(normWinX, normWinY, deviceX, deviceY);
    deviceToWorldCoords(deviceX, deviceY, worldX, worldY);
}

// Build a perspective model-view-projection matrix from pan, zoom, orbit, and aspect.
// Mirrors the 2D samples' vertex shader transform, clip = (position + pan) * zoom,
// so the shared pan/zoom event handling behaves the same in 3D:
// view = translate(-distance) * scale(zoom) * translate(pan) * orbit
void Camera::modelViewProj (mat4x4 mvp)
{
    mat4x4 projMat;
    mat4x4_perspective(projMat, cFov, mAspect, cNear, cFar);

    // Scale pan from device coords to world units on the view plane at the orbit
    // center, so panning tracks the mouse/finger exactly, as in the 2D samples
    float panScale = cDistance * mAspect * tanf(cFov * 0.5f);

    mat4x4 viewMat;
    mat4x4_identity(viewMat);
    mat4x4_translate_in_place(viewMat, 0.0f, 0.0f, -cDistance);

    mat4x4 scaleMat;
    mat4x4_identity(scaleMat);
    mat4x4_scale_iso(scaleMat, scaleMat, mZoom);
    mat4x4_mul(viewMat, viewMat, scaleMat);

    mat4x4_translate_in_place(viewMat, mPan.x * panScale, mPan.y * panScale, 0.0f);

    mat4x4_mul(viewMat, viewMat, mOrbitMat);
    mat4x4_mul(mvp, projMat, viewMat);
}

// Orbit the camera by an xy pixel delta
void Camera::setOrbitDelta (float dxPixels, float dyPixels)
{
    vec2 dragStart = { 0.0f, 0.0f };
    vec2 dragVec = { dxPixels * cOrbitSensitivity, -dyPixels * cOrbitSensitivity };

    mat4x4 identityMat;
    mat4x4_identity(identityMat);

    mat4x4 deltaRot;
    mat4x4_arcball(deltaRot, identityMat, dragStart, dragVec, 1.0f);

    // Pre-multiply so the orbit is relative to the screen, not the model
    mat4x4 newRot;
    mat4x4_mul(newRot, deltaRot, mOrbitMat);
    mat4x4_dup(mOrbitMat, newRot);

    mCameraUpdated = true;
}
