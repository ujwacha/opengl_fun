#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <math.h>
#include <filesystem>
#include "graph.hpp"
#include <format>

#include <GL/glut.h>
#include <GL/glu.h>
#include <vector>

#define PI 3.14159
#define CIRCLE_RAD 0.1

// Dracula Theme Colors
RGB dracula_bg(0.15686f, 0.1647f, 0.21176f);    // #282A36
RGB dracula_fg(0.9725f, 0.9725f, 0.9490f);     // #F8F8F2
RGB dracula_pink(1.0f, 0.4745f, 0.7764f);      // #FF79C6
RGB dracula_green(0.3137f, 0.9804f, 0.4824f);  // #50FA7B
RGB dracula_yellow(0.9451f, 0.9804f, 0.5490f); // #F1FA8C
RGB dracula_cyan(0.5451f, 0.9137f, 0.9922f);   // #8BE9FD

void RenderString(float x, float y, void *font, const char* string, RGB rgb) {  
  glColor3f(rgb.r, rgb.g, rgb.b); 
  glRasterPos3f(x, y, 0);

  for (int i = 0; string[i] != '\0'; i++)
    glutBitmapCharacter(font, string[i]);
}

void draw_line(GLfloat cx, GLfloat cy, GLfloat cxx, GLfloat cyy, RGB rgb) {
  glLineWidth(3.0f); 
  
  glBegin(GL_LINE_LOOP);
  glColor3f(rgb.r, rgb.g, rgb.b); 
  glVertex3f(cx, cy, 0);
  glVertex3f(cxx, cyy, 0);
  glEnd();

  glLineWidth(1.0f); 
}


void draw_poly(GLfloat cx, GLfloat cy, GLfloat r, int num, RGB rgb) {
  float offset_angle = 2 * PI / num;
  glBegin(GL_POLYGON);
  glColor3f(rgb.r, rgb.g, rgb.b); 
  for(int i = 0; i < num; i++) {
    float current_angle = i*offset_angle;
    float x = cx + r*std::cos(current_angle);
    float y = cy + r*std::sin(current_angle);
    glVertex3f(x, y, 0);
  }
  glEnd();
}

void draw_circle(GLfloat cx, GLfloat cy, GLfloat r, RGB rgb) {
  draw_poly(cx, cy, r, 50, rgb);
}

bool is_in(int key, std::vector<Graph_Node*> &nodes) {
  for (Graph_Node* node : nodes) {
    if(node->key == key) return true; 
  }
  return false;
}

void draw_edge(Edge* e) {
  // make vector and unit vector
  float vx = e->end2->cx - e->end1->cx;
  float vy = e->end2->cy - e->end1->cy;

  float magnetude = std::sqrt(vx*vx + vy*vy);
  vx/= magnetude;
  vy/= magnetude;


  draw_line(e->end1->cx + vx*CIRCLE_RAD,
	    e->end1->cy + vy*CIRCLE_RAD,
	    e->end2->cx - vx*CIRCLE_RAD,
	    e->end2->cy - vy*CIRCLE_RAD,
	    e->color);
}

void draw_graph(Graph_Node *g, bool first = true, bool already_checked = false) {
  static std::vector<Graph_Node*> nodes;
  if (first) nodes.clear();
  if (!already_checked && is_in(g->key, nodes)) return;
  
  draw_circle(g->cx, g->cy, CIRCLE_RAD, g->color);
  
  char format[20];
  sprintf(format, "%i", g->value);
  RenderString(g->cx, g->cy, GLUT_BITMAP_TIMES_ROMAN_24, format, dracula_pink);

  nodes.push_back(g);

  for(Edge& current : g->edges) {
    draw_edge(&current);

    if (!is_in(current.end1->key, nodes)) {
      draw_graph(current.end1, false, true);
    }
    if (!is_in(current.end2->key, nodes)) {
      draw_graph(current.end2, false, true); 
    }
  }
}

void connect_graphs(Graph_Node* g1, Graph_Node* g2) {
  Edge e = edge_new(g1, g2, dracula_pink);
  g1->edges.push_back(e);
  g2->edges.push_back(e);
};

void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Create nodes with Dracula colors
    Graph_Node g = Graph_Node_new(10, -0.5f, 0.0f, dracula_cyan);
    Graph_Node g2 = Graph_Node_new(20, 0.5f, 0.0f, dracula_cyan);
    Graph_Node g3 = Graph_Node_new(20, 0.5f, 0.5f, dracula_cyan);

    // Create edge with Dracula pink

    connect_graphs(&g, &g2);
    connect_graphs(&g, &g3);
    connect_graphs(&g2, &g3);
   
    draw_graph(&g);

    glutSwapBuffers();
}

void changeSize(int w, int h) {
    if(h == 0) h = 1;
    float ratio = 1.0f * w / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(0, ratio, 1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitWindowSize(1920, 1080);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("Dracula Theme Graph");
  glutDisplayFunc(renderScene);
  glutReshapeFunc(changeSize);
  
  // Set Dracula background color
  glClearColor(dracula_bg.r, dracula_bg.g, dracula_bg.b, 1.0f);
  
  glutMainLoop();
  return 0;
}
