#ifndef CUSTOMPROCESSDIALOG_H
#define CUSTOMPROCESSDIALOG_H

#include <QDialog>
#include <QProcess>

namespace Ui {
class CustomProcessDialog;
}

class CustomProcessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomProcessDialog(bool auto_close, QWidget *parent = 0);
    ~CustomProcessDialog();

//    bool start_process(QString exe_path, QStringList args);

//    void add_env_var(QString name, QString value);

public slots:
//    void update_stdout();
//    void update_stderr();
//    void process_finished(int);
    //void process_error_occured (QProcess::ProcessError);

private:
    bool m_auto_close;
    Ui::CustomProcessDialog *ui;

    QProcess m_process;
    QProcessEnvironment m_env;
};

#endif // CUSTOMPROCESSDIALOG_H
