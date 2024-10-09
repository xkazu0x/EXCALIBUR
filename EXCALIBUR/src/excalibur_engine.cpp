#include "renderer/vulkan/vk_types.h"
#include "renderer/vulkan/vk_instance.h"
#include "renderer/vulkan/vk_platform.h"
#include "renderer/vulkan/vk_device.h"

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

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

	window_state state = {};
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
		printf("FAILED TO REGISTER WINDOW CLASS\n");
		MessageBoxA(0, "FAILED TO REGISTER WINDOW CLASS", "ERROR", MB_ICONEXCLAMATION | MB_OK);
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
		printf("FAILED TO CREATE WINDOW\n");
		MessageBoxA(NULL, "FAILED TO CREATE WINDOW", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return -1;
	} else {
		state.hwnd = hwnd;
	}

	ShowWindow(state.hwnd, SW_SHOW);

	vulkan_context context = {};
	context.allocator = 0;

	if (!vk_instance::create(&context)) {
		printf("FAILED TO CREATE VULKAN INSTANCE\n");
		return -1;
	}

	if (!vk_platform::create_surface(&state, &context)) {
		printf("FAILED TO CREATE VULKAN SURFACE\n");
		return -1;
	}

	if (!vk_device::create(&context)) {
		printf("FAILED TO CREATE VULKAN DEVICE\n");
		return -1;
	}

	while (!quit) {
		MSG message = {};
		while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	vk_device::destroy(&context);

	vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

	vk_instance::destroy(&context);

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

void vk_platform::get_required_extension_names(std::vector<const char *> *extensions) {
	extensions->push_back("VK_KHR_win32_surface");
}

bool vk_platform::create_surface(window_state *state, vulkan_context *context) {
	VkWin32SurfaceCreateInfoKHR surface_info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surface_info.hinstance = state->instance;
	surface_info.hwnd = state->hwnd;
	VkResult result = vkCreateWin32SurfaceKHR(context->instance, &surface_info, context->allocator, &context->surface);
	if (result != VK_SUCCESS) {
		return false;
	}
	return true;
}