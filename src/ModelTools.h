#pragma once

#include <QOpenGLFunctions>

#include "Ply/PlyModel.h"

constexpr int NUM_VERTEX_VALUES = 11;
constexpr int STRIDE = NUM_VERTEX_VALUES * sizeof(GLfloat);

GLfloat *makeGrid(int w, int h, int *vertexCountOut);
GLfloat *convertPly(PlyModel model, int *vertexCountOut);
