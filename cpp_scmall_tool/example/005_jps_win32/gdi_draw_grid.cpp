#include "jps.h"

#include <windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawGrid(HWND hWnd, int);
// void CalcTarget();
void CalcTarget(HWND hWnd, int x, int y, UINT key);
void OnTimer(HWND hwnd, UINT id);

const int width = 800;
const int height = 800;

const int offset_top = 80, offset_bottom = 60, offset_left = 20, offset_right = 30;
const int rect_width = 20, rect_height = 20;

/* Key state */
int press_vk = 0;
/* Selected popup menu object */
int Select_Popup_Menu = 1;
/* Animation state */
int animation = 0;
/* Single-step state */
int StepInfo = 0;
/* Path state */
int PathStatus = 0;

POINT point;
HMENU hMenu;

#define IDM_START 1
#define IDM_GOAL 2
#define IDM_WALLS 3
#define IDM_SETNONE 4
#define IDM_CLEAN 5
#define IDM_RUN 6

static int status = 0;

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
	/* int main() */
{
	MSG msg;
	WNDCLASS wc = { 0 };

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpszClassName = L"jps";
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH); // GetSysColorBrush(COLOR_3DFACE);
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClass(&wc);
	HWND hWnd = CreateWindow(wc.lpszClassName, L"JPS Pathfinding Demo",
		(WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_SIZEBOX,
		50, 50,
		width, height,
		NULL, NULL, hInstance /*NULL*/, NULL
	);
	// ShowWindow(hWnd, nCmdShow);
	// UpdateWindow(hWnd);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	press_vk = 1;
	CalcTarget(hwnd, x, y, keyFlags);
}

#include <windowsx.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch (msg)
	{
		HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE, CalcTarget);
		//     case WM_MOUSEMOVE:
		//         if (wParam & WM_LBUTTONDOWN)
		//         {
		//             CalcTarget(hWnd, point.x, point.y, 0);
		//         }
		//         break;
	case WM_LBUTTONUP:
		press_vk = 0;
		break;
	case WM_CREATE:
		SetTimer(hWnd, 0, 1, NULL);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_START:
			Select_Popup_Menu = IDM_START;
			break;
		case IDM_GOAL:
			Select_Popup_Menu = IDM_GOAL;
			break;
		case IDM_WALLS:
			Select_Popup_Menu = IDM_WALLS;
			break;
		case IDM_SETNONE:
			Select_Popup_Menu = IDM_SETNONE, StepInfo = 0, dir_event.clear(), path.clear();
			break;
		case IDM_CLEAN:
			start = goal = NoneLoc, StepInfo = 0, dir_event.clear(), GridWalls.clear(), path.clear();
			break;
		case IDM_RUN:
			status = 1; StepInfo = 0, dir_event.clear(), path.clear();
			break;
		default:
			break;
		}
		break;
	case WM_RBUTTONUP:
		point = { LOWORD(lParam), HIWORD(lParam) };
		hMenu = CreatePopupMenu();
		ClientToScreen(hWnd, &point);

		AppendMenu(hMenu, MF_STRING, IDM_START, L"Start");
		AppendMenu(hMenu, MF_STRING, IDM_GOAL, L"Goal");
		AppendMenu(hMenu, MF_STRING, IDM_WALLS, L"Wall");
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, IDM_RUN, L"Run");
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, IDM_SETNONE, L"Clear");
		AppendMenu(hMenu, MF_STRING, IDM_CLEAN, L"Reset");

		for (int i = IDM_START; i <= IDM_SETNONE; ++i)
		{
			if (i == Select_Popup_Menu)
			{
				CheckMenuItem(hMenu, Select_Popup_Menu, MF_CHECKED);
			}
			else
				CheckMenuItem(hMenu, i, MF_UNCHECKED);
		}
		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);

		break;

	case WM_PAINT:
		DrawGrid(hWnd, 0);
		break;
	case WM_ERASEBKGND:
		press_vk = 0;
		return (LRESULT)1;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void DrawTxt(HDC hdc, DWORD fontSize, DWORD x, DWORD y, std::string info, DWORD TextColor) {
	// PAINTSTRUCT ps;
	// HDC hdc = GetDC(hwnd);
	DWORD color = RGB(192, 192, 192);
	SetBkColor(hdc, color);
	// SetBkMode(hdc, TRANSPARENT);
	// HFONT hFont = CreateFontW(15, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"SimSun");
	HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"SimSun");
	HFONT holdFont = (HFONT)SelectObject(hdc, hFont);
	SetTextColor(hdc, TextColor);
	TextOutA(hdc, x, y, info.c_str(), info.length());
	SelectObject(hdc, holdFont);

	DeleteObject(hFont);
	// DeleteObject(holdFont);
	// ReleaseDC(hwnd, hdc);
	// EndPaint(hwnd, &ps);
}

