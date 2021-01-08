#pragma once
#include <Windows.h>
#include <unordered_map>

class Stuff
{
public:
	Stuff();
	~Stuff();

	void Run();
	void Stop();

	static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);

private:
	static void SendKeyboardInputs(const std::vector<KEYBDINPUT>& inputs);
	HWND window;
	HHOOK neighbor;
	// vk is range 1 to 254. 64 * 4 = 256
	static uint64_t keyStates[4];
	static bool ready;
	// ThreadID => *Stuff
	static std::unordered_map<DWORD, Stuff*> threadContext;
};

