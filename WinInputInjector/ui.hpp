#ifndef _UI_HPP_
#define _UI_HPP_

#include "resource.hpp"
#include <Windows.h>
#include <CommCtrl.h>
#include <commdlg.h>

#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <format>
#include <iostream>
#include <thread>
#include <stop_token>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <memory>
#include <cstring>
#include <atomic>
#include <cmath>
#include <sstream>
#include <exception>

#include "shared_data.hpp"

#include "ui_constants.hpp"

#pragma comment(lib, "Comctl32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace ui {
	struct RegisterReturn {
		WNDCLASSEX* wndclass;
		HWND hwnd;
	};

	LRESULT CALLBACK windowproc_main(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {

			case WM_SYSCOMMAND:
			{
				if (wParam == SC_CLOSE) {
					PostQuitMessage(0);
					shared_data::sts_.request_stop();
					return 0;
				}
				break;
			}

			case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpMinMaxInfo = (LPMINMAXINFO)lParam;
				lpMinMaxInfo->ptMinTrackSize.x = 600;
				lpMinMaxInfo->ptMinTrackSize.y = 400;
				return 0;
			}

			case WM_CREATE:
			{
				return 0;
			}

			case WM_COMMAND:
			{
				int cmd = LOWORD(wParam);
				switch (cmd) {
					default:
					{
						break;
					}
				}
				return 0;
			}

			case WM_SIZE:
			{
				if (wParam == SIZE_MINIMIZED) return 0;
				KillTimer(hwnd, 9999);
				SetTimer(hwnd, 9999, 30, NULL);
				return 0;
			}

			case WM_TIMER:
			{
				return 0;
			}

			case WM_DESTROY:
			{
				PostQuitMessage(0);
				shared_data::sts_.request_stop();
				return 0;
			}

			default:
			{
				break;
			}

		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	RegisterReturn register_main_ui(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
		(void)hInstance;
		(void)hPrevInstance;
		(void)lpCmdLine;
		(void)nCmdShow;

		WNDCLASSEX* wndclass_main = new WNDCLASSEX();
		std::memset(wndclass_main, 0, sizeof(WNDCLASSEX));

		RegisterReturn ret;
		std::memset(&ret, 0, sizeof(RegisterReturn));

		wndclass_main->cbSize = sizeof(WNDCLASSEX);
		wndclass_main->style = NULL;
		wndclass_main->lpfnWndProc = windowproc_main;
		wndclass_main->cbClsExtra = NULL;
		wndclass_main->cbWndExtra = NULL;
		wndclass_main->hInstance = hInstance;
		//wndclass_main->hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
		wndclass_main->hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass_main->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wndclass_main->lpszMenuName = NULL;
		wndclass_main->lpszClassName = TEXT("MainUIWindowClass");
		//wndclass_main->hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));

		if (!RegisterClassEx(wndclass_main)) {
			shared_data::sts_.request_stop();
			return ret;
		}

		HWND hwnd = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			wndclass_main->lpszClassName,
			TEXT("WinInputInjector"),
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			CW_USEDEFAULT, CW_USEDEFAULT,
			UI_WIDTH, UI_HEIGHT,
			NULL,
			NULL,
			hInstance,
			NULL
		);

		if (hwnd == NULL) {
			shared_data::sts_.request_stop();
			return ret;
		}

		ret.wndclass = wndclass_main;
		ret.hwnd = hwnd;

		return ret;
	}

}

#endif // !_UI_HPP_