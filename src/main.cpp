#include <GL/glut.h>
#include <ctype.h>
#include <float.h>
#include <iostream>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_NODES 100
#define INF FLT_MAX

// Menu pixel region constants
#define MENU_WIDTH_PIXELS 150
#define BUTTON_WIDTH 130
#define BUTTON_HEIGHT 40
#define BUTTON_PADDING 10

// Modes
#define MODE_ADD_NODE       1
#define MODE_ADD_EDGE       2
#define MODE_SHORTEST_PATH  3
#define MODE_EDIT_WEIGHT    4
#define MODE_DELETE_NODE    5

int current_mode = MODE_ADD_NODE;
int selected_node = -1;    // For add edge mode
int sp_selected = -1;      // For shortest path mode

// For Dijkstra results (shortest path)
int shortest_path_nodes[MAX_NODES];
int shortest_path_length = 0;

// For weight input
bool inputting_weight = false;
char weight_input_buffer[32] = "";
int temp_src, temp_dest;
// For editing an existing edge
bool editing_existing_edge = false;
int editing_edge = -1;

// Use a constant radius for nodes
const float NODE_RADIUS = 0.05f;

typedef struct {
  float x, y;
  char label;
} Node;

typedef struct {
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
float pointToSegmentDistance(float px, float py, float ax, float ay, float bx, float by);
void delete_node(int node_index);

//
// --- Utility drawing functions ---
//

void draw_string(float x, float y, char *str) {
  glRasterPos2f(x, y);
  for (char *c = str; *c != '\0'; c++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }
}

void draw_string_pixel(int x, int y, char *str) {
  glRasterPos2i(x, y);
  for (char *c = str; *c != '\0'; c++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }
}

//
// --- Drawing the graph ---
//

// Draw nodes as light blue circles with labels centered
void draw_nodes() {
  int num_segments = 50;
  for (int i = 0; i < node_count; i++) {
    float cx = nodes[i].x;
    float cy = nodes[i].y;

    // Draw filled circle (light blue)
    glColor3f(0.68f, 0.85f, 0.90f); // light blue
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(cx, cy);
      for (int j = 0; j <= num_segments; j++) {
        float angle = 2.0f * 3.1415926f * j / num_segments;
        float x = cx + cos(angle) * NODE_RADIUS;
        float y = cy + sin(angle) * NODE_RADIUS;
        glVertex2f(x, y);
      }
    glEnd();

    // Draw circle border
    glColor3f(0.0f, 0.0f, 0.0f); // black
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
      for (int j = 0; j <= num_segments; j++) {
        float angle = 2.0f * 3.1415926f * j / num_segments;
        float x = cx + cos(angle) * NODE_RADIUS;
        float y = cy + sin(angle) * NODE_RADIUS;
        glVertex2f(x, y);
      }
    glEnd();

    // Draw label inside circle (rough centering)
    char label[2] = {nodes[i].label, '\0'};
    glColor3f(0.0f, 0.0f, 0.0f);
    // Adjust offsets as needed for centering
    draw_string(cx - 0.008f, cy - 0.008f, label);
  }
}

// Draw edges as bold light pink lines from circle edge to circle edge
void draw_edges() {
  glColor3f(1.0f, 0.71f, 0.76f); // light pink
  glLineWidth(4.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < edge_count; i++) {
    Node src = nodes[edges[i].src];
    Node dest = nodes[edges[i].dest];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0) d = 0.0001f;
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

  // Draw edge weight labels at a perpendicular offset from the midpoint
  for (int i = 0; i < edge_count; i++) {
    Node src = nodes[edges[i].src];
    Node dest = nodes[edges[i].dest];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d == 0) d = 0.0001f;
    float offsetX = (dx / d) * NODE_RADIUS;
    float offsetY = (dy / d) * NODE_RADIUS;
    float startX = src.x + offsetX;
    float startY = src.y + offsetY;
    float endX = dest.x - offsetX;
    float endY = dest.y - offsetY;
    float midX = (startX + endX) / 2;
    float midY = (startY + endY) / 2;

    // Perpendicular offset for label (adjust label_offset as needed)
    float label_offset = 0.03f;
    float perpX = -dy / d; // normalized perpendicular vector x component
    float perpY = dx / d;  // normalized perpendicular vector y component
    float labelX = midX + label_offset * perpX;
    float labelY = midY + label_offset * perpY;

    char weight_str[10];
    sprintf(weight_str, "%.1f", edges[i].weight);
    glColor3f(1.0f, 1.0f, 1.0f); // label in black color
    // Adjust the label's drawing position for rough centering
    draw_string(labelX - 0.015f, labelY - 0.015f, weight_str);
  }
}

