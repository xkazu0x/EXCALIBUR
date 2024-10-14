#include "platform.h"
#include "core/sxlogger.h"
#include "core/sxmemory.h"

#if EXCALIBUR_PLATFORM_WINDOWS
#include "renderer/vulkan/vk_types.h"
#include "renderer/vulkan/vk_platform.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

struct internal_state {
	HINSTANCE instance;
	HWND hwnd;
};

static platform_state *platform_pointer;

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

bool platform::initialize(platform_state *platform, platform_info *info) {
	platform->internal_state = sxmemory::allocate(sizeof(internal_state), MEMORY_TAG_PLATFORM);
	internal_state *state = static_cast<internal_state *>(platform->internal_state);
	
	SXDEBUG("INITIALIZING PLATFORM");
	state->instance = GetModuleHandleA(0);

	WNDCLASSA wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = state->instance;
	wc.hIcon = LoadIcon(state->instance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "EXCALIBUR_WINDOW_CLASS";

	if (!RegisterClassA(&wc)) {
		SXFATAL("FAILED TO REGISTER WINDOW CLASS");
		MessageBoxA(0, "FAILED TO REGISTER WINDOW CLASS", "FATAL ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	int screen_width = info->screen_width;
	int screen_height = info->screen_height;

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
		window_ex_style, wc.lpszClassName, info->title,
		window_style, xpos, ypos, screen_width, screen_height,
		0, 0, state->instance, 0);
	if (!hwnd) {
		SXFATAL("FAILED TO CREATE WINDOW");
		MessageBoxA(NULL, "FAILED TO CREATE WINDOW", "FATAL ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	} else {
		state->hwnd = hwnd;
	}

	platform->quit = false;
	ShowWindow(state->hwnd, SW_SHOW);

	platform_pointer = platform;
	SXDEBUG("PLATFORM INITIALIZED");

	return true;
}

void platform::shutdown(platform_state *platform) {
	SXDEBUG("SHUTTING DOWN PLATFORM");
	internal_state *state = static_cast<internal_state *>(platform->internal_state);
	if (state->hwnd) {
		DestroyWindow(state->hwnd);
		state->hwnd = 0;
	}
	sxmemory::free_memory(platform->internal_state, sizeof(internal_state), MEMORY_TAG_PLATFORM);
	platform_pointer = 0;
}

void platform::pump_messages(platform_state *platform) {
	MSG message = {};
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

bool platform::should_stop(platform_state *platform) {
	return platform->quit;
}

void platform::set_should_stop(platform_state *platform, bool value) {
	platform->quit = value;
}

void platform::console_write(const char *message, int color) {
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// FATAL, ERROR, WARN, INFO, DEBUG, TRACE
	static int levels[6] = { 64, 4, 6, 2, 3, 8 };
	SetConsoleTextAttribute(console_handle, levels[color]);
	OutputDebugStringA(message);
	size_t length = strlen(message);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
	SetConsoleTextAttribute(console_handle, 15);
}

void platform::console_write_error(const char *message, int color) {
	HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
	// FATAL, ERROR, WARN, INFO, DEBUG, TRACE
	static int levels[6] = { 64, 4, 6, 2, 3, 8 };
	SetConsoleTextAttribute(console_handle, levels[color]);
	OutputDebugStringA(message);
	size_t length = strlen(message);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
	SetConsoleTextAttribute(console_handle, 15);
}

void vk_platform::get_required_extension_names(std::vector<const char *> *extensions) {
	extensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

bool vk_platform::create_surface(platform_state *platform, vulkan_context *context) {
	SXDEBUG("CREATING VULKAN SURFACE");
	internal_state *state = static_cast<internal_state *>(platform->internal_state);

	VkWin32SurfaceCreateInfoKHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.hinstance = state->instance;
	surface_info.hwnd = state->hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(context->instance, &surface_info, context->allocator, &context->surface);
	if (result != VK_SUCCESS) {
		return false;
	}
	SXDEBUG("VULKAN SURFACE CREATED");
	return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	LRESULT result = 0;
	switch (message) {
		case WM_CLOSE:
			platform::set_should_stop(platform_pointer, true);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			result = DefWindowProcA(hwnd, message, wparam, lparam);
	}
	return result;
}
#endif // EXCALIBUR_PLATFORM_WINDOWS