void OnTimer(HWND hwnd, UINT id)
{
	DrawGrid(hwnd, 1);
}

void DrawTarget(HDC hdc) {

	/* Clear walls */
	// GridWalls.clear();

	HPEN hPen = CreatePen(PS_NULL, 1, RGB(0, 0, 0));
	HPEN HoldPen = (HPEN)SelectObject(hdc, hPen);

	HBRUSH hBrush1 = CreateSolidBrush(RGB(0, 255, 0));
	HBRUSH hBrush2 = CreateSolidBrush(RGB(0, 0, 255));
	HBRUSH hBrush3 = CreateSolidBrush(RGB(80, 80, 80));

	// SelectObject(hdc, hBrush1);
	// Rectangle(hdc, 40, 40, 100, 110);
	// SelectObject(hdc, hBrush1);
	if (start != NoneLoc)
	{
		SelectObject(hdc, hBrush1);
		Rectangle(hdc,
			start.x * rect_width + offset_left + 1,
			start.y * rect_height + offset_top + 1,
			start.x * rect_width + offset_left + rect_width + 1,
			start.y * rect_height + rect_height + offset_top + 1);
	}
	if (goal != NoneLoc)
	{
		SelectObject(hdc, hBrush2);
		Rectangle(hdc,
			goal.x * rect_width + offset_left + 1,
			goal.y * rect_height + offset_top + 1,
			goal.x * rect_width + offset_left + rect_width + 1,
			goal.y * rect_height + rect_height + offset_top + 1);
	}
	for (auto &i : GridWalls)
	{
		SelectObject(hdc, hBrush3);
		Rectangle(hdc,
			i.x * rect_width + offset_left + 1,
			i.y * rect_height + offset_top + 1,
			i.x * rect_width + offset_left + rect_width + 1,
			i.y * rect_height + rect_height + offset_top + 1);
	}

	SelectObject(hdc, HoldPen);
	if (DeleteObject(hPen))
		; // cout << "DeleteObject(hPen) ok" << "\n";
	else
		; // cout << "error" << "\n";
	if (DeleteObject(hBrush1))
		; // cout << "DeleteObject(hBrush1) ok" << "\n";
	else
		; // cout << "error" << "\n";
	if (DeleteObject(hBrush2))
		; // cout << "DeleteObject(hBrush2) ok" << "\n";
	else
		; // cout << "error" << "\n";
	if (DeleteObject(hBrush3))
		; // cout << "DeleteObject(hBrush3) ok" << "\n";
	else
		; // cout << "error" << "\n";
	// DeleteDC(hdc);
}

void DrawAnimation(HDC hdc) {

	if (dir_event.size())
	{
		for (auto i = 0; i < StepInfo; ++i)
		{
			if (start != Location{ dir_event[i].first.x, dir_event[i].first.y } && goal != Location{ dir_event[i].first.x, dir_event[i].first.y })
				DrawTxt(hdc, 16, dir_event[i].first.x * rect_width + offset_left + 2,
					dir_event[i].first.y * rect_height + offset_top + 2, dir_event[i].second, RGB(0, 0, 0));
		}
		if (StepInfo < dir_event.size())
		{
			StepInfo++;
		}
		else
			DrawTxt(hdc, 20, 20, 20, "Traversal complete", RGB(0, 0, 0));
	}
}

static DWORD SetBit = 0;

void Draw_Path(HDC hdc) {
	if (dir_event.size() && StepInfo >= dir_event.size() && path.size())
	{
		int i = 0;
		Location cur = start;
		do
		{
			Location direction = (path[i] - cur).direction();
			cur = cur + direction;
			if (cur != goal)
			{
				SetBit ^= 1;
				DrawTxt(hdc, 16, cur.x * rect_width + offset_left + 2, cur.y * rect_height + offset_top + 2,
					find_str(direction), SetBit ? RGB(0, 255, 255) : RGB(255, 255, 0));
			}
			if (cur == path[i])
			{
				i++;
			}
		} while (i < path.size());

		//         for (auto &i : path)
		//         {
		//             DrawTxt(hdc, 8, i.x*rect_width + offset_left + 2, i.y*rect_height + offset_top + 2, "J", RGB(255, 255, 255));
		//         }
	}
}

