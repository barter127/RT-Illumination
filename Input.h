#ifndef INPUT_H
#define INPUT_H

#include <DirectXMath.h>
#include <array>

static class Input
{

public:
	Input();
	~Input();

	static bool GetKeyDown(unsigned int key);
	static bool GetKeyHeld(unsigned int key);
	static bool GetKeyUp(unsigned int key);

	static bool GetMouseLButtonDown();
	static bool GetMouseLButtonHeld();
	static bool GetMouseLButtonUp();

	static bool GetMouseRButtonDown();
	static bool GetMouseRButtonHeld();
	static bool GetMouseRButtonUp();
	static DirectX::XMFLOAT2 GetMousePosition();

private:
	static const int NUM_OF_KEYS = 256;

	static std::array<bool, NUM_OF_KEYS> m_lastKeyState;
};

#endif // !INPUT_H