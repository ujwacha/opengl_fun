/**
 * @file main.cpp
 * @brief Graph Visualizer using OpenGL with Dracula Theme
 *
 * This program visualizes a graph with nodes and edges, allowing the user to
 * add nodes, add edges, find the shortest path, edit edge weights, delete nodes,
 * and find the Minimum Spanning Tree (MST) using Kruskal's algorithm.
 *
 * The visualization uses the Dracula theme for colors.
 */

#include <GL/glut.h>
#include <ctype.h>
#include <float.h>
#include <iostream>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mode constants
#define MODE_ADD_NODE 1
#define MODE_ADD_EDGE 2
#define MODE_SHORTEST_PATH 3
#define MODE_EDIT_WEIGHT 4
#define MODE_DELETE_NODE 5
#define MODE_MST 6

#define MAX_NODES 1000
#define INF FLT_MAX

// Menu pixel region constants
#define MENU_WIDTH_PIXELS 150
#define BUTTON_WIDTH 130
#define BUTTON_HEIGHT 40
#define BUTTON_PADDING 10

// Dracula theme color definitions
#define COLOR_BG_R 0.157f // #282a36 background
#define COLOR_BG_G 0.165f
#define COLOR_BG_B 0.212f

#define COLOR_NODE_FILL_R 0.384f // #6272a4 node fill
#define COLOR_NODE_FILL_G 0.447f
#define COLOR_NODE_FILL_B 0.643f

#define COLOR_NODE_BORDER_R 0.972f // #f8f8f2 node border
#define COLOR_NODE_BORDER_G 0.972f
#define COLOR_NODE_BORDER_B 0.949f

#define COLOR_EDGE_R 1.0f // #ff79c6 edge color
#define COLOR_EDGE_G 0.474f
#define COLOR_EDGE_B 0.776f

#define COLOR_MST_R 0.314f // #50fa7b MST edge color
#define COLOR_MST_G 0.980f
#define COLOR_MST_B 0.482f

#define COLOR_SP_R 0.545f // #8be9fd shortest path color
#define COLOR_SP_G 0.914f
#define COLOR_SP_B 0.992f

#define COLOR_MENU_BG_R 0.2667f // #44475a menu background
#define COLOR_MENU_BG_G 0.278f
#define COLOR_MENU_BG_B 0.3529f

#define COLOR_BUTTON_ACTIVE_R 0.741f // #bd93f9 active button
#define COLOR_BUTTON_ACTIVE_G 0.576f
#define COLOR_BUTTON_ACTIVE_B 0.976f

#define COLOR_BUTTON_INACTIVE_R 0.384f // same as node fill
#define COLOR_BUTTON_INACTIVE_G 0.447f
#define COLOR_BUTTON_INACTIVE_B 0.643f

#define COLOR_TEXT_R 0.972f // #f8f8f2 text color
#define COLOR_TEXT_G 0.972f
#define COLOR_TEXT_B 0.949f

// Global state variables
int current_mode = MODE_ADD_NODE;
int selected_node = -1; // For add edge mode
int sp_selected = -1;   // For shortest path mode

// For Dijkstra results (shortest path)
int shortest_path_nodes[MAX_NODES];
int shortest_path_length = 0;

// For MST
float mst_sum = 0;

// For weight input
bool inputting_weight = false;
char weight_input_buffer[32] = "";
int temp_src, temp_dest;
// For editing an existing edge
bool editing_existing_edge = false;
int editing_edge = -1;

// Use a constant radius for nodes
const float NODE_RADIUS = 0.05f;

typedef struct
{
  float x, y;
  char label;
} Node;

typedef struct
{
  int src, dest;
  float weight;
} Edge;

Node nodes[MAX_NODES];
Edge edges[MAX_NODES * MAX_NODES];
int node_count = 0, edge_count = 0;

// Forward declarations
void dijkstra(int start, int end);
void draw_weight_input();
int find_edge_near(float x, float y);
float pointToSegmentDistance(float px, float py, float ax, float ay, float bx,
                             float by);
