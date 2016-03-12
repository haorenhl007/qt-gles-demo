#include <QTest>

#include "Ply/PlyModelTest.h"

int main()
{
    int result = 0;
    auto runTest = [&result](QObject *test_p) {
        result |= QTest::qExec(test_p);
        delete test_p;
    };

    runTest(new PlyModelTest());

    return result;
}
