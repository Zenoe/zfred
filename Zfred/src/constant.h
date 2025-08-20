#pragma once

#define EDITOR_ID 1
#define LISTVIEW_ID  2
#define COMBO_ID 3

constexpr int clipItemSize = 200;

constexpr int PAGE_SIZE = 80;
constexpr int PADDING = 8;
//constexpr int EDIT_W = WND_W - (PADDING << 1);
constexpr int EDIT_W = 560;
constexpr int EDIT_H = 24; // to level with the default height of combox, (alos adjust the font size of the text to be 22)
constexpr int COMBO_W = 128;
//constexpr int COMBO_H = 320;  // unused, it seems the height is set by default(28)

constexpr int CONTROL_MARGIN = 1;

constexpr int MAX_ITEMS = 30;
constexpr int LIST_BOTTOM_PADDING = 4;


constexpr int WND_W = (PADDING << 1) + EDIT_W + COMBO_W + CONTROL_MARGIN;
constexpr int WND_H = (PADDING << 1) + EDIT_H + CONTROL_MARGIN + 300;

constexpr int LIST_W = WND_W;