void delete_node(int node_index);
void draw_mst();
void update_layout();
void idle();
void draw_mode_dialog();

/**
 * @brief Updates the layout of the nodes using a force-directed algorithm.
 */
void update_layout()
{
  if (node_count == 0)
    return;

  // Compute the wall's x-coordinate in GL space so that nodes don't enter the
  // side panel.
  int winWidth = glutGet(GLUT_WINDOW_WIDTH);
  float wall_x = (MENU_WIDTH_PIXELS / (float)winWidth) * 2.0f -
                 1.0f; // e.g. ~ -0.625 for 800px width

  float area = 4.0f; // (2x2 coordinate system from -1 to 1)
  float k = sqrt(area / (float)node_count);
  float disp[MAX_NODES][2];
  for (int i = 0; i < node_count; i++)
  {
    disp[i][0] = 0;
    disp[i][1] = 0;
  }

  // Repulsive forces between all pairs of nodes
  for (int i = 0; i < node_count; i++)
  {
    for (int j = 0; j < node_count; j++)
    {
      if (i == j)
        continue;
      float dx = nodes[i].x - nodes[j].x;
      float dy = nodes[i].y - nodes[j].y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < 0.001f)
        dist = 0.001f;
      float force = (k * k) / dist;
      disp[i][0] += (dx / dist) * force;
      disp[i][1] += (dy / dist) * force;
    }
  }

  // Attractive forces for nodes connected by an edge
  for (int i = 0; i < edge_count; i++)
  {
    int src = edges[i].src;
    int dest = edges[i].dest;
    float dx = nodes[src].x - nodes[dest].x;
    float dy = nodes[src].y - nodes[dest].y;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.001f)
      dist = 0.001f;
    float force = (dist * dist) / k;
    float fx = (dx / dist) * force;
    float fy = (dy / dist) * force;
    disp[src][0] -= fx;
    disp[src][1] -= fy;
    disp[dest][0] += fx;
    disp[dest][1] += fy;
  }

  // Centering force: pull nodes toward the center (0,0)
  float centering_strength = 4.0f;
  for (int i = 0; i < node_count; i++)
  {
    disp[i][0] -= nodes[i].x * centering_strength;
    disp[i][1] -= nodes[i].y * centering_strength;
  }

  // Update node positions with maximum displacement and damping factor
  float temp = 0.05f;   // maximum allowed move per iteration
  float damping = 0.1f; // damping factor to reduce oscillations
  for (int i = 0; i < node_count; i++)
  {
    float disp_length = sqrt(disp[i][0] * disp[i][0] + disp[i][1] * disp[i][1]);
    if (disp_length < 0.001f)
      disp_length = 0.001f;
    float dx = (disp[i][0] / disp_length) * fmin(disp_length, temp);
    float dy = (disp[i][1] / disp_length) * fmin(disp_length, temp);
    nodes[i].x += dx * damping;
    nodes[i].y += dy * damping;
    // Clamp x so that nodes do not cross the wall, and clamp y to [-1,1]
    if (nodes[i].x < wall_x)
      nodes[i].x = wall_x;
    if (nodes[i].x > 1)
      nodes[i].x = 1;
    if (nodes[i].y < -1)
      nodes[i].y = -1;
    if (nodes[i].y > 1)
      nodes[i].y = 1;
  }
}

/**
 * @brief Idle function for continuous layout updates.
 */
void idle()
{
  update_layout();
  glutPostRedisplay();
}

/**
 * @brief Draws a string at the specified coordinates.
 *
 * @param x X-coordinate
 * @param y Y-coordinate
 * @param str String to draw
 */
void draw_string(float x, float y, char *str)
{
  glRasterPos2f(x, y);
  for (char *c = str; *c != '\0'; c++)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }
}

