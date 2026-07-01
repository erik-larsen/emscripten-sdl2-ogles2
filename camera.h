//
// Camera - pan, zoom, orbit, and window resizing
//
#include "linmath.h"

struct Rect { int width, height; };
struct Vec2 { GLfloat x, y; };

class Camera
{
public:
    Camera();
    bool updated();
    void reset();
    bool windowResized();

    Rect& windowSize() { return mWindowSize; }
    void setWindowSize (int width, int height);
    GLfloat* viewport() { return (GLfloat*)&mViewport; }

    GLfloat* pan() { return (GLfloat*)&mPan; }
    GLfloat zoom() { return mZoom; }
    GLfloat aspect() { return mAspect; }

    void setPan (Vec2 pan) { mPan = pan; mCameraUpdated = true; }
    void setPanDelta (Vec2 panDelta) { mPan.x += panDelta.x; mPan.y += panDelta.y; mCameraUpdated = true; }
    void setZoom (GLfloat zoom) { mZoom = clamp(zoom, cZoomMin, cZoomMax); mCameraUpdated = true; }
    void setZoomDelta (GLfloat zoomDelta) { mZoom = clamp(mZoom + zoomDelta, cZoomMin, cZoomMax); mCameraUpdated = true; }
    void setAspect (GLfloat aspect) { mAspect = aspect; mCameraUpdated = true; }

    Vec2& basePan() { return mBasePan; }
    void setBasePan () { mBasePan = mPan; }

    void normWindowToDeviceCoords (float normWinX, float normWinY, float& deviceX, float& deviceY);
    void windowToDeviceCoords (int winX, int winY, float& deviceX, float& deviceY);
    void deviceToWorldCoords (float deviceX, float deviceY, float& worldX, float& worldY);
    void windowToWorldCoords (int winX, int winY, float& worldX, float& worldY);
    void normWindowToWorldCoords (float normWinX, float normWinY, float& worldX, float& worldY);

    void modelViewProj (mat4x4 mvp);
    void setOrbitDelta (float dxPixels, float dyPixels);

private:
    float clamp (float val, float lo, float hi);

    bool mCameraUpdated;
    bool mWindowResized;
    Rect mWindowSize;
    Vec2 mViewport;
    const GLfloat cZoomMin, cZoomMax;
    Vec2 mBasePan, mPan;
    GLfloat mZoom, mAspect;

    // 3D orbit and perspective
    const GLfloat cOrbitSensitivity, cFov, cNear, cFar, cDistance;
    mat4x4 mOrbitMat;
};

inline Camera::Camera()
    : mCameraUpdated (false)
    , mWindowResized (false)
    , mWindowSize ({})
    , mViewport ({})
    , cZoomMin (0.1f), cZoomMax (10.0f)
    , mBasePan ({0.0f, 0.0f})
    , mPan ({0.0f, 0.0f})
    , mZoom (1.0f)
    , mAspect (1.0f)
    , cOrbitSensitivity (0.01f)
    , cFov (45.0f * M_PI / 180.0f)
    , cNear (0.01f), cFar (1000.0f)
    , cDistance (6.0f)
{
    mat4x4_identity(mOrbitMat);
    setWindowSize(640, 480);
}