void DrawGrid(HWND hWnd, int bit_s)
{
	HDC hdc;
	PAINTSTRUCT ps;

	hdc = GetDC(hWnd); // , &ps);
	// Create memory DC
	HDC DC = CreateCompatibleDC(hdc);
	RECT rc;
	GetClientRect(hWnd, &rc);

	HBITMAP hbmMem, hbmOld;
	// Create a bitmap memory space
	hbmMem = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom);

	// Assign the bitmap memory space to the memory DC
	hbmOld = (HBITMAP)SelectObject(DC, hbmMem);
	HBRUSH hbrBKGnd = CreateSolidBrush(RGB(192, 192, 192));
	SelectBitmap(DC, hbmMem);

	FillRect(DC, &rc, hbrBKGnd);

	DeleteObject(hbrBKGnd);
	// DeleteObject(hbmMem);
	HPEN hPen1 = CreatePen(PS_SOLID, 0, RGB(64, 64, 64));
	(HPEN)SelectObject(DC, hPen1);

	GridHeight = (height - offset_top - offset_bottom) / rect_height;
	GridWidth = (width - offset_left - offset_right) / rect_width;
	for (int i = 0; i < (height - offset_top - offset_bottom) / rect_height + 1; ++i)
	{
		MoveToEx(DC, offset_left, i * rect_height + offset_top, NULL);
		LineTo(DC, width - offset_right, i * rect_height + offset_top);
	}
	for (int i = 0; i < (width - offset_left - offset_right) / rect_width + 1; ++i)
	{
		MoveToEx(DC, i * rect_width + offset_left, offset_top, NULL);
		LineTo(DC, i * rect_width + offset_left, height - offset_bottom);
	}

	// SelectObject(DC, holdPen);
	DeleteObject(hPen1);
	// DeleteObject(holdPen);

	DrawTarget(DC);
	DrawAnimation(DC);
	Draw_Path(DC);
	if (status == 1 && bit_s)
	{
		dir_event.clear();
		DrawTxt(hdc, 20, 20, 20, "Computing...", RGB(0, 0, 0));
		clock_t st = clock();
		jps_main();
		double r = (double)(clock() - st) / CLOCKS_PER_SEC;
		// InvalidateRect(hwnd, &rt, 0);
		Sleep(500);
		DrawTxt(hdc, 20, 20, 20, (std::string("Done, elapsed: ") + std::to_string(r) + " s"), RGB(0, 0, 0));
		Sleep(500);
		status = 0;
	}
	// Copy the memory DC content to the screen DC to finish drawing
	BitBlt(hdc, rc.left, rc.top, rc.right, rc.bottom, DC, 0, 0, SRCCOPY);
	SelectObject(DC, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(DC);
	DeleteDC(hdc);
	EndPaint(hWnd, &ps);
}

void CalcTarget(HWND hWnd, int x, int y, UINT key) {
	if (press_vk == 0 || x - offset_left < 0 || y - offset_top < 0)
		return;

	Location id = { (x - offset_left) / rect_width,  (y - offset_top) / rect_height };

	if (id.x >= 0 && id.x < (width - offset_left - offset_right) / rect_width &&
		id.y >= 0 && id.y < (height - offset_top - offset_bottom) / rect_height) // boundary
		switch (Select_Popup_Menu)
		{
		case IDM_START:
			if (id != goal && !GridWalls.count(id))
			{
				start = id;
			}
			break;
		case IDM_GOAL:
			if (id != start && !GridWalls.count(id))
			{
				goal = id;
			}
			break;
		case IDM_WALLS:
		{
			if (id != start && id != goal)
				GridWalls.insert(id);
		}
		break;
		case IDM_SETNONE:
		{
			if (id == start)
			{
				start = NoneLoc;
			}
			else if (id == goal)
			{
				goal = NoneLoc;
			}
			else {
				auto it = GridWalls.find(id);
				if (it != GridWalls.end())
					GridWalls.erase(it);
			}
		}
		break;
		}
	else if (id.x < 0 || id.x > width || id.y < 0 || id.y < height)
		press_vk = 0;
}