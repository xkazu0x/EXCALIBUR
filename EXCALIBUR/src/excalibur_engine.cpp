#include <windows.h>

struct window_info {
	int screen_width;
	int screen_height;
	const char *title;
};

struct window_state {
	HINSTANCE instance;
	HWND hwnd;
};

static bool quit;

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

int main() {
	window_info info = {};
	info.screen_width = 1920 / 2;
	info.screen_height = 1080 / 2;
	info.title = "EXCALIBUR";

	window_state state;
	state.instance = GetModuleHandleA(0);

	WNDCLASSA wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = state.instance;
	wc.hIcon = LoadIcon(state.instance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "EXCALIBUR_WINDOW_CLASS";

	if (!RegisterClassA(&wc)) {
		MessageBoxA(0, "FAILED TO REGISTER WINDOW CLASS.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return -1;
	}

	int screen_width = info.screen_width;
	int screen_height = info.screen_height;

	int xpos = (GetSystemMetrics(SM_CXSCREEN) - screen_width) / 2;
	int ypos = (GetSystemMetrics(SM_CYSCREEN) - screen_height) / 2;

	int window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	int window_ex_style = WS_EX_APPWINDOW;

	window_style |= WS_MAXIMIZEBOX;
	window_style |= WS_MINIMIZEBOX;
	window_style |= WS_THICKFRAME;

	RECT border_rect = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

	xpos += border_rect.left;
	ypos += border_rect.top;

	screen_width += border_rect.right - border_rect.left;
	screen_height += border_rect.bottom - border_rect.top;

	HWND hwnd = CreateWindowExA(
		window_ex_style, wc.lpszClassName, info.title,
		window_style, xpos, ypos, screen_width, screen_height,
		0, 0, state.instance, 0);
	if (!hwnd) {
		MessageBoxA(NULL, "FAILED TO CREATE WINDOW", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	} else {
		state.hwnd = hwnd;
	}

	ShowWindow(state.hwnd, SW_SHOW);

	while (!quit) {
		MSG message = {};
		while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	if (state.hwnd) {
		DestroyWindow(state.hwnd);
		state.hwnd = 0;
	}

	if (state.instance) {
		UnregisterClassA(wc.lpszClassName, state.instance);
		state.instance = 0;
	}

	return 0;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	LRESULT result = 0;
	switch (message) {
		case WM_CLOSE:
			quit = true;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			result = DefWindowProcA(hwnd, message, wparam, lparam);
	}
	return result;
}