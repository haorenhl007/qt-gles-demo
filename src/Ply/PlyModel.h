#include <QMap>
#include <QSet>
#include <QString>

class QTextStream;

class PlyModel
{
public:
    static PlyModel parse(QTextStream& stream);

    PlyModel();

    bool isValid() const;
    QSet<QString> elements() const;
    int count(const QString& element) const;
    QSet<QString> scalarProperties(const QString& element) const;
    QSet<QString> listProperties(const QString& element) const;
    double scalarValue(const QString& element, int index,
            const QString& property) const;
    QList<double> listValue(const QString& element, int index,
            const QString& property) const;

private:
    class Element
    {
    public:
        Element() : m_count(0) {}

        int m_count;
        QMap<QString, QList<double>> m_scalarProperties;
        QMap<QString, QList<QList<double>>> m_listProperties;
    };

    class ElementBuilder {
    public:
        ElementBuilder(const QString& name, int numExpected);

        QString elementName() { return m_elementName; }
        int numExpected() { return m_element.m_count; }
        Element element() { return m_element; }

        void addScalarProperty(const QString& name);
        void addListProperty(const QString& name);
        bool addInstance(const QStringList& words);

    private:
        enum class PropertyType {
            SCALAR,
            LIST
        };

        QString m_elementName;
        Element m_element;
        QList<QString> m_propertyNames;
        QMap<QString, PropertyType> m_propertyTypes;
    };

    bool m_valid;
    QMap<QString, Element> m_elements;
};
