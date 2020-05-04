#ifndef PT4LOADER_H
#define PT4LOADER_H

#include <QDialog>
#include "psnotesform.h"

namespace Ui {
class PT4Loader;
}

class PT4Loader : public QDialog
{
    Q_OBJECT

public:
    explicit PT4Loader(QList<QPair<int, int>> *labels, QWidget *parent = 0);
    ~PT4Loader();

    QList<int> reg_labels;

    void update_reg_labels();

private slots:
    void save_btn_click_slot();

private:
    Ui::PT4Loader *ui;

    QList<label_ui_t*> m_label_uis;
    QPushButton * m_save_btn;
};

#endif // PT4LOADER_H
