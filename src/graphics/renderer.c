#include "../../include/graphics.h"
#include "../../include/graph.h"
#include <stdio.h>

// Dracula theme colors
static const float COLOR_BG[] = {0.157f, 0.165f, 0.212f};
static const float COLOR_NODE_FILL[] = {0.384f, 0.447f, 0.643f};
static const float COLOR_NODE_BORDER[] = {0.972f, 0.972f, 0.949f};
static const float COLOR_EDGE[] = {1.0f, 0.474f, 0.776f};
static const float COLOR_MST[] = {0.314f, 0.980f, 0.482f};
static const float COLOR_SP[] = {0.545f, 0.914f, 0.992f};
static const float COLOR_MENU_BG[] = {0.2667f, 0.278f, 0.3529f};
static const float COLOR_TEXT[] = {0.972f, 0.972f, 0.949f};

void init_graphics(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutCreateWindow("Graph Visualizer - Dracula Theme");
  glClearColor(COLOR_BG[0], COLOR_BG[1], COLOR_BG[2], 1.0f);
}

void draw_nodes()
{
  // ...existing draw_nodes implementation...
}

void draw_edges()
{
  // ...existing draw_edges implementation...
}

// ... Continue with other drawing functions ...
