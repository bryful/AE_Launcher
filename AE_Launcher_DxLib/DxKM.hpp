#pragma once
#include "DxLib.h"

struct DxKM_WIN
{
	int width = 0;
	int height = 0;
};

class DxKM
{
private:
	int m_winWidth = 0;
	int m_winHeight = 0;
	int m_mouseX = 0;
	int m_mouseY = 0;
	int m_oldMouseX = 0;
	int m_oldMouseY = 0;
	char m_KeyState[256] = { 0 };
	char m_OldKeyState[256] = { 0 };

	bool m_IsLeftMouseClicked = false;
	bool m_IsRightMouseClicked = false;
	bool m_mouseMoved = false;
public:
	int MoouseX() const { return m_mouseX; }
	int MoouseY() const { return m_mouseY; }
	bool IsLeftMouseClicked() const { return m_IsLeftMouseClicked; }
	bool IsRightMouseClicked() const { return m_IsRightMouseClicked; }
	bool IsMouseMoved() const { return m_mouseMoved; }
	int  winWidth() const { return m_winWidth; }
	int  winHeight() const { return m_winHeight; }
	void SetWinSize(int width, int height) { m_winWidth = width; m_winHeight = height; }
	DxKM(int width, int height)
		: m_winWidth(width), m_winHeight(height)
	{
	}
	DxKM()
	{}
	~DxKM()
	{
	}
	/// <summary>
	/// 起動時にマウスの左ボタンが押されている場合、離すまで待機する
	/// </summary>
	void StartWait()
	{
		GetHitKeyStateAll(m_KeyState);
		for (int i = 0; i < 256; i++) m_OldKeyState[i] = m_KeyState[i];
		while ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
			if (ProcessMessage() != 0) break;
		}
		while ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0) {
			if (ProcessMessage() != 0) break;
		}
		m_IsLeftMouseClicked = false;
		m_IsRightMouseClicked = false;
	}
	void Update()
	{
		for (int i = 0; i < 256; i++) m_OldKeyState[i] = m_KeyState[i];
		GetHitKeyStateAll(m_KeyState);
		m_oldMouseX = m_mouseX;
		m_oldMouseY = m_mouseY;
		m_mouseMoved = false;
		m_IsLeftMouseClicked = false;
		m_IsRightMouseClicked = false;
		GetMousePoint(&m_mouseX, &m_mouseY);
		if (m_mouseX >= 0 && m_mouseX < m_winWidth && m_mouseY >= 0 && m_mouseY < m_winHeight)
		{
			if (m_oldMouseX != m_mouseX || m_oldMouseY != m_mouseY) {
				m_oldMouseX = m_mouseX;
				m_oldMouseY = m_mouseY;
				m_mouseMoved = true;
			}
			if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
				m_IsLeftMouseClicked = true;
			}
			if ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0) {
				m_IsRightMouseClicked = true;
			}
		}
	}
	bool IsKeyPressed(int key) const
	{
		return m_KeyState[key] != 0 && m_OldKeyState[key] == 0;
	}
};

