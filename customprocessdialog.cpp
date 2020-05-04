#include "customprocessdialog.h"
#include "ui_customprocessdialog.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaEnum>
#include <QDebug>
#include <QDesktopWidget>
#include <QTimer>
#include <QStyle> // Qt 4.11

CustomProcessDialog::CustomProcessDialog(QString action, bool debug_mode = false, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomProcessDialog)
{
    ui->setupUi(this);
    ui->actionLabel->setText(action);

    QObject::connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tab_change(int)));

    m_debug_mode = debug_mode;

    if (debug_mode)
        ui->tabWidget->setCurrentIndex(1);
    else {
        ui->debugEdit->hide();
        ui->stdoutEdit->hide();
        ui->stderrEdit->hide();
        this->setMinimumHeight(80);
        this->setMaximumHeight(80);
        int w = ui->actionLabel->sizeHint().width() + 80;
        this->resize(w, 80);
        this->setMinimumWidth(w);
        this->setMaximumWidth(w);
    }

    m_loading_movie.reset(new QMovie(":/resources/resources/loader.gif"));
    ui->loadingLabel->setMovie(m_loading_movie.get());
    m_loading_movie->start();
}

CustomProcessDialog::~CustomProcessDialog()
{
    delete ui;
}

void CustomProcessDialog::tab_change(int index) {
    if (index == 0)
    {
        ui->debugEdit->hide(); ui->stdoutEdit->hide(); ui->stderrEdit->hide(); this->setMinimumHeight(80);

        int w = ui->actionLabel->sizeHint().width() + 80;
        this->setMinimumWidth(w);
        this->setMaximumWidth(w);
        this->setMaximumHeight(80);

        this->resize(w, 80);

        this->setGeometry(QStyle::alignedRect( Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->desktop()->availableGeometry()));
    }
    else {
        ui->debugEdit->show(); ui->stdoutEdit->show(); ui->stderrEdit->show();

        this->setMinimumHeight(500);
        this->setMinimumWidth(600);

        this->setGeometry(QStyle::alignedRect( Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->desktop()->availableGeometry()));
    }
}

bool CustomProcessDialog::start_process(QString exe_path, QStringList args) {
    if (!QFileInfo(exe_path).exists()) {
        QMessageBox::warning((QWidget*)this->parent(),"Error", "Can not start process '" + exe_path + "' because file does not exist");
        return false;
    }

    m_process.setProcessEnvironment(m_env);

    connect (&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(update_stdout()));
    connect (&m_process, SIGNAL(readyReadStandardError()), this, SLOT (update_stderr()));
    connect (&m_process, SIGNAL(finished(int)), this, SLOT (process_finished(int)));
    connect (&m_process, SIGNAL(started()), this, SLOT (process_started()));
    connect (&m_process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(state_changed(QProcess::ProcessState)));
    connect (&m_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(process_error_occured(QProcess::ProcessError)));

    QString args_str;
    foreach (auto arg, args)
        args_str += "\"" + arg + "\" ";

    ui->debugEdit->appendPlainText("Starting \"" + exe_path + "\" " + args_str);
    m_process.start(exe_path, args);
    m_timer.start();
    return true;
}

void CustomProcessDialog::process_started() {
    m_process.closeWriteChannel();
}

void CustomProcessDialog::process_error_occured(QProcess::ProcessError error) {
    QString error_type = QMetaEnum::fromType<QProcess::ProcessError>().valueToKey(error);
    QMessageBox::warning(this, "Error", "Process experienced the following error : " + error_type + " | " + m_process.errorString());
}

void CustomProcessDialog::update_stderr() {
    QString new_output = m_process.readAllStandardError();
    ui->stderrEdit->appendPlainText(new_output);
    m_stderr += new_output;
}

void CustomProcessDialog::update_stdout() {
    QString new_output = m_process.readAllStandardOutput();
    ui->stdoutEdit->appendPlainText(new_output);
    m_stdout += new_output;
}

void CustomProcessDialog::add_env_var(QString name, QString value) {
    m_env.insert(name, value);
}

void CustomProcessDialog::process_finished(int exit_code) {
    ui->debugEdit->appendPlainText("----Process finished with exit code " + QString::number(exit_code) + " , execution took " + QString::number(m_timer.elapsed() / 1000.0) + " seconds");

    if (m_debug_mode) {
        if (QMessageBox::Yes == QMessageBox::question(this, "", "Process finished, close?"))
            this->close();
    }
    else {
        if (exit_code != 0 || m_process.error() != QProcess::UnknownError) {
            if (QMessageBox::question(this, "Error", "Process might have encountered an error, review debug info?") == QMessageBox::Yes)
                return;
        }

        if (m_timer.elapsed() < 1200)
            QTimer::singleShot(1200 - m_timer.elapsed(), this, SLOT(close()));
        else
            this->close();
    }
}

void CustomProcessDialog::state_changed(QProcess::ProcessState new_state) {
    ui->debugEdit->appendPlainText("State changed : " + QString (QMetaEnum::fromType<QProcess::ProcessState>().valueToKey(new_state)));
}
