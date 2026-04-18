#include "Input.h"
#include <windows.h>

using namespace DirectX;

std::array<bool, Input::NUM_OF_KEYS> Input::m_lastKeyState;

Input::Input() {}

Input::~Input() {}

bool Input::GetKeyDown(unsigned int key)
{
    SHORT keyState = GetKeyState(key);

    bool isPressed = keyState & (1 << 15); // Most significant bit holds if key is down
    bool wasPressed = m_lastKeyState[key];

    bool returnVal = isPressed && !wasPressed;

    m_lastKeyState[key] = isPressed;

    return returnVal;
}

bool Input::GetKeyHeld(unsigned int key)
{
    return GetAsyncKeyState(key);
}

bool Input::GetKeyUp(unsigned int key)
{
    SHORT keyState = GetKeyState(key);

    bool isPressed = keyState & (1 << 15); // Most significant bit holds if key is down
    bool wasPressed = m_lastKeyState[key];

    bool returnVal = !isPressed && wasPressed;

    m_lastKeyState[key] = isPressed;

    return returnVal;
}

bool Input::GetMouseLButtonDown()
{
    return GetKeyDown(VK_LBUTTON);
}

bool Input::GetMouseLButtonHeld()
{
    return GetKeyHeld(VK_LBUTTON);
}

bool Input::GetMouseLButtonUp()
{
    return GetKeyUp(VK_LBUTTON);
}

bool Input::GetMouseRButtonDown()
{
    return GetKeyDown(VK_RBUTTON);
}

bool Input::GetMouseRButtonHeld()
{
    return GetKeyHeld(VK_RBUTTON);
}

bool Input::GetMouseRButtonUp()
{
    return GetKeyUp(VK_RBUTTON);
}

XMFLOAT2 Input::GetMousePosition()
{
    POINT point;
    GetCursorPos(&point);

    return XMFLOAT2(point.x, point.y);
}