/**
 * @brief Draws a string at the specified pixel coordinates.
 *
 * @param x X-coordinate in pixels
 * @param y Y-coordinate in pixels
 * @param str String to draw
 */
void draw_string_pixel(int x, int y, const char *str)
{
  glRasterPos2i(x, y);
  for (const char *c = str; *c != '\0'; c++)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }
}

/**
 * @brief Draws the nodes as circles with centered labels.
 */
void draw_nodes()
{
  int num_segments = 50;
  for (int i = 0; i < node_count; i++)
  {
    float cx = nodes[i].x;
    float cy = nodes[i].y;

    // Filled circle (node fill color)
    glColor3f(COLOR_NODE_FILL_R, COLOR_NODE_FILL_G, COLOR_NODE_FILL_B);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int j = 0; j <= num_segments; j++)
    {
      float angle = 2.0f * 3.1415926f * j / num_segments;
      float x = cx + cos(angle) * NODE_RADIUS;
      float y = cy + sin(angle) * NODE_RADIUS;
      glVertex2f(x, y);
    }
    glEnd();

    // Circle border (node border color)
    glColor3f(COLOR_NODE_BORDER_R, COLOR_NODE_BORDER_G, COLOR_NODE_BORDER_B);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j <= num_segments; j++)
    {
      float angle = 2.0f * 3.1415926f * j / num_segments;
      float x = cx + cos(angle) * NODE_RADIUS;
      float y = cy + sin(angle) * NODE_RADIUS;
      glVertex2f(x, y);
    }
    glEnd();

    // Label centered in the circle
    char label[2] = {nodes[i].label, '\0'};
    glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    draw_string(cx - 0.008f, cy - 0.02f, label);
  }
}

/**
 * @brief Draws the edges between nodes.
 */
void draw_edges()
{
  glColor3f(COLOR_EDGE_R, COLOR_EDGE_G, COLOR_EDGE_B);
  glLineWidth(4.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < edge_count; i++)
  {
    Node src = nodes[edges[i].src];
    Node dest = nodes[edges[i].dest];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0)
      d = 0.0001f;
    float offsetX = (dx / d) * NODE_RADIUS;
    float offsetY = (dy / d) * NODE_RADIUS;
    float startX = src.x + offsetX;
    float startY = src.y + offsetY;
    float endX = dest.x - offsetX;
    float endY = dest.y - offsetY;
    glVertex2f(startX, startY);
    glVertex2f(endX, endY);
  }
  glEnd();

  // Draw edge weight labels
  for (int i = 0; i < edge_count; i++)
  {
    Node src = nodes[edges[i].src];
    Node dest = nodes[edges[i].dest];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0)
      d = 0.0001f;
    float offsetX = (dx / d) * NODE_RADIUS;
    float offsetY = (dy / d) * NODE_RADIUS;
    float startX = src.x + offsetX;
    float startY = src.y + offsetY;
    float endX = dest.x - offsetX;
    float endY = dest.y - offsetY;
    float midX = (startX + endX) / 2;
    float midY = (startY + endY) / 2;

    float label_offset = 0.03f;
    float perpX = -dy / d;
    float perpY = dx / d;
    float labelX = midX + label_offset * perpX;
    float labelY = midY + label_offset * perpY;

    char weight_str[10];
    sprintf(weight_str, "%.1f", edges[i].weight);
    glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    draw_string(labelX - 0.015f, labelY - 0.015f, weight_str);
  }
}

/**
 * @brief Draws the shortest path between two nodes.
 */
void draw_shortest_path()
{
  if (shortest_path_length < 2)
    return;
  glColor3f(COLOR_SP_R, COLOR_SP_G, COLOR_SP_B);
  glLineWidth(4.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < shortest_path_length - 1; i++)
  {
    Node src = nodes[shortest_path_nodes[i]];
    Node dest = nodes[shortest_path_nodes[i + 1]];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0)
      d = 0.0001f;
    float offsetX = (dx / d) * NODE_RADIUS;
    float offsetY = (dy / d) * NODE_RADIUS;
    float startX = src.x + offsetX;
    float startY = src.y + offsetY;
    float endX = dest.x - offsetX;
    float endY = dest.y - offsetY;
    glVertex2f(startX, startY);
    glVertex2f(endX, endY);
  }
  glEnd();
}

