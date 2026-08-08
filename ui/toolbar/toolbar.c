// toolbar.c - Toolbar Implementation (C)
#include "toolbar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Toolbar Structure
// ============================================================================

typedef struct Toolbar {
    Tool** tools;
    int toolCount;
    int toolCapacity;
    int orientation;
    int spacing;
    int padding;
    int borderWidth;
    int cornerRadius;
    char* backgroundColor;
    char* borderColor;
    int autoHide;
    int floating;
    int resizable;
    int movable;
    int visible;
    int x, y;
    int width, height;
    void* nativeHandle;
} Toolbar;

// ============================================================================
// Function Declarations
// ============================================================================

Toolbar* toolbar_create(void);
void toolbar_destroy(Toolbar* toolbar);
void toolbar_add_tool(Toolbar* toolbar, Tool* tool);
void toolbar_remove_tool_by_id(Toolbar* toolbar, const char* id);
void toolbar_remove_tool_by_index(Toolbar* toolbar, int index);
void toolbar_clear(Toolbar* toolbar);
int toolbar_get_count(const Toolbar* toolbar);
Tool* toolbar_get_tool(const Toolbar* toolbar, int index);
void toolbar_set_orientation(Toolbar* toolbar, int orientation);
int toolbar_get_orientation(const Toolbar* toolbar);
void toolbar_set_spacing(Toolbar* toolbar, int spacing);
int toolbar_get_spacing(const Toolbar* toolbar);
void toolbar_set_padding(Toolbar* toolbar, int padding);
int toolbar_get_padding(const Toolbar* toolbar);
void toolbar_set_background_color(Toolbar* toolbar, const char* color);
const char* toolbar_get_background_color(const Toolbar* toolbar);
void toolbar_set_border_color(Toolbar* toolbar, const char* color);
const char* toolbar_get_border_color(const Toolbar* toolbar);
void toolbar_set_border_width(Toolbar* toolbar, int width);
int toolbar_get_border_width(const Toolbar* toolbar);
void toolbar_set_corner_radius(Toolbar* toolbar, int radius);
int toolbar_get_corner_radius(const Toolbar* toolbar);
void toolbar_set_auto_hide(Toolbar* toolbar, int autoHide);
int toolbar_get_auto_hide(const Toolbar* toolbar);
void toolbar_set_floating(Toolbar* toolbar, int floating);
int toolbar_get_floating(const Toolbar* toolbar);
void toolbar_set_resizable(Toolbar* toolbar, int resizable);
int toolbar_get_resizable(const Toolbar* toolbar);
void toolbar_set_movable(Toolbar* toolbar, int movable);
int toolbar_get_movable(const Toolbar* toolbar);
void toolbar_show(Toolbar* toolbar);
void toolbar_hide(Toolbar* toolbar);
int toolbar_is_visible(const Toolbar* toolbar);
void toolbar_set_position(Toolbar* toolbar, int x, int y);
void toolbar_get_position(const Toolbar* toolbar, int* x, int* y);
void toolbar_set_size(Toolbar* toolbar, int width, int height);
void toolbar_get_size(const Toolbar* toolbar, int* width, int* height);
void toolbar_render(Toolbar* toolbar, void* context);
void toolbar_update(Toolbar* toolbar);

// ============================================================================
// Implementation
// ============================================================================

Toolbar* toolbar_create(void) {
    Toolbar* toolbar = (Toolbar*)calloc(1, sizeof(Toolbar));
    if (!toolbar) return NULL;
    
    toolbar->toolCapacity = 10;
    toolbar->tools = (Tool**)calloc(toolbar->toolCapacity, sizeof(Tool*));
    toolbar->toolCount = 0;
    toolbar->orientation = 0; // LEFT
    toolbar->spacing = 4;
    toolbar->padding = 4;
    toolbar->borderWidth = 1;
    toolbar->cornerRadius = 4;
    toolbar->backgroundColor = strdup("#f0f0f0");
    toolbar->borderColor = strdup("#cccccc");
    toolbar->autoHide = 0;
    toolbar->floating = 0;
    toolbar->resizable = 0;
    toolbar->movable = 0;
    toolbar->visible = 1;
    toolbar->x = 0;
    toolbar->y = 0;
    toolbar->width = 100;
    toolbar->height = 32;
    toolbar->nativeHandle = NULL;
    
    return toolbar;
}

