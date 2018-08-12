//
// Camera - pan, zoom, and window resizing
//
#include <algorithm>
#include <SDL.h>
#include <SDL_opengles2.h>
#include "camera.h"

bool Camera::updated()
{
    if (mCameraUpdated)
    {
        mCameraUpdated = false;
        return true;
    }
    else
        return false;
}

void Camera::setWindowSize(int width, int height)
{
    mWindowSize.width = width;
    mWindowSize.height = height;
    setAspect(width / (float)height);
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
