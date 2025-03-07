#include <iostream>
#include <vector>

class RGB {
public:
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

  RGB() {
    r = 1;
    g = 0;
    b = 0;
    a = 0;
  }

};



struct Graph_Node;

class Edge {
public: 
  Graph_Node* end1;
  Graph_Node* end2;
  RGB color;

};

class Graph_Node {
public:
  char key; // key to assign
  int value;
  std::vector<Edge> edges;
  RGB color;
  float cx, cy;
};


Edge edge_new(Graph_Node *first, Graph_Node *second, RGB color = RGB(1, 0, 0, 0)) {
  Edge e;
  e.color = color;
  e.end1 = first;
  e.end2 = second;
  return e;
}

Graph_Node Graph_Node_new(int value, float cx = 0, float cy = 0, RGB color = RGB(1, 0, 0, 0)) {
  static int key_current = 0;

  Graph_Node g;
  g.value = value;
  g.color = color;
  g.cx = cx;
  g.cy = cy;
  g.key = key_current++;

  return g;
}

