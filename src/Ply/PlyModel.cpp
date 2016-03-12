#include "PlyModel.h"

#include <QMap>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QTextStream>

//=============================================================================
PlyModel::ElementBuilder::ElementBuilder(const QString& name, int numExpected)
{
    m_elementName = name;
    m_element.m_count = numExpected;
}

//=============================================================================
void PlyModel::ElementBuilder::addScalarProperty(const QString& name)
{
    (void)m_element.m_scalarProperties.insert(name, QList<double>());
    m_propertyNames.append(name);
    (void)m_propertyTypes.insert(name, PropertyType::SCALAR);
}

//=============================================================================
void PlyModel::ElementBuilder::addListProperty(const QString& name)
{
    (void)m_element.m_listProperties.insert(name, QList<QList<double>>());
    m_propertyNames.append(name);
    (void)m_propertyTypes.insert(name, PropertyType::LIST);
}

//=============================================================================
bool PlyModel::ElementBuilder::addInstance(const QStringList& words)
{
    QList<double> values;
    for(int i = 0; i < words.count(); ++i) {
        bool ok = false;
        double value = words.value(i).toDouble(&ok);
        if(!ok) return false;
        values.append(value);
    }

    for(auto property : m_propertyNames) {
        if(values.isEmpty()) return false;
        switch(m_propertyTypes.value(property)) {
        case PropertyType::SCALAR:
            {
                double value = values.takeFirst();
                m_element.m_scalarProperties[property].append(value);
            } break;
        case PropertyType::LIST:
            {
                int count = (int)values.takeFirst();
                QList<double> list;
                for(int i = 0; i < count; ++i) {
                    if(values.isEmpty()) return false;
                    list.append(values.takeFirst());
                }
                m_element.m_listProperties[property].append(list);
            } break;
        }
    }
    return true;
}

//=============================================================================
PlyModel PlyModel::parse(QTextStream& stream)
{
    const QString headerBegin = stream.readLine();
    if(headerBegin.isNull() || headerBegin != "ply") return PlyModel();

    const QString format = stream.readLine();
    if(format.isNull() || format != "format ascii 1.0") return PlyModel();

    QList<QSharedPointer<ElementBuilder>> builders;
    QSharedPointer<ElementBuilder> currentBuilder_p;

    while(true) {
        QString line = stream.readLine();
        if(line.isNull()) return PlyModel();

        QStringList words = line.trimmed().split(QRegularExpression("\\s+"));
        if(words.isEmpty()) return PlyModel();

        QString command = words.first();
        if(command == "end_header") {
            break;
        } else if(command == "element") {
            if(words.count() != 3) return PlyModel();
            bool ok = false;
            int count = words.value(2).toInt(&ok);
            if(!ok) return PlyModel();
            currentBuilder_p = QSharedPointer<ElementBuilder>::create(
                    words.value(1), count);
            builders.append(currentBuilder_p);
        } else if(command == "property") {
            if(currentBuilder_p.isNull()) return PlyModel();
            if(words.count() < 2) return PlyModel();
            if(words.value(1) == "list") {
                if(words.count() < 5) return PlyModel();
                currentBuilder_p->addListProperty(words.value(4));
            } else {
                if(words.count() < 3) return PlyModel();
                currentBuilder_p->addScalarProperty(words.value(2));
            }
        } else if(command == "comment") {
        } else {
            return PlyModel();
        }
    }

    PlyModel model;

    for(auto builder_p : builders) {
        for(int i = 0; i < builder_p->numExpected(); ++i) {
            QString line = stream.readLine();
            if(line.isNull()) return PlyModel();
            QStringList words =
                    line.trimmed().split(QRegularExpression("\\s+"));
            if(!builder_p->addInstance(words)) return PlyModel();
        }
        (void)model.m_elements.insert(
                builder_p->elementName(), builder_p->element());
    }

    model.m_valid = true;
    return model;
}

//=============================================================================
PlyModel::PlyModel() : m_valid(false)
{
}

//=============================================================================
bool PlyModel::isValid() const
{
    return m_valid;
}

//=============================================================================
QSet<QString> PlyModel::elements() const
{
    return m_elements.keys().toSet();
}

//=============================================================================
int PlyModel::count(const QString& element) const
{
    return m_elements.value(element).m_count;
}

//=============================================================================
QSet<QString> PlyModel::scalarProperties(const QString& element) const
{
    return m_elements.value(element).m_scalarProperties.keys().toSet();
}

//=============================================================================
QSet<QString> PlyModel::listProperties(const QString& element) const
{
    return m_elements.value(element).m_listProperties.keys().toSet();
}

//=============================================================================
double PlyModel::scalarValue(const QString& element, int index,
        const QString& property) const
{
    Element e = m_elements.value(element);
    return e.m_scalarProperties.value(property).value(index);
}

//=============================================================================
QList<double> PlyModel::listValue(const QString& element, int index,
        const QString& property) const
{
    Element e = m_elements.value(element);
    return e.m_listProperties.value(property).value(index);
}