/**
 * @brief Draws the Minimum Spanning Tree (MST) using Kruskal's algorithm.
 */
void draw_mst()
{
  typedef struct
  {
    int src, dest;
    float weight;
  } MstEdge;

  MstEdge mst_edges[MAX_NODES];
  int mst_count = 0;

  int parent[MAX_NODES];
  for (int i = 0; i < node_count; i++)
  {
    parent[i] = i;
  }
  auto find = [&](int x) -> int
  {
    while (parent[x] != x)
    {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  };
  auto union_set = [&](int x, int y)
  {
    int rootx = find(x);
    int rooty = find(y);
    parent[rootx] = rooty;
  };

  int indices[MAX_NODES * MAX_NODES];
  for (int i = 0; i < edge_count; i++)
  {
    indices[i] = i;
  }
  for (int i = 0; i < edge_count - 1; i++)
  {
    for (int j = i + 1; j < edge_count; j++)
    {
      if (edges[indices[j]].weight < edges[indices[i]].weight)
      {
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
      }
    }
  }

  for (int i = 0; i < edge_count; i++)
  {
    int idx = indices[i];
    int u = edges[idx].src;
    int v = edges[idx].dest;
    if (find(u) != find(v))
    {
      union_set(u, v);
      mst_edges[mst_count].src = u;
      mst_edges[mst_count].dest = v;
      mst_edges[mst_count].weight = edges[idx].weight;
      mst_count++;
      if (mst_count == node_count - 1)
        break;
    }
  }

  for (int i = 0; i < mst_count; i++)
  {
    mst_sum += mst_edges[i].weight;
  }

  glColor3f(COLOR_MST_R, COLOR_MST_G, COLOR_MST_B);
  glLineWidth(4.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < mst_count; i++)
  {
    Node src = nodes[mst_edges[i].src];
    Node dest = nodes[mst_edges[i].dest];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0)
      d = 0.0001f;
    float offsetX = (dx / d) * NODE_RADIUS;
    float offsetY = (dy / d) * NODE_RADIUS;
    float startX = src.x + offsetX;
    float startY = src.y + offsetY;
    float endX = dest.x - offsetX;
    float endY = dest.y - offsetY;
    glVertex2f(startX, startY);
    glVertex2f(endX, endY);
  }
  glEnd();
}

/**
 * @brief Draws the side menu with buttons.
 */
