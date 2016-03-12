#include <QObject>

class PlyModelTest : public QObject
{
    Q_OBJECT;

private slots:
    void parserReturnsInvalidForEmptyFile();
    void parserReturnsInvalidIfFormatMissing();
    void parserReturnsInvalidIfFormatNotAscii();
    void parserReturnsInvalidIfEndHeaderMissing();
    void parserReturnsValidForMinimalFile();
    void minimalFileHasNoElements();
    void parserIgnoresCommentsInHeader();
    void parserReadsElementDefinitions();
    void parserReadsScalarPropertyDefinitions();
    void parserReadsListPropertyDefinitions();
    void parserReturnsInvalidIfPropertyBeforeFirstElement();
    void parserReturnsInvalidIfTooFewElementInstances();
    void parserReadsElementCounts();
    void parserReturnsInvalidIfScalarValueMissing();
    void parserReturnsInvalidIfListIsTooShort();
    void parserReadsScalarPropertyValues();
    void parserReadsListPropertyValues();
};
