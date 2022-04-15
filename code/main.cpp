#include <stdio.h>
#include <windows.h>

#include <assert.h> 

//#pragma comment(lib, "user32.lib");

typedef unsigned char c8;
typedef unsigned int u32;
typedef signed	 int i32;
typedef unsigned long int u64;
typedef signed	 long int i64;

struct String {
	c8 	*base;
	u64  count;
};
struct PlatformCharacter
{
	u32  code;
	bool is_character;
};

// size of the whole array devided by the size of one element
#define array_count(static_array) (sizeof(static_array)/ sizeof(*(static_array)))
#define build_string(static_string) { (c8 *) static_string, array_count(static_string) }

void insert(String *text, u32 *text_cursor, u64 max_count, c8 character) {
	if (text->count < max_count) {

		// destination : 1 to the right of the current cursor position
		// source      : Position of the cursor
		// memory	   : total text minus the cursor position
		if(*text_cursor < text->count)
			memcpy(text->base + *text_cursor + 1, text->base + *text_cursor, text->count - *text_cursor); // xxx in bigger files

		text->base[*text_cursor] = character;
		text->count    += 1;
		(*text_cursor) += 1;
	}
}

void remove(String *text, u32 *text_cursor, u64 max_count, u64 remove_count) {
	if (*text_cursor >= remove_count && *text_cursor + remove_count <= text->count) {
		// destination : 1 to the left of the current cursor position
		// source      : Position of the cursor
		// memory	   : total text minus the cursor position
		memcpy(text->base + *text_cursor - remove_count, text->base + *text_cursor, text->count - *text_cursor); // xxx in bigger files
		
		text->count    -= 1;
		(*text_cursor) -= remove_count;
	}
}

void push_char(PlatformCharacter *platform_characters, u32 *platform_character_count, u64 max_character_count, u32 code, bool is_character) {
	if (*platform_character_count < max_character_count) {
		PlatformCharacter* character = &platform_characters[(*platform_character_count)++]; // xxx copy
		character->code 		= code;
		character->is_character = is_character;
	}
}

LRESULT  win32_window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	switch (message) {
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;
	}

	auto result = DefWindowProcA(window_handle, message, w_param, l_param);

	return result;
}

int main()
{
	printf("Hello World!");

	HINSTANCE win32_instance = (HINSTANCE) GetModuleHandleA(NULL);
	
	WNDCLASSA window_class = {};
	window_class.lpszClassName = "Kood";
	window_class.hInstance 	   = win32_instance;
	window_class.lpfnWndProc   = (WNDPROC)win32_window_callback;

	RegisterClassA(&window_class);

	HWND window_handle = CreateWindowExA(
		0, window_class.lpszClassName, "Kood" ,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		640, 480, NULL, NULL, win32_instance, 0);

	if (!window_handle) {
		printf("Could not create window\n");
		return -1;
	}

	ShowWindow(window_handle, SW_SHOW);

	PlatformCharacter platform_characters[32];
	u32 platform_character_count = 0;

	unsigned char text_buffer[1024];

	String text;
	text.base  		= text_buffer;
	text.count 		= 0;
	u32 text_cursor = 0;

	bool running = true; 

	while (running)	{
		MSG msg;
		while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {

			switch (msg.message) {
				case WM_QUIT: {
					running = false;
				} break;
				case WM_CHAR:
				{
					push_char(platform_characters, &platform_character_count, array_count(platform_characters), msg.wParam, true);	
					InvalidateRect(window_handle, NULL, TRUE); // xxx forces the window to be redrawn
					
				} break;

				case WM_KEYDOWN:
				{
					push_char(platform_characters, &platform_character_count, array_count(platform_characters), msg.wParam, false);
					InvalidateRect(window_handle, NULL, TRUE); // xxx forces the window to be redrawn
					
				} break;

				case WM_PAINT:
				{
					for (u32 index = 0; index < platform_character_count; index++) {
						PlatformCharacter character = platform_characters[index];

						printf("char 1: %i %c \n", (int)character.code, (char)character.code);

						if (character.is_character) {
							switch (character.code)
							{
								case 8: // backspace
								{
									remove(&text, &text_cursor, array_count(text_buffer), 1);
								} break;

								case 13: // new line
								{
									insert(&text, &text_cursor, array_count(text_buffer), '\n');
								} break;

								default:
								{
									if ((character.code >= ' ') && (character.code <= 127)) {
										insert(&text, &text_cursor, array_count(text_buffer), character.code);
									}
								}
							}
						}
						else {
							switch (character.code)
							{
								case VK_HOME: 
								{
									i32 index = text_cursor;
									while (index != 0) {
										if((char)text.base[index-1] == '\n')
											break;

										index--;
									}

									text_cursor = index;

								} break;
								case VK_END: 
								{
									i32 index = text_cursor;
									while (index != text.count) {
										if((char)text.base[index] == '\n')
											break;

										index++;
									}

									text_cursor = index;

								} break;
								case VK_LEFT: // left arrow
								{
									if(text_cursor > 0)
										text_cursor -= 1;

								} break;

								case VK_RIGHT: // right arrow
								{
									if(text_cursor < text.count)
										text_cursor += 1;

								} break;
							}
						}
					}

					platform_character_count = 0;

					PAINTSTRUCT  paint;
					HDC device_contex = BeginPaint(window_handle, &paint);

					RECT viewport;
					GetClientRect(window_handle, &viewport);

					FillRect(device_contex, &viewport, (HBRUSH) (COLOR_WINDOW + 1));

					RECT rect;
					rect.left   = 100;
					rect.top 	= 100;
					rect.right 	= rect.left + 300;
					rect.bottom = rect.top + 300;

					DrawText(device_contex, (char *)text.base, (int)text.count, &rect, DT_LEFT);

					EndPaint(window_handle, &paint);
					
				} break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(10);
	}

	printf("Shuting down!");
	return 0;
}