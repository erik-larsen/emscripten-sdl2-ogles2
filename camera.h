//
// Camera - pan, zoom, and window resizing
//
struct Rect { int width, height; };
struct Vec2 { GLfloat x, y; };

class Camera
{
public:
    Camera();
    bool updated();
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

private:
    float clamp (float val, float lo, float hi);

    bool mCameraUpdated;
    bool mWindowResized;
    Rect mWindowSize;
    Vec2 mViewport;  
    const GLfloat cZoomMin, cZoomMax;
    Vec2 mBasePan, mPan;
    GLfloat mZoom, mAspect; 
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
{
    setWindowSize(640, 480);
}