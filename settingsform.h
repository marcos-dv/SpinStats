#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QWidget>
#include <QDialog>
#include <QDir>
#include <QPair>
#include "psnotes.h"
#include "psnotesform.h"
#include <QMainWindow>
#include <QCloseEvent>

namespace Ui {
class SettingsForm;
}

class SettingsForm : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsForm();
    ~SettingsForm();

    const QString & get_psql_path() { return m_psql_path; }

    void closeEvent (QCloseEvent * event);

    QString get_db_user();
    QString get_db_pass();
    QString get_db_host();
    QString get_db_port();
    QString get_db_path();

    QString get_psql_util(QString program);

    QMainWindow * parent_window;

    bool check_db_connection();

    bool get_debug_mode() { return m_debug_mode; }

    bool is_finished() { return m_finished; }

private slots:
    void on_psqlButton_clicked();

    void on_dbcheckButton_clicked();

private:
    Ui::SettingsForm *ui;
    QString m_psql_path;


    QString get_settings_path();

    QString m_db_user;
    QString m_db_pass;
    QString m_db_host;
    QString m_db_port;
    QString m_db_path;

    bool m_debug_mode;
    bool m_finished;

    bool check_psql_dir(QDir);
};

#endif // SETTINGSFORM_H
