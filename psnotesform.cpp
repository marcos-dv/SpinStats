#include "psnotesform.h"
#include "ui_psnotesform.h"
#include <QMessageBox>

PSNotesForm::PSNotesForm(QList<ps_label_t> * labels, QMap<QString, QSet<QString>> * players):
    ui(new Ui::psnotesform),
    m_ps_labels(labels),
    m_ps_players(players)
{
    ui->setupUi(this);

    foreach (auto label, *m_ps_labels) {
        auto label_ui = new label_ui_t (label.id, label.comment, label.color, this);

        ui->verticalLayout->addLayout(label_ui->get_layout());

        m_label_uis.append(label_ui);
    }

    m_saveBtn = new QPushButton(this);
    m_saveBtn->setMinimumWidth(100);
    m_saveBtn->setMaximumWidth(100);
    m_saveBtn->setText("Save");

    QObject::connect(m_saveBtn, SIGNAL(clicked(bool)), this, SLOT(on_save_btn_click()));

    ui->verticalLayout->addWidget(m_saveBtn);
    ui->verticalLayout->setAlignment(m_saveBtn, Qt::AlignHCenter);
}

void PSNotesForm::on_save_btn_click() {
    int reg_count = 0;

    foreach (auto label_ui, m_label_uis) {
        if (label_ui->m_checkbox->isChecked()) {
            reg_labels.append(label_ui->id);
            reg_count += m_ps_players->value(label_ui->id).size();
        }
    }

    auto answer = QMessageBox::question(this, "Confirmation",
                                        "There are " + QString::number(reg_count) + " players matching the selected labels, save them in the reg list?");

    if (answer == QMessageBox::Yes) {
        this->hide();
    }
    else
        reg_labels.clear();
}

PSNotesForm::~PSNotesForm()
{
    foreach (auto label_ui, m_label_uis)
        delete label_ui;

    delete m_saveBtn;
    delete ui;
}
