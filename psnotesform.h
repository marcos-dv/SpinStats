#ifndef PSNOTESFORM_H
#define PSNOTESFORM_H

#include <QWidget>
#include "psnotes.h"
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDebug>
#include <QFile>
#include <QDialog>

struct label_ui_t {
    label_ui_t (QString id, QString comment, QString color, QWidget* parent = 0):
        id (id), comment (comment)
    {
        m_gfx_scene = new QGraphicsScene(parent);
        m_gfx_view = new QGraphicsView(parent);
        m_checkbox = new QCheckBox(parent);
        m_label = new QLabel(parent);
        m_layout = new QHBoxLayout;

        if (!color.startsWith("#"))
            color = "#" + color;

        if (color.size() < 7) {
            color = "#" + QString ("0").repeated(7 - color.size()) + color.mid(1);
        }

        QString first = color.mid(1, 2);
        QString second = color.mid(3, 2);
        QString third = color.mid(5, 2);
        color = "#" + third + second + first;

        m_gfx_scene->setBackgroundBrush(QBrush(QColor(color)));
        m_gfx_view->setStyleSheet( "QGraphicsView { border-style: none; }" );
        m_gfx_view->setScene(m_gfx_scene);
        m_gfx_view->setMinimumHeight(20);
        m_gfx_view->setMinimumWidth(20);
        m_gfx_view->setMaximumHeight(20);
        m_gfx_view->setMaximumWidth(20);

        m_checkbox->setText("Save label");
        m_label->setText(comment);

        m_layout->addWidget(m_gfx_view);
        m_layout->addWidget(m_label);
        m_layout->addWidget(m_checkbox);
    }

    ~label_ui_t() {
        delete m_gfx_view;
        delete m_gfx_scene;
        delete m_checkbox;
        delete m_label;
        delete m_layout;
    }

    QHBoxLayout * get_layout() { return m_layout; }

    QCheckBox * m_checkbox;
    QGraphicsScene * m_gfx_scene;
    QGraphicsView * m_gfx_view;
    QLabel * m_label;
    QHBoxLayout * m_layout;

    QString id;
    QString comment;
};

namespace Ui {
class psnotesform;
}

class PSNotesForm : public QDialog
{
    Q_OBJECT

public:
    explicit PSNotesForm(QList<ps_label_t> * labels, QMap<QString, QSet<QString>> * players);
    ~PSNotesForm();

    QList<QString> reg_labels;
private slots:
    void on_save_btn_click();

private:
    Ui::psnotesform *ui;

    QList<label_ui_t*> m_label_uis;

    QList<ps_label_t> * m_ps_labels;
    QMap<QString, QSet<QString>> * m_ps_players;
    QPushButton * m_saveBtn;
};

#endif // PSNOTESFORM_H
