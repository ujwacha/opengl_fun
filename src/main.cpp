#include <GL/freeglut_std.h>
#include <GL/gl.h>
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




void RenderString(float x, float y, void *font, const char* string, RGB rgb) {  
  glColor3f(rgb.r, rgb.g, rgb.b); 
  glRasterPos3f(x, y, 0);

  for (int i = 0; string[i] != '\0'; i++)
    glutBitmapCharacter(font, string[i]);
}

void draw_line(GLfloat cx, GLfloat cy, GLfloat cxx, GLfloat cyy, RGB rgb) {
  glBegin(GL_LINE_LOOP);
  glColor3f(rgb.r, rgb.g, rgb.b); 
  glVertex3f(cx, cy, 0);
  glVertex3f(cxx, cyy, 0);
  glEnd();
}



void draw_poly(GLfloat cx, GLfloat cy, GLfloat r, int num, RGB rgb) {
  float offset_angle = 2 * PI / num;
  glBegin(GL_LINE_LOOP);
  glColor3f(rgb.r, rgb.g, rgb.b); 
  for(int i = 0; i < num; i++) {
    float current_angle = i*offset_angle;
    float x = cx + r*std::cos(current_angle);
    float y = cy + r*std::sin(current_angle);
    glVertex3f(x, y, 0);
  }
  glEnd();
}

void draw_circle(GLfloat cx, GLfloat cy, GLfloat r, RGB rgb) { draw_poly(cx, cy, r, 50, rgb);}


bool is_in(int key, std::vector<Graph_Node*> &nodes) {// linear search O(n) unfortunately

  std::cout << "\tIS IN: "  << nodes.size() << std::endl;

  for (int i = 0; i < nodes.size(); i++) {
    
    std::cout << "nodes[" << i << "]->key = " << nodes[i]->key << std::endl;
    std::cout << "key = " << key << std::endl;
    
    if(nodes[i]->key == key) {
      return true; 
    }
  }
  return false;
}

void draw_edge(Edge* e) {
  draw_line(e->end1->cx, e->end1->cy, e->end2->cx, e->end2->cy, e->color);
}

void draw_graph(Graph_Node *g, bool first = true, bool already_checked = false) {
  static std::vector<Graph_Node*> nodes;
  std::cout << "DRAW GRAPH" << std::endl;;

  if (first) nodes.clear();

  if (!already_checked)
    if(is_in(g->key, nodes)) return;
  
  // Do stuff if the key already exists, ie it's already drawn
  draw_circle(g->cx, g->cy, 0.1, g->color);

  char* format = (char*) malloc(sizeof(char) * 20);
  sprintf(format, "%i", g->value);
  RenderString(g->cx, g->cy, GLUT_BITMAP_TIMES_ROMAN_10, format, g->color); // give formated string instead of "Node"

  nodes.push_back(g);

  for(int i = 0; i < g->edges.size(); i++) { // recursively draw the rest
    Edge current = g->edges[i];
    if (!is_in(current.end1->key, nodes)) {
      draw_graph(current.end1, false, true);
      draw_edge(&current);
    }
    if (!is_in(current.end2->key, nodes)) {
      draw_graph(current.end2, false, true); 
      draw_edge(&current);
    }
  }
}
 
void renderScene(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// glBegin(GL_TRIANGLES);
	// 	glVertex3f(-1,-1,0.0);
	// 	glVertex3f(-1,1,0.0);
	// 	glVertex3f(1,0,0.0);
	// glEnd();


	Graph_Node g = Graph_Node_new(10);

	Graph_Node g2 = Graph_Node_new(20, 0.5, 0.5);

	Edge e = edge_new(&g, &g2);

	g.edges.push_back(e);
	g2.edges.push_back(e);


	
	draw_graph(&g);


	glutSwapBuffers();
}

void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;
	float ratio = 1.0* w / h;

	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);

        // Reset Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	// // Set the correct perspective.

	gluPerspective(0, ratio, 1, 1000);

	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}



int main(int argc, char** argv) {
  std::cout << "Hello GLUT" << std::endl;
 
  // INIT glut
  glutInit(&argc, argv);

  // init glut window
  glutInitWindowSize(1920, 1080);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

  glutCreateWindow("HELLO WORLD");

  // Register Callback
  glutDisplayFunc(renderScene);
  glutReshapeFunc(changeSize);
  
  // main loop
  glutMainLoop();
  

  return 0;
  
}
