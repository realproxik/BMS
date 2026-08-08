// tool.c - Tool Implementation (C Style)
#include "tool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Tool Structure
// ============================================================================

typedef struct Tool {
    char* id;
    char* tooltip;
    char* label;
    char* icon;
    char* value;
    int x, y;
    int width, height;
    int type;
    int state;
    int enabled;
    int visible;
    void* userData;
    struct Tool* parent;
    struct Tool** children;
    int childCount;
    int childCapacity;
} Tool;

// ============================================================================
// Function Declarations
// ============================================================================

Tool* tool_create(int type);
void tool_destroy(Tool* tool);
const char* tool_get_id(const Tool* tool);
void tool_set_id(Tool* tool, const char* id);
const char* tool_get_tooltip(const Tool* tool);
void tool_set_tooltip(Tool* tool, const char* tooltip);
int tool_get_type(const Tool* tool);
int tool_get_state(const Tool* tool);
void tool_set_state(Tool* tool, int state);
int tool_is_enabled(const Tool* tool);
void tool_set_enabled(Tool* tool, int enabled);
int tool_is_visible(const Tool* tool);
void tool_set_visible(Tool* tool, int visible);
int tool_get_x(const Tool* tool);
int tool_get_y(const Tool* tool);
void tool_set_position(Tool* tool, int x, int y);
int tool_get_width(const Tool* tool);
int tool_get_height(const Tool* tool);
void tool_set_size(Tool* tool, int width, int height);
const char* tool_get_label(const Tool* tool);
void tool_set_label(Tool* tool, const char* label);
const char* tool_get_icon(const Tool* tool);
void tool_set_icon(Tool* tool, const char* icon);
const char* tool_get_value(const Tool* tool);
void tool_set_value(Tool* tool, const char* value);
void tool_add_child(Tool* parent, Tool* child);
void tool_remove_child(Tool* parent, Tool* child);
Tool* tool_get_parent(const Tool* tool);
int tool_get_child_count(const Tool* tool);
Tool* tool_get_child(const Tool* tool, int index);
void tool_render(Tool* tool, void* context);
void tool_update(Tool* tool);

// ============================================================================
// Implementation
// ============================================================================

Tool* tool_create(int type) {
    Tool* tool = (Tool*)calloc(1, sizeof(Tool));
    if (!tool) return NULL;
    
    tool->type = type;
    tool->state = 0; // NORMAL
    tool->enabled = 1;
    tool->visible = 1;
    tool->width = 32;
    tool->height = 32;
    tool->x = 0;
    tool->y = 0;
    tool->childCount = 0;
    tool->childCapacity = 4;
    tool->children = (Tool**)calloc(tool->childCapacity, sizeof(Tool*));
    
    tool->id = strdup("tool_unknown");
    tool->tooltip = strdup("");
    tool->label = strdup("");
    tool->icon = strdup("");
    tool->value = strdup("");
    
    return tool;
}

void tool_destroy(Tool* tool) {
    if (!tool) return;
    
    free(tool->id);
    free(tool->tooltip);
    free(tool->label);
    free(tool->icon);
    free(tool->value);
    
    for (int i = 0; i < tool->childCount; i++) {
        tool_destroy(tool->children[i]);
    }
    free(tool->children);
    free(tool);
}

const char* tool_get_id(const Tool* tool) {
    return tool ? tool->id : NULL;
}

void tool_set_id(Tool* tool, const char* id) {
    if (!tool || !id) return;
    free(tool->id);
    tool->id = strdup(id);
}

const char* tool_get_tooltip(const Tool* tool) {
    return tool ? tool->tooltip : NULL;
}

void tool_set_tooltip(Tool* tool, const char* tooltip) {
    if (!tool || !tooltip) return;
    free(tool->tooltip);
    tool->tooltip = strdup(tooltip);
}

int tool_get_type(const Tool* tool) {
    return tool ? tool->type : -1;
}

int tool_get_state(const Tool* tool) {
    return tool ? tool->state : -1;
}

void tool_set_state(Tool* tool, int state) {
    if (!tool) return;
    tool->state = state;
}

int tool_is_enabled(const Tool* tool) {
    return tool ? tool->enabled : 0;
}

void tool_set_enabled(Tool* tool, int enabled) {
    if (!tool) return;
    tool->enabled = enabled;
}

int tool_is_visible(const Tool* tool) {
    return tool ? tool->visible : 0;
}

void tool_set_visible(Tool* tool, int visible) {
    if (!tool) return;
    tool->visible = visible;
}

int tool_get_x(const Tool* tool) {
    return tool ? tool->x : 0;
}

int tool_get_y(const Tool* tool) {
    return tool ? tool->y : 0;
}

void tool_set_position(Tool* tool, int x, int y) {
    if (!tool) return;
    tool->x = x;
    tool->y = y;
}

int tool_get_width(const Tool* tool) {
    return tool ? tool->width : 0;
}

int tool_get_height(const Tool* tool) {
    return tool ? tool->height : 0;
}

void tool_set_size(Tool* tool, int width, int height) {
    if (!tool) return;
    tool->width = width;
    tool->height = height;
}

const char* tool_get_label(const Tool* tool) {
    return tool ? tool->label : NULL;
}

void tool_set_label(Tool* tool, const char* label) {
    if (!tool || !label) return;
    free(tool->label);
    tool->label = strdup(label);
}

const char* tool_get_icon(const Tool* tool) {
    return tool ? tool->icon : NULL;
}

void tool_set_icon(Tool* tool, const char* icon) {
    if (!tool || !icon) return;
    free(tool->icon);
    tool->icon = strdup(icon);
}

const char* tool_get_value(const Tool* tool) {
    return tool ? tool->value : NULL;
}

void tool_set_value(Tool* tool, const char* value) {
    if (!tool || !value) return;
    free(tool->value);
    tool->value = strdup(value);
}

void tool_add_child(Tool* parent, Tool* child) {
    if (!parent || !child) return;
    
    if (parent->childCount >= parent->childCapacity) {
        parent->childCapacity *= 2;
        parent->children = (Tool**)realloc(parent->children, 
                                           parent->childCapacity * sizeof(Tool*));
    }
    
    parent->children[parent->childCount++] = child;
    child->parent = parent;
}

void tool_remove_child(Tool* parent, Tool* child) {
    if (!parent || !child) return;
    
    for (int i = 0; i < parent->childCount; i++) {
        if (parent->children[i] == child) {
            for (int j = i; j < parent->childCount - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->childCount--;
            child->parent = NULL;
            return;
        }
    }
}

Tool* tool_get_parent(const Tool* tool) {
    return tool ? tool->parent : NULL;
}

int tool_get_child_count(const Tool* tool) {
    return tool ? tool->childCount : 0;
}

Tool* tool_get_child(const Tool* tool, int index) {
    if (!tool || index < 0 || index >= tool->childCount) return NULL;
    return tool->children[index];
}

void tool_render(Tool* tool, void* context) {
    if (!tool || !tool->visible) return;
    
    // Platform-specific rendering would go here
    // For now, just print to console
    printf("Tool: %s [%d,%d] %dx%d\n", 
           tool->label, tool->x, tool->y, tool->width, tool->height);
    
    // Render children
    for (int i = 0; i < tool->childCount; i++) {
        tool_render(tool->children[i], context);
    }
}

void tool_update(Tool* tool) {
    if (!tool) return;
    
    // Update tool state
    // Check for events, animations, etc.
    
    // Update children
    for (int i = 0; i < tool->childCount; i++) {
        tool_update(tool->children[i]);
    }
}

#ifdef __cplusplus
}
#endif