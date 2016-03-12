#include "PlyModelTest.h"

#include <QtTest>

#include "Ply/PlyModel.h"

//=============================================================================
void PlyModelTest::parserReturnsInvalidForEmptyFile()
{
    QTextStream stream("");
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfFormatMissing()
{
    QTextStream stream(
        "ply\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfFormatNotAscii()
{
    QTextStream stream(
        "ply\n"
        "format binary_big_endian 1.0\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfEndHeaderMissing()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsValidForMinimalFile()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(model.isValid());
}

//=============================================================================
void PlyModelTest::minimalFileHasNoElements()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QSet<QString> elements = model.elements();
    QCOMPARE(elements.count(), 0);
}

//=============================================================================
void PlyModelTest::parserIgnoresCommentsInHeader()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "comment blah blah blah\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(model.isValid());
}

//=============================================================================
void PlyModelTest::parserReadsElementDefinitions()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element abc 0\n"
        "element zyx 0\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QSet<QString> elements = model.elements();
    QVERIFY(elements.contains({"abc", "zyx"}));
    QCOMPARE(elements.count(), 2);
}

//=============================================================================
void PlyModelTest::parserReadsScalarPropertyDefinitions()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element abc 0\n"
        "property float foo\n"
        "property float bar\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QSet<QString> props = model.scalarProperties("abc");
    QVERIFY(props.contains({"foo", "bar"}));
    QCOMPARE(props.count(), 2);
}

//=============================================================================
void PlyModelTest::parserReadsListPropertyDefinitions()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element abc 0\n"
        "property list uchar int foo\n"
        "property list int double bar\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QSet<QString> props = model.listProperties("abc");
    QVERIFY(props.contains({"foo", "bar"}));
    QCOMPARE(props.count(), 2);
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfPropertyBeforeFirstElement()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "property float x\n"
        "element abc 0\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfTooFewElementInstances()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element abc 1\n"
        "property float x\n"
        "end_header\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReadsElementCounts()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element abc 3\n"
        "property float x\n"
        "end_header\n"
        "0\n"
        "0\n"
        "0\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QCOMPARE(model.count("abc"), 3);
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfScalarValueMissing()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element vertex 1\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "end_header\n"
        "-1 0\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReturnsInvalidIfListIsTooShort()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element face 1\n"
        "property list uchar int verts\n"
        "end_header\n"
        "4 -1 0 0.5\n"
    );
    PlyModel model = PlyModel::parse(stream);
    QVERIFY(!model.isValid());
}

//=============================================================================
void PlyModelTest::parserReadsScalarPropertyValues()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element vertex 1\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "end_header\n"
        "-1 0 0.5\n"
    );
    PlyModel model = PlyModel::parse(stream);

    QCOMPARE(model.scalarValue("vertex", 0, "x"), -1.0);
    QCOMPARE(model.scalarValue("vertex", 0, "y")+1.0, 1.0);
    QCOMPARE(model.scalarValue("vertex", 0, "z"), 0.5);
}

//=============================================================================
void PlyModelTest::parserReadsListPropertyValues()
{
    QTextStream stream(
        "ply\n"
        "format ascii 1.0\n"
        "element face 1\n"
        "property list uchar int verts\n"
        "end_header\n"
        "3 -1 0 0.5\n"
    );
    PlyModel model = PlyModel::parse(stream);

    QList<double> list = model.listValue("face", 0, "verts");
    QCOMPARE(list.value(0), -1.0);
    QCOMPARE(list.value(1)+1.0, 1.0);
    QCOMPARE(list.value(2), 0.5);
    QCOMPARE(list.count(), 3);
}
