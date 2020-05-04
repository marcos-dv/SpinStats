#include "pt4loader.h"
#include "ui_pt4loader.h"
#include <QPair>

PT4Loader::PT4Loader(QList<QPair<int, int> > *labels, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PT4Loader)
{
    ui->setupUi(this);

    QMap<int, QString> color_map;
    color_map[0] = "#000000";
    color_map[1] = "#FFDB30";
    color_map[2] = "#97FF30";
    color_map[3] = "#80FFFF";
    color_map[4] = "#309BFF";
    color_map[5] = "#4E30FF";
    color_map[6] = "#D730FF";
    color_map[7] = "#FF3E30";
    color_map[8] = "#FF8519";

    foreach (auto label, *labels) {
        int id = label.first;
        auto label_ui = new label_ui_t (QString::number(id), QString::number(label.second) + " players", color_map[id]);

        ui->verticalLayout_3->addLayout(label_ui->get_layout());
        m_label_uis.append(label_ui);
    }

    m_save_btn = new QPushButton(this);
    m_save_btn->setMinimumWidth(100);
    m_save_btn->setMaximumWidth(100);
    m_save_btn->setText("Save");

    QObject::connect(m_save_btn, SIGNAL(clicked(bool)), this, SLOT(save_btn_click_slot()));

    ui->verticalLayout_3->addWidget(m_save_btn);
    ui->verticalLayout_3->setAlignment(m_save_btn, Qt::AlignHCenter);
}

void PT4Loader::update_reg_labels() {
    foreach (auto label_ui, m_label_uis) {
        if (label_ui->m_checkbox->isChecked())
            reg_labels.append(label_ui->id.toInt());
    }
}

void PT4Loader::save_btn_click_slot() {
    this->hide();
}

PT4Loader::~PT4Loader()
{
    foreach (auto label_ui, m_label_uis)
        delete label_ui;

    delete m_save_btn;
    delete ui;
}
