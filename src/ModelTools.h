#pragma once

#include <QOpenGLFunctions>
#include <QVector>

#include "Ply/PlyModel.h"

constexpr int NUM_VERTEX_VALUES = 11;
constexpr GLsizei STRIDE = NUM_VERTEX_VALUES * sizeof(GLfloat);
constexpr intptr_t POSITION_OFFSET = 0;
constexpr intptr_t NORMAL_OFFSET = 3 * sizeof(GLfloat);
constexpr intptr_t FACE_NORMAL_OFFSET = 6 * sizeof(GLfloat);
constexpr intptr_t TEXTURE_COORD_OFFSET = 9 * sizeof(GLfloat);

QVector<GLfloat> makeGrid(int w, int h);
QVector<GLfloat> convertPly(PlyModel model);
