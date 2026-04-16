#include "framework.hpp"
#include "ui.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	ui::RegisterReturn rrt = ui::register_main_ui(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	HWND hwnd = rrt.hwnd;

	if (shared_data::sts_.stop_requested() || (hwnd == nullptr)) { shared_data::sts_.request_stop(); return 1; }

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg{ 0 };
	while (GetMessage(&msg, NULL, 0, 0) && (!shared_data::sts_.stop_requested())) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}