// Draw shortest path segments as silver lines (adjusted to avoid nodes)
void draw_shortest_path() {
  if (shortest_path_length < 2)
    return;
  glColor3f(0/255.0f,38/255.0f,99/255.0f); // silver
  glLineWidth(4.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < shortest_path_length - 1; i++) {
    Node src = nodes[shortest_path_nodes[i]];
    Node dest = nodes[shortest_path_nodes[i + 1]];
    float dx = dest.x - src.x;
    float dy = dest.y - src.y;
    float d = sqrt(dx*dx + dy*dy);
    if (d == 0) d = 0.0001f;
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

//
// --- Drawing the side menu ---
//

void draw_menu_pixel() {
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
  glColor3f(0.2f, 0.2f, 0.2f);
  glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(MENU_WIDTH_PIXELS, 0);
    glVertex2i(MENU_WIDTH_PIXELS, h);
    glVertex2i(0, h);
  glEnd();

  int y = 20;
  // Add Node button
  glColor3f(current_mode == MODE_ADD_NODE ? 0.0f : 0.4f, 0.6f,
            current_mode == MODE_ADD_NODE ? 0.9f : 0.4f);
  glBegin(GL_QUADS);
    glVertex2i(10, y);
    glVertex2i(BUTTON_WIDTH + 10, y);
    glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
    glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  draw_string_pixel(15, y + 25, "Add Node");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Add Edge button
  glColor3f(current_mode == MODE_ADD_EDGE ? 0.0f : 0.4f, 0.6f,
            current_mode == MODE_ADD_EDGE ? 0.9f : 0.4f);
  glBegin(GL_QUADS);
    glVertex2i(10, y);
    glVertex2i(BUTTON_WIDTH + 10, y);
    glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
    glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  draw_string_pixel(15, y + 25, "Add Edge");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Shortest Path button
  glColor3f(current_mode == MODE_SHORTEST_PATH ? 0.0f : 0.4f, 0.6f,
            current_mode == MODE_SHORTEST_PATH ? 0.9f : 0.4f);
  glBegin(GL_QUADS);
    glVertex2i(10, y);
    glVertex2i(BUTTON_WIDTH + 10, y);
    glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
    glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  draw_string_pixel(15, y + 25, "Shortest Path");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Edit Weight button
  glColor3f(current_mode == MODE_EDIT_WEIGHT ? 0.0f : 0.4f, 0.6f,
            current_mode == MODE_EDIT_WEIGHT ? 0.9f : 0.4f);
  glBegin(GL_QUADS);
    glVertex2i(10, y);
    glVertex2i(BUTTON_WIDTH + 10, y);
    glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
    glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  draw_string_pixel(15, y + 25, "Edit Weight");

  y += BUTTON_HEIGHT + BUTTON_PADDING;
  // Delete Node button
  glColor3f(current_mode == MODE_DELETE_NODE ? 0.0f : 0.4f, 0.6f,
            current_mode == MODE_DELETE_NODE ? 0.9f : 0.4f);
  glBegin(GL_QUADS);
    glVertex2i(10, y);
    glVertex2i(BUTTON_WIDTH + 10, y);
    glVertex2i(BUTTON_WIDTH + 10, y + BUTTON_HEIGHT);
    glVertex2i(10, y + BUTTON_HEIGHT);
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  draw_string_pixel(15, y + 25, "Delete Node");

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

//
// --- Weight input popup ---
//

void draw_weight_input() {
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  // Draw a small centered input box
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

  // Box background
  glColor3f(1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + box_width, y);
    glVertex2i(x + box_width, y + box_height);
    glVertex2i(x, y + box_height);
  glEnd();

  // Draw border
  glColor3f(0.0f, 0.0f, 0.0f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + box_width, y);
    glVertex2i(x + box_width, y + box_height);
    glVertex2i(x, y + box_height);
  glEnd();

  // Draw prompt text
  char prompt[64];
  char fromChar, toChar;
  if (editing_existing_edge) {
    fromChar = 'A' + edges[editing_edge].src;
    toChar = 'A' + edges[editing_edge].dest;
  } else {
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

//
// --- Utility functions ---
//

int find_node(float x, float y) {
  for (int i = 0; i < node_count; i++) {
    float dx = nodes[i].x - x;
    float dy = nodes[i].y - y;
    if (dx * dx + dy * dy < NODE_RADIUS * NODE_RADIUS)
      return i;
  }
  return -1;
}

void add_edge(int src, int dest, float weight) {
  edges[edge_count++] = (Edge){src, dest, weight};
}

// Compute distance from point (px,py) to segment from (ax,ay) to (bx,by)
float pointToSegmentDistance(float px, float py, float ax, float ay, float bx, float by) {
  float vx = bx - ax, vy = by - ay;
  float wx = px - ax, wy = py - ay;
  float c1 = vx * wx + vy * wy;
  if (c1 <= 0) return sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
  float c2 = vx * vx + vy * vy;
  if (c2 <= c1) return sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
  float bVal = c1 / c2;
  float projx = ax + bVal * vx;
  float projy = ay + bVal * vy;
  return sqrt((px - projx) * (px - projx) + (py - projy) * (py - projy));
}

// Find an edge whose line segment is near the point (x,y)
int find_edge_near(float x, float y) {
  const float threshold = 0.05f; // adjust as needed
  for (int i = 0; i < edge_count; i++) {
    Node a = nodes[edges[i].src];
    Node b = nodes[edges[i].dest];
    float dist = pointToSegmentDistance(x, y, a.x, a.y, b.x, b.y);
    if (dist < threshold)
      return i;
  }
  return -1;
}

//
// --- Dijkstra's algorithm ---
//

void dijkstra(int start, int end) {
  float dist[MAX_NODES];
  bool visited[MAX_NODES];
  int prev[MAX_NODES];
  for (int i = 0; i < node_count; i++) {
    dist[i] = INF;
    visited[i] = false;
    prev[i] = -1;
  }
  dist[start] = 0;

  for (int i = 0; i < node_count; i++) {
    int u = -1;
    float min_dist = INF;
    for (int j = 0; j < node_count; j++) {
      if (!visited[j] && dist[j] < min_dist) {
        u = j;
        min_dist = dist[j];
      }
    }
    if (u == -1)
      break;
    visited[u] = true;

    for (int k = 0; k < edge_count; k++) {
      int v = -1;
      if (edges[k].src == u)
        v = edges[k].dest;
      if (edges[k].dest == u)
        v = edges[k].src;
      if (v != -1 && !visited[v] && dist[u] + edges[k].weight < dist[v]) {
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
  while (at != -1) {
    path[shortest_path_length++] = at;
    at = prev[at];
  }

  for (int i = 0; i < shortest_path_length; i++) {
    shortest_path_nodes[i] = path[shortest_path_length - 1 - i];
  }
}

//
// --- Delete node and update graph ---
//

void delete_node(int node_index) {
  // Remove all edges connected to node_index
  for (int i = 0; i < edge_count; ) {
    if (edges[i].src == node_index || edges[i].dest == node_index) {
      for (int j = i; j < edge_count - 1; j++) {
        edges[j] = edges[j + 1];
      }
      edge_count--;
    } else {
      // Adjust indices for edges coming after the deleted node
      if (edges[i].src > node_index) edges[i].src--;
      if (edges[i].dest > node_index) edges[i].dest--;
      i++;
    }
  }
  // Remove the node from the array and update labels
  for (int i = node_index; i < node_count - 1; i++) {
    nodes[i] = nodes[i + 1];
    nodes[i].label = 'A' + i;
  }
  node_count--;
}

//
// --- Mouse callback ---
//

void mouse(int button, int state, int x, int y) {
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  std::cout << "X: " << x << " Y: " << y << "\n";

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    if (x < MENU_WIDTH_PIXELS) {
      int y_pos = y; // Using window pixel coordinates
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
      glutPostRedisplay();
      return;
    }

    float gl_x = (x / (float)w) * 2.0f - 1.0f;
    float gl_y = 1.0f - (y / (float)h) * 2.0f;

    if (current_mode == MODE_ADD_NODE) {
      if (find_node(gl_x, gl_y) == -1 && node_count < MAX_NODES) {
        nodes[node_count] = (Node){gl_x, gl_y, 'A' + node_count};
        node_count++;
      }
    } else if (current_mode == MODE_ADD_EDGE) {
      int node = find_node(gl_x, gl_y);
      if (node != -1) {
        if (selected_node == -1) {
          selected_node = node;
        } else if (node != selected_node) {
          temp_src = selected_node;
          temp_dest = node;
          inputting_weight = true;
          editing_existing_edge = false;
          weight_input_buffer[0] = '\0';
          selected_node = -1;
        }
      }
    } else if (current_mode == MODE_SHORTEST_PATH) {
      int node = find_node(gl_x, gl_y);
      if (node != -1) {
        if (sp_selected == -1) {
          sp_selected = node;
        } else if (node != sp_selected) {
          dijkstra(sp_selected, node);
          sp_selected = -1;
        }
      }
    } else if (current_mode == MODE_EDIT_WEIGHT) {
      int edge_index = find_edge_near(gl_x, gl_y);
      if (edge_index != -1) {
        editing_edge = edge_index;
        editing_existing_edge = true;
        inputting_weight = true;
        snprintf(weight_input_buffer, sizeof(weight_input_buffer), "%.1f", edges[edge_index].weight);
      }
    } else if (current_mode == MODE_DELETE_NODE) {
      int node = find_node(gl_x, gl_y);
      if (node != -1) {
        delete_node(node);
      }
    }
    glutPostRedisplay();
  }
}

//
// --- Keyboard callback ---
//

void keyboard(unsigned char key, int x, int y) {
  if (inputting_weight) {
    if (key == '\r' || key == '\n') {
      float weight = atof(weight_input_buffer);
      if (weight > 0) {
        if (editing_existing_edge) {
          edges[editing_edge].weight = weight;
          editing_edge = -1;
          editing_existing_edge = false;
        } else {
          add_edge(temp_src, temp_dest, weight);
        }
      }
      inputting_weight = false;
      weight_input_buffer[0] = '\0';
    } else if (key == 8 || key == 127) { // Backspace
      int len = strlen(weight_input_buffer);
      if (len > 0)
        weight_input_buffer[len - 1] = '\0';
    } else if ((isdigit(key) || key == '.') && (strlen(weight_input_buffer) < 31)) {
      if (key == '.' && strchr(weight_input_buffer, '.') != NULL) {
        // Skip adding a second dot
      } else {
        char keyStr[2] = { key, '\0' };
        strncat(weight_input_buffer, keyStr, 1);
      }
    }
    glutPostRedisplay();
  }
}

//
// --- Display callback ---
//

void display() {
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_nodes();
  draw_edges();

  // Only show the shortest path if in shortest path mode
  if (current_mode == MODE_SHORTEST_PATH)
    draw_shortest_path();

  draw_menu_pixel();
  if (inputting_weight)
    draw_weight_input();

  glFlush();
}

//
// --- Main ---
//

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Graph Visualizer");

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutMainLoop();
  return 0;
}