void draw_menu_pixel()
{
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Menu background
  glColor3f(COLOR_MENU_BG_R, COLOR_MENU_BG_G, COLOR_MENU_BG_B);
  glBegin(GL_QUADS);
  glVertex2i(0, 0);
  glVertex2i(MENU_WIDTH_PIXELS, 0);
  glVertex2i(MENU_WIDTH_PIXELS, h);
  glVertex2i(0, h);
  glEnd();

  int y = 20;
  // Add Node button
  glColor3f(current_mode == MODE_ADD_NODE ? COLOR_BUTTON_ACTIVE_R
                                          : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_ADD_NODE ? COLOR_BUTTON_ACTIVE_G
                                          : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_ADD_NODE ? COLOR_BUTTON_ACTIVE_B
                                          : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Add Node");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Add Edge button
  glColor3f(current_mode == MODE_ADD_EDGE ? COLOR_BUTTON_ACTIVE_R
                                          : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_ADD_EDGE ? COLOR_BUTTON_ACTIVE_G
                                          : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_ADD_EDGE ? COLOR_BUTTON_ACTIVE_B
                                          : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Add Edge");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Shortest Path button
  glColor3f(current_mode == MODE_SHORTEST_PATH ? COLOR_BUTTON_ACTIVE_R
                                               : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_SHORTEST_PATH ? COLOR_BUTTON_ACTIVE_G
                                               : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_SHORTEST_PATH ? COLOR_BUTTON_ACTIVE_B
                                               : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Shortest Path");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Edit Weight button
  glColor3f(current_mode == MODE_EDIT_WEIGHT ? COLOR_BUTTON_ACTIVE_R
                                             : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_EDIT_WEIGHT ? COLOR_BUTTON_ACTIVE_G
                                             : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_EDIT_WEIGHT ? COLOR_BUTTON_ACTIVE_B
                                             : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Edit Weight");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Delete Node button
  glColor3f(current_mode == MODE_DELETE_NODE ? COLOR_BUTTON_ACTIVE_R
                                             : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_DELETE_NODE ? COLOR_BUTTON_ACTIVE_G
                                             : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_DELETE_NODE ? COLOR_BUTTON_ACTIVE_B
                                             : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Delete Node");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // MST button
  glColor3f(current_mode == MODE_MST ? COLOR_BUTTON_ACTIVE_R
                                     : COLOR_BUTTON_INACTIVE_R,
            current_mode == MODE_MST ? COLOR_BUTTON_ACTIVE_G
                                     : COLOR_BUTTON_INACTIVE_G,
            current_mode == MODE_MST ? COLOR_BUTTON_ACTIVE_B
                                     : COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "MST");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Clear Screen button
  glColor3f(COLOR_BUTTON_INACTIVE_R, COLOR_BUTTON_INACTIVE_G, COLOR_BUTTON_INACTIVE_B);
  glBegin(GL_QUADS);
  glVertex2i(10, y);
  glVertex2i(BUTTON_WIDTH + 10, y);
  glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
  glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  draw_string_pixel(15, y + 25, "Clear Screen");

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

/**
 * @brief Draws the weight input popup.
 */
void draw_weight_input()
{
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  int box_width = 200;
  int box_height = 50;
  int x = (w - box_width) / 2;
  int y = (h - box_height) / 2;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Box background (using menu background color)
  glColor3f(COLOR_MENU_BG_R, COLOR_MENU_BG_G, COLOR_MENU_BG_B);
  glBegin(GL_QUADS);
  glVertex2i(x, y);
  glVertex2i(x + box_width, y);
  glVertex2i(x + box_width, y + box_height);
  glVertex2i(x, y + box_height);
  glEnd();

  // Border (text color)
  glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2i(x, y);
  glVertex2i(x + box_width, y);
  glVertex2i(x + box_width, y + box_height);
  glVertex2i(x, y + box_height);
  glEnd();

  // Prompt text
  char prompt[64];
  char fromChar, toChar;
  if (editing_existing_edge)
  {
    fromChar = 'A' + edges[editing_edge].src;
    toChar = 'A' + edges[editing_edge].dest;
  }
  else
  {
    fromChar = 'A' + temp_src;
    toChar = 'A' + temp_dest;
  }
  sprintf(prompt, "Weight for %c-%c:", fromChar, toChar);
  draw_string_pixel(x + 10, y + 20, prompt);
  sprintf(prompt, "%s_", weight_input_buffer);
  draw_string_pixel(x + 10, y + 40, prompt);

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

/**
 * @brief Finds the node at the specified coordinates.
 *
 * @param x X-coordinate
 * @param y Y-coordinate
 * @return Index of the node, or -1 if no node is found
 */
int find_node(float x, float y)
{
  for (int i = 0; i < node_count; i++)
  {
    float dx = nodes[i].x - x;
    float dy = nodes[i].y - y;
    if (dx * dx + dy * dy < NODE_RADIUS * NODE_RADIUS)
      return i;
  }
  return -1;
}

/**
 * @brief Adds an edge between two nodes with the specified weight.
 *
 * @param src Source node index
 * @param dest Destination node index
 * @param weight Weight of the edge
 */
void add_edge(int src, int dest, float weight)
{
  edges[edge_count++] = (Edge){src, dest, weight};
}

/**
 * @brief Calculates the distance from a point to a line segment.
 *
 * @param px X-coordinate of the point
 * @param py Y-coordinate of the point
 * @param ax X-coordinate of the segment start
 * @param ay Y-coordinate of the segment start
 * @param bx X-coordinate of the segment end
 * @param by Y-coordinate of the segment end
 * @return Distance from the point to the segment
 */
float pointToSegmentDistance(float px, float py, float ax, float ay, float bx,
                             float by)
{
  float vx = bx - ax, vy = by - ay;
  float wx = px - ax, wy = py - ay;
  float c1 = vx * wx + vy * wy;
  if (c1 <= 0)
    return sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
  float c2 = vx * vx + vy * vy;
  if (c2 <= c1)
    return sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
  float bVal = c1 / c2;
  float projx = ax + bVal * vx;
  float projy = ay + bVal * vy;
  return sqrt((px - projx) * (px - projx) + (py - projy) * (py - projy));
}

/**
 * @brief Finds the edge near the specified coordinates.
 *
 * @param x X-coordinate
 * @param y Y-coordinate
 * @return Index of the edge, or -1 if no edge is found
 */
int find_edge_near(float x, float y)
{
  const float threshold = 0.05f;
  for (int i = 0; i < edge_count; i++)
  {
    Node a = nodes[edges[i].src];
    Node b = nodes[edges[i].dest];
    float dist = pointToSegmentDistance(x, y, a.x, a.y, b.x, b.y);
    if (dist < threshold)
      return i;
  }
  return -1;
}

/**
 * @brief Finds the shortest path between two nodes using Dijkstra's algorithm.
 *
 * @param start Index of the start node
 * @param end Index of the end node
 */
void dijkstra(int start, int end)
{
  if (start < 0 || start >= node_count || end < 0 || end >= node_count)
  {
    std::cerr << "Invalid start or end node for Dijkstra's algorithm.\n";
    return;
  }

  float dist[MAX_NODES];
  bool visited[MAX_NODES];
  int prev[MAX_NODES];
  for (int i = 0; i < node_count; i++)
  {
    dist[i] = INF;
    visited[i] = false;
    prev[i] = -1;
  }
  dist[start] = 0;

  for (int i = 0; i < node_count; i++)
  {
    int u = -1;
    float min_dist = INF;
    for (int j = 0; j < node_count; j++)
    {
      if (!visited[j] && dist[j] < min_dist)
      {
        u = j;
        min_dist = dist[j];
      }
    }
    if (u == -1)
      break;
    visited[u] = true;

    for (int k = 0; k < edge_count; k++)
    {
      int v = -1;
      if (edges[k].src == u)
        v = edges[k].dest;
      if (edges[k].dest == u)
        v = edges[k].src;
      if (v != -1 && !visited[v] && dist[u] + edges[k].weight < dist[v])
      {
        dist[v] = dist[u] + edges[k].weight;
        prev[v] = u;
      }
    }
  }

  shortest_path_length = 0;
  if (dist[end] == INF)
    return;

  int path[MAX_NODES];
  int at = end;
  while (at != -1)
  {
    path[shortest_path_length++] = at;
    at = prev[at];
  }

  for (int i = 0; i < shortest_path_length; i++)
  {
    shortest_path_nodes[i] = path[shortest_path_length - 1 - i];
  }
}

/**
 * @brief Deletes a node and updates the graph.
 *
 * @param node_index Index of the node to delete
 */
void delete_node(int node_index)
{
  for (int i = 0; i < edge_count;)
  {
    if (edges[i].src == node_index || edges[i].dest == node_index)
    {
      for (int j = i; j < edge_count - 1; j++)
      {
        edges[j] = edges[j + 1];
      }
      edge_count--;
    }
    else
    {
      if (edges[i].src > node_index)
        edges[i].src--;
      if (edges[i].dest > node_index)
        edges[i].dest--;
      i++;
    }
  }
  for (int i = node_index; i < node_count - 1; i++)
  {
    nodes[i] = nodes[i + 1];
    nodes[i].label = 'A' + i;
  }
  node_count--;
}

/**
 * @brief Mouse callback function to handle mouse events.
 *
 * @param button Mouse button
 * @param state Button state (GLUT_DOWN or GLUT_UP)
 * @param x X-coordinate of the mouse
 * @param y Y-coordinate of the mouse
 */
void mouse(int button, int state, int x, int y)
{
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  std::cout << "X: " << x << " Y: " << y << "\n";

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    if (x < MENU_WIDTH_PIXELS)
    {
      selected_node = -1;
      sp_selected = -1;
      int y_pos = y;
      if (y_pos >= 20 && y_pos <= 60)
        current_mode = MODE_ADD_NODE;
      else if (y_pos >= 70 && y_pos <= 110)
        current_mode = MODE_ADD_EDGE;
      else if (y_pos >= 120 && y_pos <= 160)
        current_mode = MODE_SHORTEST_PATH;
      else if (y_pos >= 170 && y_pos <= 210)
        current_mode = MODE_EDIT_WEIGHT;
      else if (y_pos >= 220 && y_pos <= 260)
        current_mode = MODE_DELETE_NODE;
      else if (y_pos >= 270 && y_pos <= 310)
        current_mode = MODE_MST;
      else if (y_pos >= 320 && y_pos <= 360)
      {
        // Clear Screen button clicked
        node_count = 0;
        edge_count = 0;
      }
      glutPostRedisplay();
      return;
    }

    float gl_x = (x / (float)w) * 2.0f - 1.0f;
    float gl_y = 1.0f - (y / (float)h) * 2.0f;

    if (current_mode == MODE_ADD_NODE)
    {
      if (find_node(gl_x, gl_y) == -1 && node_count < MAX_NODES)
      {
        nodes[node_count] = (Node){gl_x, gl_y, 'A' + node_count};
        node_count++;
      }
    }
    else if (current_mode == MODE_ADD_EDGE)
    {
      int node = find_node(gl_x, gl_y);
      if (node != -1)
      {
        if (selected_node == -1)
        {
          selected_node = node;
        }
        else if (node != selected_node)
        {
          temp_src = selected_node;
          temp_dest = node;
          inputting_weight = true;
          editing_existing_edge = false;
          weight_input_buffer[0] = '\0';
          selected_node = -1;
        }
      }
    }
    else if (current_mode == MODE_SHORTEST_PATH)
    {
      int node = find_node(gl_x, gl_y);
      if (node != -1)
      {
        if (sp_selected == -1)
        {
          sp_selected = node;
        }
        else if (node != sp_selected)
        {
          dijkstra(sp_selected, node);
          sp_selected = -1;
        }
      }
    }
    else if (current_mode == MODE_EDIT_WEIGHT)
    {
      int edge_index = find_edge_near(gl_x, gl_y);
      if (edge_index != -1)
      {
        editing_edge = edge_index;
        editing_existing_edge = true;
        inputting_weight = true;
        snprintf(weight_input_buffer, sizeof(weight_input_buffer), "%.1f",
                 edges[edge_index].weight);
      }
    }
    else if (current_mode == MODE_DELETE_NODE)
    {
      int node = find_node(gl_x, gl_y);
      if (node != -1)
      {
        delete_node(node);
      }
    }
    glutPostRedisplay();
  }
}

/**
 * @brief Keyboard callback function to handle keyboard events.
 *
 * @param key Key pressed
 * @param x X-coordinate of the mouse
 * @param y Y-coordinate of the mouse
 */
void keyboard(unsigned char key, int x, int y)
{
  if (inputting_weight)
  {
    if (key == '\r' || key == '\n')
    {
      float weight = atof(weight_input_buffer);
      if (weight > 0)
      {
        if (editing_existing_edge)
        {
          edges[editing_edge].weight = weight;
          editing_edge = -1;
          editing_existing_edge = false;
        }
        else
        {
          add_edge(temp_src, temp_dest, weight);
        }
      }
      inputting_weight = false;
      weight_input_buffer[0] = '\0';
    }
    else if (key == 8 || key == 127)
    {
      int len = strlen(weight_input_buffer);
      if (len > 0)
        weight_input_buffer[len - 1] = '\0';
    }
    else if ((isdigit(key) || key == '.') &&
             (strlen(weight_input_buffer) < 31))
    {
      if (key == '.' && strchr(weight_input_buffer, '.') != NULL)
      {
      }
      else
      {
        char keyStr[2] = {key, '\0'};
        strncat(weight_input_buffer, keyStr, 1);
      }
    }
    glutPostRedisplay();
  }
}

/**
 * @brief Display callback function to render the scene.
 */
void display()
{
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_nodes();
  draw_edges();

  if (current_mode == MODE_SHORTEST_PATH)
    draw_shortest_path();
  else if (current_mode == MODE_MST)
  {
    draw_mst();
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    char sum_str[50];
    sprintf(sum_str, "MST Sum: %.1f", mst_sum);
    glColor3f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    draw_string_pixel(w - 200, 30, sum_str);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    mst_sum = 0;
  }

  draw_menu_pixel();
  if (inputting_weight)
    draw_weight_input();

  draw_mode_dialog(); // Add this line to draw the mode dialog

  glFlush();
}

/**
 * @brief Draws the mode dialog with instructions.
 */
void draw_mode_dialog()
{
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  int box_width = 300;
  int box_height = 75;
  int x = w - box_width - 20;
  int y = 20;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Enable blending for transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Box background (using menu background color)
  glColor4f(COLOR_MENU_BG_R, COLOR_MENU_BG_G, COLOR_MENU_BG_B, 0.5f);
  glBegin(GL_QUADS);
  glVertex2i(x, y);
  glVertex2i(x + box_width, y);
  glVertex2i(x + box_width, y + box_height);
  glVertex2i(x, y + box_height);
  glEnd();

  // Border (text color)
  glColor4f(COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 0.5f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2i(x, y);
  glVertex2i(x + box_width, y);
  glVertex2i(x + box_width, y + box_height);
  glVertex2i(x, y + box_height);
  glEnd();

  // Mode instructions
  const char *mode_str;
  switch (current_mode)
  {
  case MODE_ADD_NODE:
    mode_str = "Mode: Add Node\nClick empty area to add a node.";
    break;
  case MODE_ADD_EDGE:
    mode_str = "Mode: Add Edge\nClick two nodes to add an edge.";
    break;
  case MODE_SHORTEST_PATH:
    mode_str = "Mode: Shortest Path\nClick two nodes to find the\nshortest path.";
    break;
  case MODE_EDIT_WEIGHT:
    mode_str = "Mode: Edit Weight\nClick an edge to edit its weight.";
    break;
  case MODE_DELETE_NODE:
    mode_str = "Mode: Delete Node\nClick a node to delete it.";
    break;
  case MODE_MST:
    mode_str = "Mode: MST\nMinimum Spanning Tree will be\ndisplayed.";
    break;
  default:
    mode_str = "Mode: Unknown";
    break;
  }

  // Draw mode instructions
  int line_height = 20;
  int line_y = y + 20;
  char *line = strtok(strdup(mode_str), "\n");
  while (line != NULL)
  {
    draw_string_pixel(x + 10, line_y, line);
    line_y += line_height;
    line = strtok(NULL, "\n");
  }

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

/**
 * @brief Main function to initialize the program and start the GLUT main loop.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status
 */
int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Graph Visualizer - Dracula Theme");

  // Set the background to Dracula theme color
  glClearColor(COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, 1.0f);

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  glutMainLoop();
  return 0;
}
