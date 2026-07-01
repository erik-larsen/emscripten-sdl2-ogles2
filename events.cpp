//
// Window and input event handling
//
#include <algorithm>
#include <SDL.h>
#include <SDL_opengles2.h>
#include "events.h"

// #define EVENTS_DEBUG

void EventHandler::windowResizeEvent(int width, int height)
{
    glViewport(0, 0, width, height);
    mCamera.setWindowSize(width, height);
}

void EventHandler::initWindow(const char* title)
{
    // Create SDL window
    mpWindow =
        SDL_CreateWindow(title,
                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         mCamera.windowSize().width, mCamera.windowSize().height,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE| SDL_WINDOW_SHOWN);
    mWindowID = SDL_GetWindowID(mpWindow);

    // Create OpenGLES 2 context on SDL window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GLContext glc = SDL_GL_CreateContext(mpWindow);

    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize viewport
    windowResizeEvent(mCamera.windowSize().width, mCamera.windowSize().height);
}

void EventHandler::swapWindow()
{
    SDL_GL_SwapWindow(mpWindow);
}

void EventHandler::zoomEventMouse(bool mouseWheelDown, int x, int y)
{
    if (mManipMode == MANIP_2D)
    {
        float preZoomWorldX, preZoomWorldY;
        mCamera.windowToWorldCoords(mMousePositionX, mMousePositionY, preZoomWorldX, preZoomWorldY);

        // Zoom by scaling up/down in 0.05 increments
        float zoomDelta = mouseWheelDown ? -cMouseWheelZoomDelta : cMouseWheelZoomDelta;
        mCamera.setZoomDelta(zoomDelta);

        // Zoom to point: Keep the world coords under mouse position the same before and after the zoom
        float postZoomWorldX, postZoomWorldY;
        mCamera.windowToWorldCoords(mMousePositionX, mMousePositionY, postZoomWorldX, postZoomWorldY);
        Vec2 deltaWorld = { postZoomWorldX - preZoomWorldX, postZoomWorldY - preZoomWorldY };
        mCamera.setPanDelta (deltaWorld);
    }
    else
    {
        // In 3D mode, zoom about the view center (TODO: add zoom to mouse x,y as in 2D)
        mCamera.setZoomDelta(mouseWheelDown ? -cMouseWheelZoomDelta : cMouseWheelZoomDelta);
    }

}

void EventHandler::zoomEventPinch (float pinchDist, float pinchX, float pinchY)
{
    switch (mManipMode)
    {
        case MANIP_2D:
        {
            float preZoomWorldX, preZoomWorldY;
            mCamera.normWindowToWorldCoords(pinchX, pinchY, preZoomWorldX, preZoomWorldY);

            // Zoom in/out by positive/negative mPinch distance
            float zoomDelta = pinchDist * cPinchScale;
            mCamera.setZoomDelta(zoomDelta);

            // Zoom to point: Keep the world coords under pinch position the same before and after the zoom
            float postZoomWorldX, postZoomWorldY;
            mCamera.normWindowToWorldCoords(pinchX, pinchY, postZoomWorldX, postZoomWorldY);
            Vec2 deltaWorld = { postZoomWorldX - preZoomWorldX, postZoomWorldY - preZoomWorldY };
            mCamera.setPanDelta (deltaWorld);
        }
        break;
        case MANIP_3D:
            // In 3D mode, zoom about the view center (TODO: add zoom to mouse x,y as in 2D)
            mCamera.setZoomDelta(pinchDist * cPinchScale);
        break;
    }
}

void EventHandler::panEventMouse(int x, int y)
{
     int deltaX = mCamera.windowSize().width / 2 + (x - mPanMouseButtonDownX),
         deltaY = mCamera.windowSize().height / 2 + (y - mPanMouseButtonDownY);

    float deviceX, deviceY;
    mCamera.windowToDeviceCoords(deltaX,  deltaY, deviceX, deviceY);

    Vec2 pan = { mCamera.basePan().x + deviceX / mCamera.zoom(),
                 mCamera.basePan().y + deviceY / mCamera.zoom() / mCamera.aspect() };
    mCamera.setPan(pan);
}

void EventHandler::panEventFinger(float x, float y)
{
    float deltaX = 0.5f + (x - mFingerDownX),
          deltaY = 0.5f + (y - mFingerDownY);

    float deviceX, deviceY;
    mCamera.normWindowToDeviceCoords(deltaX,  deltaY, deviceX, deviceY);

    Vec2 pan = { mCamera.basePan().x + deviceX / mCamera.zoom(),
                 mCamera.basePan().y + deviceY / mCamera.zoom() / mCamera.aspect() };
    mCamera.setPan(pan);
}