void toolbar_destroy(Toolbar* toolbar) {
    if (!toolbar) return;
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        tool_destroy(toolbar->tools[i]);
    }
    free(toolbar->tools);
    free(toolbar->backgroundColor);
    free(toolbar->borderColor);
    free(toolbar);
}

void toolbar_add_tool(Toolbar* toolbar, Tool* tool) {
    if (!toolbar || !tool) return;
    
    if (toolbar->toolCount >= toolbar->toolCapacity) {
        toolbar->toolCapacity *= 2;
        toolbar->tools = (Tool**)realloc(toolbar->tools, 
                                         toolbar->toolCapacity * sizeof(Tool*));
    }
    
    toolbar->tools[toolbar->toolCount++] = tool;
    toolbar_update_layout(toolbar);
}

void toolbar_remove_tool_by_id(Toolbar* toolbar, const char* id) {
    if (!toolbar || !id) return;
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        const char* toolId = tool_get_id(toolbar->tools[i]);
        if (toolId && strcmp(toolId, id) == 0) {
            toolbar_remove_tool_by_index(toolbar, i);
            return;
        }
    }
}

void toolbar_remove_tool_by_index(Toolbar* toolbar, int index) {
    if (!toolbar || index < 0 || index >= toolbar->toolCount) return;
    
    tool_destroy(toolbar->tools[index]);
    
    for (int i = index; i < toolbar->toolCount - 1; i++) {
        toolbar->tools[i] = toolbar->tools[i + 1];
    }
    toolbar->toolCount--;
    
    toolbar_update_layout(toolbar);
}

void toolbar_clear(Toolbar* toolbar) {
    if (!toolbar) return;
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        tool_destroy(toolbar->tools[i]);
    }
    toolbar->toolCount = 0;
    toolbar_update_layout(toolbar);
}

int toolbar_get_count(const Toolbar* toolbar) {
    return toolbar ? toolbar->toolCount : 0;
}

Tool* toolbar_get_tool(const Toolbar* toolbar, int index) {
    if (!toolbar || index < 0 || index >= toolbar->toolCount) return NULL;
    return toolbar->tools[index];
}

void toolbar_set_orientation(Toolbar* toolbar, int orientation) {
    if (!toolbar) return;
    toolbar->orientation = orientation;
    toolbar_update_layout(toolbar);
}

int toolbar_get_orientation(const Toolbar* toolbar) {
    return toolbar ? toolbar->orientation : -1;
}

void toolbar_set_spacing(Toolbar* toolbar, int spacing) {
    if (!toolbar) return;
    toolbar->spacing = spacing > 0 ? spacing : 0;
    toolbar_update_layout(toolbar);
}

int toolbar_get_spacing(const Toolbar* toolbar) {
    return toolbar ? toolbar->spacing : 0;
}

void toolbar_set_padding(Toolbar* toolbar, int padding) {
    if (!toolbar) return;
    toolbar->padding = padding > 0 ? padding : 0;
    toolbar_update_layout(toolbar);
}

int toolbar_get_padding(const Toolbar* toolbar) {
    return toolbar ? toolbar->padding : 0;
}

void toolbar_set_background_color(Toolbar* toolbar, const char* color) {
    if (!toolbar || !color) return;
    free(toolbar->backgroundColor);
    toolbar->backgroundColor = strdup(color);
}

const char* toolbar_get_background_color(const Toolbar* toolbar) {
    return toolbar ? toolbar->backgroundColor : NULL;
}

void toolbar_set_border_color(Toolbar* toolbar, const char* color) {
    if (!toolbar || !color) return;
    free(toolbar->borderColor);
    toolbar->borderColor = strdup(color);
}

const char* toolbar_get_border_color(const Toolbar* toolbar) {
    return toolbar ? toolbar->borderColor : NULL;
}

void toolbar_set_border_width(Toolbar* toolbar, int width) {
    if (!toolbar) return;
    toolbar->borderWidth = width > 0 ? width : 0;
}

int toolbar_get_border_width(const Toolbar* toolbar) {
    return toolbar ? toolbar->borderWidth : 0;
}

void toolbar_set_corner_radius(Toolbar* toolbar, int radius) {
    if (!toolbar) return;
    toolbar->cornerRadius = radius > 0 ? radius : 0;
}

int toolbar_get_corner_radius(const Toolbar* toolbar) {
    return toolbar ? toolbar->cornerRadius : 0;
}

