#ifndef CUSTOMPROCESSDIALOG_H
#define CUSTOMPROCESSDIALOG_H

#include <QDialog>
#include <QProcess>
#include <QTime>
#include <QMovie>
#include <memory>

namespace Ui {
class CustomProcessDialog;
}

class CustomProcessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomProcessDialog(QString action, bool debug_mode, QWidget *parent = 0);
    ~CustomProcessDialog();

    bool start_process(QString exe_path, QStringList args);

    void add_env_var(QString name, QString value);

public slots:
    void update_stdout();
    void update_stderr();
    void process_started();
    void process_finished(int);
    void process_error_occured (QProcess::ProcessError);

    void state_changed(QProcess::ProcessState);

    void tab_change(int index);

    QProcess & proc_ref() { return m_process; }

    QString * get_stdout() { return &m_stdout; }
    QString * get_stderr() { return &m_stderr; }

private:
    Ui::CustomProcessDialog *ui;

    QProcess m_process;
    QProcessEnvironment m_env;

    bool m_debug_mode;

    QString m_stdout;
    QString m_stderr;

    QTime m_timer;
    std::shared_ptr<QMovie> m_loading_movie;
};

#endif // CUSTOMPROCESSDIALOG_H