void EventHandler::orbitEventFinger(float x, float y)
{
    mCamera.setOrbitDelta(x * mCamera.windowSize().width,
                          y * mCamera.windowSize().height);
}

void EventHandler::processEvents()
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

            case SDL_KEYDOWN:
                // Reset view
                if (event.key.keysym.scancode == SDL_SCANCODE_R)
                    mCamera.reset();
                break;

            case SDL_WINDOWEVENT:
            {
                if (event.window.windowID == mWindowID
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
            	#ifdef EVENTS_DEBUG
                	printf ("SDL_MOUSEWHEEL= x,y=%d,%d preciseX,preciseY=%f,%f\n", m->x, m->y, m->preciseX, m->preciseY);
            	#endif
            	bool mouseWheelDown = (m->preciseY < 0.0);
            	zoomEventMouse(mouseWheelDown, mMousePositionX, mMousePositionY);
            	break;
            }

            case SDL_MOUSEMOTION:
            {
                SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
                mMousePositionX = m->x;
                mMousePositionY = m->y;

                if (mPanMouseButtonDown && !mFingerDown && !mPinch)
                    panEventMouse(mMousePositionX, mMousePositionY);

                if (mManipMode == MANIP_3D && (m->state & SDL_BUTTON_LMASK)
                    && m->which != SDL_TOUCH_MOUSEID && !mFingerDown && !mPinch)
                    mCamera.setOrbitDelta((float)m->xrel, (float)m->yrel);

                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;

                // Pan is left button in 2D, right button in 3D
                Uint8 panButton = (mManipMode == MANIP_2D ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
                if (m->button == panButton && !mFingerDown && !mPinch)
                {
                    mPanMouseButtonDown = true;
                    mPanMouseButtonDownX = m->x;
                    mPanMouseButtonDownY = m->y;
                    mCamera.setBasePan();
                }
                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                Uint8 panButton = (mManipMode == MANIP_2D ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
                if (m->button == panButton)
                    mPanMouseButtonDown = false;
                break;
            }

            case SDL_FINGERMOTION:
                if (mFingerDown)
                {
                    SDL_TouchFingerEvent *m = (SDL_TouchFingerEvent*)&event;

                    // Finger down and finger moving must match
                    if (m->fingerId == mFingerDownId)
                    {
                        switch (mManipMode)
                        {
                            case MANIP_2D: panEventFinger(m->x, m->y);   break;
                            case MANIP_3D: orbitEventFinger(m->dx, m->dy); break;
                        }
                    }
                }
                break;

            case SDL_FINGERDOWN:
                if (!mPinch)
                {
                    // Finger already down means multiple fingers, which is handled by multigesture event
                    if (mFingerDown)
                        mFingerDown = false;
                    else
                    {
                        SDL_TouchFingerEvent *m = (SDL_TouchFingerEvent*)&event;

                        mFingerDown = true;
                        mFingerDownX = m->x;
                        mFingerDownY = m->y;
                        mFingerDownId = m->fingerId;
                        mCamera.setBasePan();
                    }
                }
                break;

            case SDL_MULTIGESTURE:
            {
                SDL_MultiGestureEvent *m = (SDL_MultiGestureEvent*)&event;
                if (m->numFingers == 2 && fabs(m->dDist) >= cPinchZoomThreshold)
                {
                    mPinch = true;
                    mFingerDown = false;
                    mPanMouseButtonDown = false;
                    zoomEventPinch(m->dDist, m->x, m->y);
                }
                break;
            }

            case SDL_FINGERUP:
                mFingerDown = false;
                mPinch = false;
                break;
        }

        #ifdef EVENTS_DEBUG
            printf ("event=%d mousePos=%d,%d mouseButtonDown=%d fingerDown=%d pinch=%d aspect=%f window=%dx%d\n",
                    event.type, mMousePositionX, mMousePositionY, mPanMouseButtonDown, mFingerDown, mPinch, mCamera.aspect(), mCamera.windowSize().width, mCamera.windowSize().height);
            printf ("    zoom=%f pan=%f,%f\n", mCamera.zoom(), mCamera.pan()[0], mCamera.pan()[1]);
        #endif
    }
}
