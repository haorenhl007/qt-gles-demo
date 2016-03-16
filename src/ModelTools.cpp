#include "ModelTools.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QVector3D>

//=============================================================================
GLfloat *makeGrid(int w, int h, int *vertexCountOut)
{
    struct Point
    {
        GLfloat x;
        GLfloat y;
    };
    const Point vertexOffsets[] = {
        { 0.0, 0.0 },
        { 1.0, 1.0 },
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0, 0.0 },
        { 1.0, 1.0 }
    };

    const Point textureCoords[] = {
        { 0.0, 0.0 },
        { 1.0, 1.0 },
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0, 0.0 },
        { 1.0, 1.0 }
    };

    const int cellCount = w * h;
    const int vertexCount = cellCount * 6;
    auto data_p = new GLfloat[NUM_VERTEX_VALUES * vertexCount];
    GLfloat *vertex_p = data_p;
    for(int y = -h/2; y < h/2; ++y) {
        for(int x = -w/2; x < w/2; ++x) {
            for(int i = 0; i < 6; ++i) {
                vertex_p[0] = 2*x + 2*vertexOffsets[i].x; // x
                vertex_p[1] = 2*y + 2*vertexOffsets[i].y; // y
                vertex_p[2] = 0.0f; // z
                vertex_p[3] = 0.0f; // nx
                vertex_p[4] = 0.0f; // ny
                vertex_p[5] = 1.0f; // nz
                vertex_p[6] = 0.0f; // fnx
                vertex_p[7] = 0.0f; // fny
                vertex_p[8] = 1.0f; // fnz
                vertex_p[9] = textureCoords[i].x; // s
                vertex_p[10] = textureCoords[i].y; // t
                vertex_p += NUM_VERTEX_VALUES;
            }
        }
    }
    if(vertexCountOut) *vertexCountOut = vertexCount;
    return data_p;
}

//=============================================================================
GLfloat *convertPly(PlyModel model, int *vertexCountOut)
{
    struct Vertex
    {
        double x;
        double y;
        double z;
        double nx;
        double ny;
        double nz;
        double s;
        double t;
    };

    QSet<QString> elements = model.elements();
    if(!elements.contains("vertex")) return nullptr;
    if(!elements.contains("face")) return nullptr;

    QSet<QString> vertexProperties = model.scalarProperties("vertex");
    if(!vertexProperties.contains("x")) return nullptr;
    if(!vertexProperties.contains("y")) return nullptr;
    if(!vertexProperties.contains("z")) return nullptr;
    if(!vertexProperties.contains("nx")) return nullptr;
    if(!vertexProperties.contains("ny")) return nullptr;
    if(!vertexProperties.contains("nz")) return nullptr;
    if(!vertexProperties.contains("s")) return nullptr;
    if(!vertexProperties.contains("t")) return nullptr;

    QSet<QString> faceProperties = model.listProperties("face");
    if(!faceProperties.contains("vertex_indices")) return nullptr;

    QList<Vertex> verts;
    const int vertexCount = model.count("vertex");
    for(int v = 0; v < vertexCount; ++v) {
        Vertex vert;
        vert.x = model.scalarValue("vertex", v, "x");
        vert.y = model.scalarValue("vertex", v, "y");
        vert.z = model.scalarValue("vertex", v, "z");
        vert.nx = model.scalarValue("vertex", v, "nx");
        vert.ny = model.scalarValue("vertex", v, "ny");
        vert.nz = model.scalarValue("vertex", v, "nz");
        vert.s = model.scalarValue("vertex", v, "s");
        vert.t = model.scalarValue("vertex", v, "t");
        verts.append(vert);
    }

    QList<double> face_verts;
    const int faceCount = model.count("face");
    for(int f = 0; f < faceCount; ++f) {
        QList<double> indices = model.listValue("face", f, "vertex_indices");
        if(indices.count() != 3) return nullptr;

        QVector3D faceNormal;
        {
            Vertex v1 = verts.value(indices.value(0));
            Vertex v2 = verts.value(indices.value(1));
            Vertex v3 = verts.value(indices.value(2));
            QVector3D p1(v1.x, v1.y, v1.z);
            QVector3D p2(v2.x, v2.y, v2.z);
            QVector3D p3(v3.x, v3.y, v3.z);
            faceNormal = QVector3D::crossProduct((p2 - p1), (p3 - p2));
        }

        for(int i = 0; i < indices.count(); ++i) {
            int v = (int)indices.value(i);
            if(v >= verts.count()) return nullptr;
            Vertex vert = verts.value(v);
            face_verts.append(vert.x);
            face_verts.append(vert.y);
            face_verts.append(vert.z);
            face_verts.append(vert.nx);
            face_verts.append(vert.ny);
            face_verts.append(vert.nz);
            face_verts.append(faceNormal.x());
            face_verts.append(faceNormal.y());
            face_verts.append(faceNormal.z());
            face_verts.append(vert.s);
            face_verts.append(vert.t);
        }
    }

    GLfloat *data_p = new GLfloat[face_verts.count()];
    for(int i = 0; i < face_verts.count(); ++i) {
        data_p[i] = face_verts.value(i);
    }
    if(vertexCountOut) {
        *vertexCountOut = face_verts.count() / NUM_VERTEX_VALUES;
    }
    return data_p;
}
