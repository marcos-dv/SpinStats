#include "customprocessdialog.h"
#include "ui_customprocessdialog.h"
#include <QFileInfo>
#include <QMessageBox>
//#include <QMetaEnum>

CustomProcessDialog::CustomProcessDialog(bool auto_close, QWidget *parent) :
    QDialog(parent),
    m_auto_close(auto_close),
    ui(new Ui::CustomProcessDialog)
{
    ui->setupUi(this);
}

CustomProcessDialog::~CustomProcessDialog()
{
    delete ui;
}

/*bool CustomProcessDialog::start_process(QString exe_path, QStringList args) {
    if (!QFileInfo(exe_path).exists()) {
        QMessageBox::warning(this, "Error", "Can not start process '" + exe_path + "' because file does not exist");
        return false;
    }

    m_process.setProcessEnvironment(m_env);
    m_process.start(exe_path, args);

    connect (&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(update_stdout()));
    connect (&m_process, SIGNAL(readyReadStandardError()), this, SLOT (update_stderr()));
    connect (&m_process, SIGNAL(finished(int)), this, SLOT (process_finished()));
    //connect (&m_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(process_error_occured(QProcess::ProcessError)));

    return true;
}

void CustomProcessDialog::process_error_occured(QProcess::ProcessError error) {
    QString error_type = QMetaEnum::fromType<QProcess::ProcessError>().valueToKey(error);
    QMessageBox::warning(this, "Error", "Process experienced the following error : " + error_type + " | " + m_process.errorString());
}

void CustomProcessDialog::update_stderr() {
    ui->stderrEdit->appendPlainText(m_process.readAllStandardError());
}

void CustomProcessDialog::update_stdout() {
    ui->stdoutEdit->appendPlainText(m_process.readAllStandardOutput());
}

void CustomProcessDialog::add_env_var(QString name, QString value) {
    m_env.insert(name, value);
}

void CustomProcessDialog::process_finished(int exit_code) {
    ui->stdoutEdit->appendPlainText("----Process finished with exit code" + QString::number(exit_code));
    ui->stderrEdit->appendPlainText("----Process finished with exit code" + QString::number(exit_code));
}
*/
