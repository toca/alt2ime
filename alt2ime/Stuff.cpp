#include "Stuff.h"
#include <Windows.h>
#include <system_error>

constexpr LPCWSTR WINDOW_CLASS_NAME = L"stuff_window_class";
constexpr DWORD WM_COMMAND_LEFT_MENU  = WM_APP + 1;
constexpr DWORD WM_COMMAND_RIGHT_MENU = WM_APP + 2;
constexpr DWORD WM_COMMAND_SEND_ALT   = WM_APP + 3;

std::unordered_map<DWORD, Stuff*> Stuff::threadContext;
uint64_t Stuff::keyStates[4];
bool Stuff::ready = false;


Stuff::Stuff()
    : window(nullptr)
    , neighbor(nullptr)
    
{
}

Stuff::~Stuff()
{
}

void Stuff::Run()
{
    //register window    
    WNDCLASSEXW windowClass{};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    ATOM registerResult = ::RegisterClassExW(&windowClass);
    if (!registerResult) 
    {
        auto lastError = ::GetLastError();
        throw std::system_error(std::error_code((int)lastError, std::system_category()));
    }

    // create window
    auto instance = ::GetModuleHandleW(L"");
    this->window = ::CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"stuff-window",
        0,
        0, 0,
        0, 0,
        nullptr,
        nullptr,
        instance,
        this
    );
    if (!this->window)
    {
        auto lastError = ::GetLastError();
        throw std::system_error(std::error_code((int)lastError, std::system_category()));
    }

    //set hook
    this->neighbor = ::SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, instance, 0);
    DWORD tid = ::GetCurrentThreadId();
    Stuff::threadContext[tid] = this;
    if (!this->neighbor)
    {
        auto lastError = ::GetLastError();
        throw std::system_error(std::error_code((int)lastError, std::system_category()));
    }

    // main loop
    MSG msg{};
    while(true) 
    {
        BOOL messageRes = ::GetMessageW(&msg, this->window, 0, 0);
        if (!messageRes)
        {
            break;
        }
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
}

void Stuff::Stop()
{
    BOOL res = ::UnhookWindowsHookEx(this->neighbor);
    if (!res)
    {
        auto lastError = ::GetLastError();
        throw std::system_error(std::error_code((int)lastError, std::system_category()));
    }
    for (auto each : Stuff::threadContext) 
    {
        if (each.second == this)
        {
            Stuff::threadContext.erase(each.first);
            break;
        }
    }

    LRESULT messageRes = ::SendMessageW(this->window, WM_DESTROY, 0, 0);
    if (!messageRes)
    {
        auto lastError = ::GetLastError();
        throw std::system_error(std::error_code((int)lastError, std::system_category()));
    }
}

LRESULT Stuff::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        ::DefWindowProcW(window, message, wParam, lParam);
        return TRUE;
    case WM_COMMAND_LEFT_MENU:
        printf("left ALT -> NONCONVERT\n");
        Stuff::SendKeyboardInputs({
            //{ VK_ESCAPE, 0, 0, 0, 0 },
            //{ VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0, 0 },
            { VK_NONCONVERT, 0, 0, 0, 0 },
            { VK_NONCONVERT, 0, KEYEVENTF_KEYUP, 0, 0 }
        });
        return TRUE;
    case WM_COMMAND_RIGHT_MENU:
        printf("right ALT -> CONVERT\n");
        Stuff::SendKeyboardInputs({
            //{ VK_ESCAPE, 0, 0, 0, 0 },
            //{ VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0, 0 },
            { VK_CONVERT, 0, 0, 0, 0 },
            { VK_CONVERT, 0, KEYEVENTF_KEYUP, 0, 0 }
            });
        return TRUE;
    case WM_COMMAND_SEND_ALT:
        printf("send ALT KEY\n");
        Stuff::SendKeyboardInputs({ { VK_LMENU, 0, 0, 0, 0 }, { (WORD)wParam, 0, 0, 0, 0 } });
        return TRUE;
    default:
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
}

LRESULT Stuff::HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    DWORD tid = ::GetCurrentThreadId();
    Stuff* self = Stuff::threadContext[tid];
    LRESULT result = ::CallNextHookEx(self->neighbor, code, wParam, lParam);

    KBDLLHOOKSTRUCT* kbdInfo = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    //printf("action: %04x, vk: %04x, flag: %08x\n", wParam, kbdInfo->vkCode, kbdInfo->flags);
    if (kbdInfo->flags & LLKHF_INJECTED)
    {
        return result;
    }
    switch (wParam)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        //printf("action: %04llx, vk: %04x, flag: 0x%08x\n", wParam, kbdInfo->vkCode, kbdInfo->flags);
        // no key pressed
        if (   kbdInfo->vkCode == VK_LMENU && ( Stuff::keyStates[2] == (1ull << VK_LMENU % 64) || Stuff::keyStates[2] == 0 ) && 0 == Stuff::keyStates[0] + Stuff::keyStates[1] + Stuff::keyStates[3]
            || kbdInfo->vkCode == VK_RMENU && ( Stuff::keyStates[2] == (1ull << VK_RMENU % 64) || Stuff::keyStates[2] == 0 ) && 0 == Stuff::keyStates[0] + Stuff::keyStates[1] + Stuff::keyStates[3]
        ){
            Stuff::ready = true;
            // disable when only [ALT] pressed
            result = 1;
            //printf("block alt\n");
        }
        else
        {
            //printf("trough alt\n");
            Stuff::ready = false;   
            if (Stuff::keyStates[2] & (1ull << VK_LMENU % 64)) {
                PostMessage(self->window, WM_COMMAND_SEND_ALT, kbdInfo->vkCode, 0);
            }
        }
        Stuff::keyStates[kbdInfo->vkCode / 64] |= 1ull << (kbdInfo->vkCode % 64);
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        Stuff::keyStates[kbdInfo->vkCode / 64] &= ~(1ull << (kbdInfo->vkCode % 64));
        if (0 != Stuff::keyStates[0] + Stuff::keyStates[1] + Stuff::keyStates[2] + Stuff::keyStates[3])
        {
            break;
        }
        if (kbdInfo->vkCode == VK_LMENU && Stuff::ready)
        {
            auto r = ::PostMessageW(self->window, WM_COMMAND_LEFT_MENU, 0, 0);
        }
        else if (kbdInfo->vkCode == VK_RMENU && Stuff::ready)
        {
            ::PostMessageW(self->window, WM_COMMAND_RIGHT_MENU, 0, 0);
        }
        break;
    default:
        break;
    }

    return result;

}

void Stuff::SendKeyboardInputs(const std::vector<KEYBDINPUT>& inputs)
{
    std::vector<INPUT> keys;
    for (auto& each : inputs)
    {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki = each;
        keys.push_back(input);

    }
    ::SendInput(keys.size(), keys.data(), sizeof(INPUT));
}
