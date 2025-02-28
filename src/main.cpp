#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <iostream>
#include <cstring>
#include <math.h>
#include <filesystem>


#include <GL/glut.h>
#include <GL/glu.h>


#define PI 3.14159


struct RGB {
  float r;
  float g;
  float b;
  float a;

  RGB(float r_, float g_, float b_, float a_ = 0.0f) {
    r = r_;
    g = g_;
    b = b_;
    a = a_;
  }

};


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

void draw_circle(GLfloat cx, GLfloat cy, GLfloat r, RGB rgb) { draw_poly(cx, cy, r, 50, rgb);}


void renderScene(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// glBegin(GL_TRIANGLES);
	// 	glVertex3f(-1,-1,0.0);
	// 	glVertex3f(-1,1,0.0);
	// 	glVertex3f(1,0,0.0);
	// glEnd();

	draw_line(0, 0, 1, 1, RGB(1, 1, 1));

	draw_poly(0, 0, 0.5, 50, RGB(0, 0, 1));

	draw_circle(1, 1, 0.2, RGB(0, 1, 0));

	RenderString(0, 0, GLUT_BITMAP_TIMES_ROMAN_10, "HELLO GLUT", RGB(1, 0, 0));
	
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
