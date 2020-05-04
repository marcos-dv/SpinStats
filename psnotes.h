#ifndef PSNOTES_H
#define PSNOTES_H
#include <QList>
#include <QXmlStreamReader>
#include <QMap>
#include <QSet>
#include <QFile>
#include <QDebug>

struct ps_label_t {
    ps_label_t (int id, QString color, QString comment) {
        this->id = id;
        this->color = color;
        this->comment = comment;
    }

    ps_label_t () {}

    QString id;
    QString color;
    QString comment;
};

class ps_notes_reader_t {
public:
    ps_notes_reader_t(const QString & filename) {
        m_filename = filename;
    }

    void read() {
        QFile f(m_filename);
        f.open(QFile::ReadOnly);
        m_xml.setDevice(&f);

        if (m_xml.readNextStartElement()) {
            if (m_xml.name() == "notes")
                read_notes();
            else
                m_warnings += "Notes root element not found\n";
        }

        qDebug() << "Read finished, label count = " << m_labels.count() << " | player count " << m_players.count();
    }

    QList<ps_label_t> * get_labels() { return &m_labels; }

    QMap<QString, QSet<QString>> * get_players () { return &m_players; }

    QString get_warnings() { return m_warnings; }

    bool has_data() {
        if (m_labels.size() != 0 && m_players.size() != 0)
            return true;

        return false;
    }

private:
    void read_notes() {
        Q_ASSERT (m_xml.isStartElement() && m_xml.name() == "notes");

        while (m_xml.readNextStartElement()) {
            if (m_xml.name() == "labels")
                read_labels();
            else if (m_xml.name() == "note")
                read_note();
            else {
                m_warnings += "Unknown element found (read_notes) : " + m_xml.name().toString() + "\n";
                m_xml.skipCurrentElement();
            }
        }
    }

    void read_labels() {
        Q_ASSERT (m_xml.isStartElement() && m_xml.name() == "labels");

        while (m_xml.readNextStartElement()) {
            if (m_xml.name() == "label")
                read_label();
            else {
                m_warnings += "Unknown element found (read_labels) : " + m_xml.name().toString() + "\n";
                m_xml.skipCurrentElement();
            }
        }
    }

    void read_label() {
        Q_ASSERT (m_xml.isStartElement() && m_xml.name() == "label");

        if (!m_xml.attributes().hasAttribute("id") ||
                !m_xml.attributes().hasAttribute("color"))
        {
            m_warnings += "Invalid label found in labels\n";
            m_xml.skipCurrentElement();
            return;
        }

        ps_label_t label;

        label.id = m_xml.attributes().value("id").toString();
        label.color = m_xml.attributes().value("color").toString();
        label.comment = m_xml.readElementText();
        m_labels.append(label);
    }

    void read_note() {
        Q_ASSERT (m_xml.isStartElement() && m_xml.name() == "note");

        if(!m_xml.attributes().hasAttribute("player") ||
                !m_xml.attributes().hasAttribute("label"))
        {
            m_warnings += "Invalid note found\n";
            m_xml.skipCurrentElement();
            return;
        }

        QString name = m_xml.attributes().value("player").toString().toLower();
        QString label = m_xml.attributes().value("label").toString();

        m_players [label].insert(name);
        m_xml.skipCurrentElement();
    }

    QString m_filename;
    QXmlStreamReader m_xml;
    QList<ps_label_t> m_labels;
    QMap<QString, QSet<QString>> m_players;
    QString m_warnings;
};

#endif // PSNOTES_H
