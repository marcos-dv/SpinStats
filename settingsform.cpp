#include "settingsform.h"
#include "ui_settingsform.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <windows.h>
#include <QStandardPaths>
#include "customprocessdialog.h"

SettingsForm::SettingsForm() :
    ui(new Ui::SettingsForm)
{
    ui->setupUi(this);

    m_debug_mode = false;
    m_finished = false;

    m_db_user = "postgres";
    m_db_pass = "dbpass";
    m_db_host = "localhost";
    m_db_port = "5432";

    qDebug() << "Searching for file at " << get_settings_path();

    if (QFileInfo (get_settings_path()).exists()) {
        QFile file(get_settings_path());
        file.open (QFile::ReadOnly);
        QTextStream fs(&file);

        qDebug() <<"Found settings file, loading settings";

        while (!fs.atEnd())  {
            QString line = fs.readLine();

            auto args = line.split("==");

            if (args[0] == "db_user")
            {
                ui->usernameEdit->setText(args[1]);
                m_db_user = args[1];
            }
            else if (args [0] == "db_pass") {
                ui->passwordEdit->setText(args[1]);
                m_db_pass = args [1];
            }
            else if (args [0] == "db_host") {
                ui->passwordEdit->setText(args[1]);
                m_db_host = args[1];
            }
            else if (args [0] == "db_port") {
                ui->portEdit->setText(args[1]);
                m_db_port = args[1];
            }
            else if (args [0] == "psql_path") {
                ui->psqlEdit->setText(args[1]);
                m_db_path = args[1];
            }
        }
    }
}

void SettingsForm::closeEvent (QCloseEvent * event) {
    bool save_db = false;

    if (ui->psqlEdit->text().length() == 0) {
        if (QMessageBox::question(this, "Warning", "You must select the PostgreSQL directory. Quit?") == QMessageBox::Yes)
            exit (1);
        else  {
            event->ignore();
            return;
        }
    }

    if (!check_db_connection() &&
            QMessageBox::question(this, "Error", "Failed to connect to PostgreSQL, check your login details. Quit settings?") == QMessageBox::No)
        exit (1);

    else if (ui->usernameEdit->text() != m_db_user ||
            ui->passwordEdit->text() != m_db_pass ||
            ui->hostEdit->text() != m_db_host ||
            ui->portEdit->text() != m_db_port)
    {
        if (QMessageBox::question(this, "Confirm save", "You've modified Database login details,"
                                  "do you want to save them? Keep in mind this is NOT secure "
                                  "so click NO if you do not want to compromise your details") == QMessageBox::Yes)
            save_db = true;
    }

    if (save_db || ui->psqlEdit->text() != m_db_path) {
        QFile file(get_settings_path());
        file.open(QFile::WriteOnly);
        QTextStream stream (&file);

        qDebug() << "Saving settings to " << file.fileName();

        if (save_db) {
            stream << "db_user==" << ui->usernameEdit->text() << "\n";
            stream << "db_pass==" << ui->passwordEdit->text() << "\n";
            stream << "db_host==" << ui->hostEdit->text() << "\n";
            stream << "db_port==" << ui->portEdit->text() << "\n";
        }

        if (ui->psqlEdit->text().length() > 0)
            stream << "psql_path==" << ui->psqlEdit->text() << "\n";
    }

    m_finished = true;
    parent_window->show();
    event->accept();
}

SettingsForm::~SettingsForm()
{
    delete ui;
}

bool SettingsForm::check_psql_dir(QDir path) {
    if (path.entryList().contains("pg_dump.exe") &&
            path.entryList().contains("pg_restore.exe") &&
            path.entryList().contains("createdb.exe") &&
            path.entryList().contains("psql.exe"))
        return true;

    return false;
}

void SettingsForm::on_psqlButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::Detail);

    if (QDir ("C:/Program Files/PostgreSQL").exists())
        dialog.setDirectory("C:/Program Files/PostgreSQL");
    else
        dialog.setDirectory("C:/Program Files");

    dialog.exec();

    QDir path = dialog.selectedFiles().first();

    if (!check_psql_dir(path)) {
        // If user selected outermost psql directory but has more than one version
        if (!path.entryList().contains("bin")) {
            if (path.entryList().size() != 3)
                goto error;

            // Assume we're in the correct outermost directory, navigate to the only version number available
            path.cd(path.entryList().last());

            if (!path.entryList().contains("bin"))
                goto error;
        }

        path.cd ("bin");

        if (!check_psql_dir (path))
            goto error;
    }

    m_psql_path = path.absolutePath();
    ui->psqlEdit->setText(m_psql_path);
    return;

    error:
    QMessageBox::warning(this, "Error",
                         "Invalid directory, please select the PostgreSQL directory"
                         "that contains a directory called 'bin' or select the 'bin'"
                         "directory of your PostgreSQL installation");
}


QString SettingsForm::get_db_user() { return ui->usernameEdit->text(); }
QString SettingsForm::get_db_pass() { return ui->passwordEdit->text(); }
QString SettingsForm::get_db_host() { return ui->hostEdit->text(); }
QString SettingsForm::get_db_port() { return ui->portEdit->text(); }
QString SettingsForm::get_db_path() { return ui->psqlEdit->text(); }

QString SettingsForm::get_psql_util(QString program) {
    QString psql_path = ui->psqlEdit->text();
    if (psql_path.length() == 0)
        return "";

    psql_path += "/" + program;
    if (!program.endsWith(".exe"))
        psql_path += ".exe";

    if (QFileInfo(psql_path).exists())
        return psql_path;

    return "";
}

bool SettingsForm::check_db_connection() {
    CustomProcessDialog dialog("Checking db connection", false, this);
    dialog.add_env_var("PGPASSWORD", get_db_pass());
    if (dialog.start_process(get_psql_util("psql"),
                             QStringList() << "--username=" + get_db_user() << "--list")) {
        dialog.exec();
        if (dialog.proc_ref().exitCode() == 0)
            return true;
    }

    return false;
}

void SettingsForm::on_dbcheckButton_clicked()
{
    if (!check_db_connection())
        QMessageBox::warning(this, "Error", "Failed to connect to PostgreSQL");
    else
        QMessageBox::information(this, "Info", "PostgreSQL connection OK!");
}

QString SettingsForm::get_settings_path() {
    QDir dir (QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!dir.exists())
        dir.mkpath(dir.path());

    return dir.path() + "/settings.txt";
}
