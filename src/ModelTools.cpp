#include "ModelTools.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QVector3D>

//=============================================================================
QVector<GLfloat> makeGrid(int w, int h)
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

    QVector<GLfloat> data;
    for(int y = -h/2; y < h/2; ++y) {
        for(int x = -w/2; x < w/2; ++x) {
            for(int i = 0; i < 6; ++i) {
                data.append(2*x + 2*vertexOffsets[i].x); // x
                data.append(2*y + 2*vertexOffsets[i].y); // y
                data.append(0.0f); // z
                data.append(0.0f); // nx
                data.append(0.0f); // ny
                data.append(1.0f); // nz
                data.append(0.0f); // fnx
                data.append(0.0f); // fny
                data.append(1.0f); // fnz
                data.append(textureCoords[i].x); // s
                data.append(textureCoords[i].y); // t
            }
        }
    }
    return data;
}

//=============================================================================
QVector<GLfloat> convertPly(PlyModel model)
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
    if(!elements.contains("vertex")) return QVector<GLfloat>();
    if(!elements.contains("face")) return QVector<GLfloat>();

    QSet<QString> vertexProperties = model.scalarProperties("vertex");
    if(!vertexProperties.contains("x")) return QVector<GLfloat>();
    if(!vertexProperties.contains("y")) return QVector<GLfloat>();
    if(!vertexProperties.contains("z")) return QVector<GLfloat>();
    if(!vertexProperties.contains("nx")) return QVector<GLfloat>();
    if(!vertexProperties.contains("ny")) return QVector<GLfloat>();
    if(!vertexProperties.contains("nz")) return QVector<GLfloat>();
    if(!vertexProperties.contains("s")) return QVector<GLfloat>();
    if(!vertexProperties.contains("t")) return QVector<GLfloat>();

    QSet<QString> faceProperties = model.listProperties("face");
    if(!faceProperties.contains("vertex_indices")) return QVector<GLfloat>();

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

    QVector<GLfloat> face_verts;
    const int faceCount = model.count("face");
    for(int f = 0; f < faceCount; ++f) {
        QList<double> indices = model.listValue("face", f, "vertex_indices");
        if(indices.count() != 3) return QVector<GLfloat>();

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
            if(v >= verts.count()) return QVector<GLfloat>();
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

    return face_verts;
}