void toolbar_set_auto_hide(Toolbar* toolbar, int autoHide) {
    if (!toolbar) return;
    toolbar->autoHide = autoHide;
}

int toolbar_get_auto_hide(const Toolbar* toolbar) {
    return toolbar ? toolbar->autoHide : 0;
}

void toolbar_set_floating(Toolbar* toolbar, int floating) {
    if (!toolbar) return;
    toolbar->floating = floating;
}

int toolbar_get_floating(const Toolbar* toolbar) {
    return toolbar ? toolbar->floating : 0;
}

void toolbar_set_resizable(Toolbar* toolbar, int resizable) {
    if (!toolbar) return;
    toolbar->resizable = resizable;
}

int toolbar_get_resizable(const Toolbar* toolbar) {
    return toolbar ? toolbar->resizable : 0;
}

void toolbar_set_movable(Toolbar* toolbar, int movable) {
    if (!toolbar) return;
    toolbar->movable = movable;
}

int toolbar_get_movable(const Toolbar* toolbar) {
    return toolbar ? toolbar->movable : 0;
}

void toolbar_show(Toolbar* toolbar) {
    if (!toolbar) return;
    toolbar->visible = 1;
}

void toolbar_hide(Toolbar* toolbar) {
    if (!toolbar) return;
    toolbar->visible = 0;
}

int toolbar_is_visible(const Toolbar* toolbar) {
    return toolbar ? toolbar->visible : 0;
}

void toolbar_set_position(Toolbar* toolbar, int x, int y) {
    if (!toolbar) return;
    toolbar->x = x;
    toolbar->y = y;
    toolbar_update_layout(toolbar);
}

void toolbar_get_position(const Toolbar* toolbar, int* x, int* y) {
    if (!toolbar) return;
    if (x) *x = toolbar->x;
    if (y) *y = toolbar->y;
}

void toolbar_set_size(Toolbar* toolbar, int width, int height) {
    if (!toolbar) return;
    toolbar->width = width;
    toolbar->height = height;
    toolbar_update_layout(toolbar);
}

void toolbar_get_size(const Toolbar* toolbar, int* width, int* height) {
    if (!toolbar) return;
    if (width) *width = toolbar->width;
    if (height) *height = toolbar->height;
}

void toolbar_update_layout(Toolbar* toolbar) {
    if (!toolbar || toolbar->toolCount == 0) return;
    
    int currentX = toolbar->x + toolbar->padding;
    int currentY = toolbar->y + toolbar->padding;
    int horizontal = (toolbar->orientation == 0 || toolbar->orientation == 2);
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        Tool* tool = toolbar->tools[i];
        if (!tool_is_visible(tool)) continue;
        
        if (horizontal) {
            tool_set_position(tool, currentX, currentY);
            currentX += tool_get_width(tool) + toolbar->spacing;
        } else {
            tool_set_position(tool, currentX, currentY);
            currentY += tool_get_height(tool) + toolbar->spacing;
        }
    }
    
    // Update toolbar size
    if (horizontal) {
        toolbar->width = currentX - toolbar->x + toolbar->padding;
        toolbar->height = 0;
        for (int i = 0; i < toolbar->toolCount; i++) {
            Tool* tool = toolbar->tools[i];
            if (tool_is_visible(tool)) {
                int h = tool_get_height(tool) + toolbar->padding * 2;
                if (h > toolbar->height) toolbar->height = h;
            }
        }
    } else {
        toolbar->height = currentY - toolbar->y + toolbar->padding;
        toolbar->width = 0;
        for (int i = 0; i < toolbar->toolCount; i++) {
            Tool* tool = toolbar->tools[i];
            if (tool_is_visible(tool)) {
                int w = tool_get_width(tool) + toolbar->padding * 2;
                if (w > toolbar->width) toolbar->width = w;
            }
        }
    }
}

void toolbar_render(Toolbar* toolbar, void* context) {
    if (!toolbar || !toolbar->visible) return;
    
    printf("Toolbar: %d tools at [%d,%d] %dx%d\n", 
           toolbar->toolCount, toolbar->x, toolbar->y, 
           toolbar->width, toolbar->height);
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        tool_render(toolbar->tools[i], context);
    }
}

void toolbar_update(Toolbar* toolbar) {
    if (!toolbar) return;
    
    for (int i = 0; i < toolbar->toolCount; i++) {
        tool_update(toolbar->tools[i]);
    }
}

#ifdef __cplusplus
}
#endif