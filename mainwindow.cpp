#include "mainwindow.h"
#include "chartwindow.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QDesktopWidget>
#include <QStringBuilder>
#include <QTemporaryDir>
#include <customprocessdialog.h>
#include <QFileDialog>
#include <QInputDialog>
#include <QMovie>
#include <QPixmap>
#include "pt4loader.h"
// Drawing charts libs
#include <QtCharts>
// File writing libs
#include <QFile>
#include <QTextStream>

#define mydebug(x) fprintf(stderr, "-%s \n", x)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    chart_window_abs = NULL;
    chart_window_rel = NULL;
    chart_window_tmp= NULL;

    m_settings.parent_window = this;
    m_loading_tournies = false;

    QTimer::singleShot(0, this, SLOT(launch_settings()));

    ui->databaseCombo->addItem("");

    m_db = QSqlDatabase::addDatabase("QPSQL");

    ui->loadingLabel->hide();

    connect(ui->from_date_edit, &QDateEdit::dateChanged, [=](const QDate &date) {
        QString db_name = ui->databaseCombo->currentText();
        auto & database = m_databases [db_name];
        if (db_name == "" || date > database.tournies.last().ts_start.date())
            return;

        QSettings settings;
        settings.setValue("db/" + db_name + "/from_date", date);
    });

    connect(ui->to_date_edit, &QDateEdit::dateChanged, [=](const QDate &date) {
        QString db_name = ui->databaseCombo->currentText();
        auto & database = m_databases [db_name];
        //if (settings.contains("db/" + db_name + "/to_date"))
        //    qDebug() << "New to_date_edit value : " << date << " | current settings value = " << settings.value("db/"+db_name+"/to_date").toDate();

        if (db_name == "" || date < database.tournies.first().ts_start.date())
            return;

        QSettings settings;
        settings.setValue("db/" + db_name + "/to_date", date);
    });
}

void MainWindow::launch_settings()
{
    this->hide();
    m_settings.show();
}

MainWindow::~MainWindow()
{
    if (chart_window_abs) {
        chart_window_abs->hide();
        delete chart_window_abs;
    }
    if (chart_window_rel) {
        chart_window_rel->hide();
        delete chart_window_rel;
    }
    delete ui;
}


bool MainWindow::split_db (QString src_db, QString new_db, QString dump_file, QList<int> tourney_ids)
{
    int return_value = true;

    CustomProcessDialog create_db_dialog("Creating new database", m_settings.get_debug_mode(), this);
    create_db_dialog.add_env_var("PGPASSWORD", m_settings.get_db_pass());
    if (create_db_dialog.start_process(m_settings.get_psql_util("createdb"),
                                   QStringList() << "--no-password" << "--username=" + m_settings.get_db_user() << new_db))
        create_db_dialog.exec();
    else {
        QMessageBox::warning (this, "Error", "Failed to start createdb process");
        return false;
    }

    if (create_db_dialog.proc_ref().exitCode() != 0) {
        QMessageBox::warning(this, "Error", "Failed to create new database");
        return false;
    }


    CustomProcessDialog restore_db_dialog("Restoring dump into new database", m_settings.get_debug_mode(), this);
    restore_db_dialog.add_env_var("PGPASSWORD", m_settings.get_db_pass());
    if (restore_db_dialog.start_process(m_settings.get_psql_util("pg_restore"),
                                        QStringList() << "--no-password" << "--username=" + m_settings.get_db_user()
                                        << "-d" << new_db << "-j2" << "-v" << dump_file))
        restore_db_dialog.exec();
    else {
        QMessageBox::warning(this, "Error", "Failed to start pg_restore process");
        return false;
    }

    if (restore_db_dialog.proc_ref().exitCode() != 0) {
        QMessageBox::warning(this, "Error", "Failed to restore dump into new database, clone failed");
        return false;
    }

    if (m_db.isOpen())
        m_db.close();

    m_db.setDatabaseName(new_db);

    if (!m_db.open()) {
        QMessageBox::warning(this, "Error", "Failed to connect to cloned DB : " + new_db);
        m_db.setDatabaseName(src_db);

        if(!m_db.open()) {
            QMessageBox::critical(this, "Error", "Failed to reconnect to source DB, exiting");
            exit (0);
        }
    }

    QTime timer;
    timer.start();
    qDebug() << "Deleting " << tourney_ids.count() << " tournies";
    m_db.transaction();

    QWidget info_widget;
    std::shared_ptr<QHBoxLayout> layout(new QHBoxLayout);
    std::shared_ptr<QLabel> icon_lbl(new QLabel(&info_widget));
    std::shared_ptr<QLabel> action_lbl(new QLabel(&info_widget));
    std::shared_ptr<QMovie> movie(new QMovie(":/resources/resources/loading.gif"));

    info_widget.setWindowTitle("Progress");
    movie->start();
    icon_lbl->setMovie(movie.get());

    layout->addWidget(icon_lbl.get());
    layout->addWidget(action_lbl.get());
    info_widget.setLayout(layout.get());

    info_widget.resize(120, 35);

    info_widget.show();

    info_widget.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, info_widget.size(), this->window()->frameGeometry()));

    int BATCH_SIZE = 10;
    for (int i = 0; i < tourney_ids.count(); i += BATCH_SIZE) {
        QList<int> batch;

        for (int jj = 0; jj < BATCH_SIZE; jj++) {
            if (i + jj < tourney_ids.count())
                batch.append(tourney_ids[i + jj]);
            else
                batch.append(-1);
        }

        QString where_clause = " WHERE " + QString("id_tourney=? OR ").repeated(BATCH_SIZE - 1) + "id_tourney=?";
        QSqlQuery q;
        q.prepare("DELETE FROM tourney_summary " + where_clause);
        foreach (int id, batch)
            q.addBindValue(id);

        if (!q.exec())
            goto fail;

        q.prepare("DELETE FROM tourney_results " + where_clause);
        foreach (int id, batch)
            q.addBindValue(id);

        if (!q.exec())
            goto fail;

        q.prepare("DELETE FROM tourney_hand_player_statistics " + where_clause);
        foreach (int id, batch)
            q.addBindValue(id);

        if (!q.exec())
            goto fail;

        q.prepare("DELETE FROM tourney_hand_summary " + where_clause);
        foreach (int id, batch)
            q.addBindValue(id);

        if (!q.exec())
            goto fail;

        action_lbl->setText(QString("Deleted %1 / %2 tournies (from cloned DB)").arg(i).arg(tourney_ids.count()));
        qApp->processEvents();
    }

    m_db.commit();

    qDebug() << "Deleting took " << timer.elapsed() / 1000;

    goto ok;

    fail:
    QMessageBox::warning(this, "Error", "Failed to delete tournament in new DB, aborting split");
    return_value = false;

    ok:
    if(m_db.isOpen()) {
        m_db.close();
        m_db.setDatabaseName(src_db);

        if (!m_db.open()) {
            QMessageBox::critical(this, "Error", "Failed to reconnect to source DB, exiting");
            exit (0);
        }
    }

    return return_value;
}

void MainWindow::showEvent(QShowEvent *event) {
    event->accept();

    if (ui->databaseCombo->count() == 1 && m_settings.is_finished()) {
        foreach (auto db, list_databases())
            ui->databaseCombo->addItem(db);
    }
}

void MainWindow::on_databaseCombo_currentIndexChanged(const QString &db_name)
{
    clear_info();

    if (db_name == "")
        return;

    database_t & database = m_databases [db_name];

    if (m_db.isOpen())
        m_db.close();

    m_db.setHostName(m_settings.get_db_host());
    m_db.setPort(m_settings.get_db_port().toInt());
    m_db.setUserName(m_settings.get_db_user());
    m_db.setPassword(m_settings.get_db_pass());

    m_db.setDatabaseName(db_name);

    if (!m_db.open()) {
        QMessageBox::warning(this, "Error", "Could not connect to database. " + m_db.lastError().databaseText());
        return;
    }

    if (!m_db.tables().contains("player")) {
        QMessageBox::warning(this, "Error", "Table 'player' not found in Database " + db_name);
        clear_info();
        return;
    }

    if (!m_db.record("player").contains("custom_type")) {
        if (QMessageBox::question(this, "Dialog",
                                  "Did not find 'custom_type' column in your selected database, add it? Without this column nothing will work. Please backup before going any further.")
                == QMessageBox::Yes) {
            auto q = m_db.exec("ALTER TABLE player ADD COLUMN custom_type TEXT NOT NULL DEFAULT 'recreational'");

            if (q.lastError().type() != q.lastError().NoError) {
                QMessageBox::warning(this, "Error", "Failed to add column, please contact support");
                clear_info();
                return;
            }
        }
        else
            return;
    }

    if (database.tournies.count() == 0) {
        QSqlQuery q;
        // check if tourney is Ultra or Hyper
        if (!q.exec("SELECT id_table_type, description FROM tourney_table_type WHERE description='"+ULTRA_DESCRIPTION+"' or description='"+HYPER_DESCRIPTION+"'")) {
            QMessageBox::warning(this, "Error", "Could not find lottery type SNGs in your database, error : " + q.lastError().text());
            clear_info();
            return;
        }

        q.next();

        int lottery_id = q.value(0).toInt();
        QString description = q.value(1).toString();

        // query tourney attributes
        q.prepare("SELECT id_tourney, date_start, date_end, amt_buyin + amt_fee, amt_prize_pool FROM tourney_summary WHERE id_table_type=? ORDER BY date_start ASC");
        q.addBindValue(lottery_id);

        if (!q.exec()) {
            QMessageBox::warning(this, "Error", "Failed to get tournament list for the selected database, error : " + q.lastError().text());
            return;
        }

        database.is_valid = true;

        QApplication::setOverrideCursor(Qt::WaitCursor);
        ui->loadingLabel->show();
        QTime timer;
        timer.start();
        m_loading_tournies = true;

        int remaining_time = 0;
        QSet<QString> player_types;
        QMap<QString, int> player_count;

        while (q.next()) {
            tourney_t tourney;
            tourney.id = q.value(0).toInt();
            tourney.ts_start = q.value(1).toDateTime();
            tourney.ts_end = q.value(2).toDateTime();
            tourney.amt_buyin = q.value(3).toFloat();
            tourney.amt_prize_pool = q.value(4).toFloat();
            tourney.description = description;

            get_tourney_info (tourney);
            // check toruney is valid
            if (tourney.players.count() != 3)
                continue;

            // insert player types (to types-list)
            foreach (auto player, tourney.players) {
                player_types.insert(player.type);

                if (!player_count.contains(player.name))
                    player_count [player.name] = 1;
                else
                    player_count [player.name] += 1;
            }

            // append the tourney if everthing goes ok
            database.tournies.append(tourney);

            auto count = database.tournies.count();
            if (count % 100 == 0)
                remaining_time = ((timer.elapsed() / count) * (q.size() - count)) / 1000 + 1;

            ui->loadingLabel->setText(QString ("Loading tournies, %1 / %2 | %3 remaining seconds").arg(count).arg(q.size()).arg(remaining_time));
            qApp->processEvents();
        }
        QApplication::restoreOverrideCursor();

        ui->loadingLabel->setText("Finished loading " + QString::number(database.tournies.count()) + " tournies, took " + QString::number(timer.elapsed() / 1000) + " seconds");

        QSettings settings;
        QString from_date_key = "db/" + database.name + "/from_date";
        QString to_date_key = "db/" + database.name + "/to_date";

        if (!settings.contains(from_date_key)) {
            ui->from_date_edit->setDate(database.tournies.first().ts_start.date());
            settings.setValue(from_date_key, ui->from_date_edit->date());
        }
        else
            ui->from_date_edit->setDate(settings.value(from_date_key).toDate());

        if (!settings.contains(to_date_key)) {
            ui->to_date_edit->setDate(database.tournies.last().ts_start.date());
            settings.setValue(to_date_key, ui->to_date_edit->date());
        }
        else
            ui->to_date_edit->setDate(settings.value(to_date_key).toDate());

        // select hero
        {
            QList<QPair<QString, int>> players;

            foreach (auto key, player_count.keys())
                players.append(qMakePair(key, player_count [key]));

            struct {
                bool operator()(QPair<QString, int> a, QPair<QString, int> b) {
                    return a.second > b.second;
                }
            } _customGreater;

            std::sort(players.begin(), players.end(), _customGreater);

            QList<QString> sorted_players;
            foreach (auto pair, players)
                sorted_players.append(pair.first);

            database.hero = QInputDialog::getItem(this, "Dialog", "Select active player or input custom (not advised)", sorted_players);
            if (database.hero == "")
            {
                QMessageBox::warning(this, "Warning", "Database NOT loaded. You must select an active player.");
                ui->databaseCombo->setCurrentIndex(0);
                database.tournies.clear();
                database.reg_labels.clear();
                database.is_valid = false;
                return;
            }
        }

        foreach (auto type, player_types) {
            QStandardItem * item = new QStandardItem(type.toLower());
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);

            if(type != "recreational")
                database.player_type_model->appendRow(item);

            database.expression_player_type_model->appendRow(item->clone());
        }

        ui->expressionRegTypes->setModel(database.expression_player_type_model.get());
        ui->regTypeCombo->setModel(database.player_type_model.get());

        m_loading_tournies = false;

        QMessageBox::information(this, "Info", "Finished loading tournaments !");
    }
    else {
        ui->loadingLabel->hide();
        ui->line->hide();
    }
}

void MainWindow::get_tourney_info(tourney_t & tourney) {
    QSqlQuery q;
    q.prepare("SELECT p.player_name_search, p.custom_type, thps.amt_expected_won "
              "FROM tourney_hand_player_statistics thps, player p WHERE p.id_player = thps.id_player AND thps.id_tourney=? ORDER BY id_hand ASC");
    q.addBindValue(tourney.id);

    if (!q.exec()) {
        QMessageBox::warning(this, "Error", "Failed to get tourney info for id=" + QString::number(tourney.id));
        return;
    }

    while (q.next()) {
        QString name = q.value(0).toString();
        QString type = q.value(1).toString();
        int cev = q.value(2).toInt();

        if (!tourney.players.contains(name)) {
            player_t player (name, type);
            player.cev = cev;
            tourney.players[name] = player;
        }
        else
            tourney.players[name].cev += cev;
    }
}

void MainWindow::clear_info() {
    ui->regCountLabel->setText("0 regs found");
    ui->recCountLabel->setText("0 recs found");

    ui->zeroRegLabel->setText("# 0-reg games (# %)");
    ui->oneRegLabel->setText("# 1-reg games (# %)");
    ui->twoRegLabel->setText("# 2-reg games (# %)");
    ui->expressionRegTypes->clear();
    ui->regTypeCombo->clear();
    ui->loadingLabel->hide();
}

void MainWindow::on_regLoadButton_clicked()
{
    if (!m_db.isOpen() || !m_databases [ui->databaseCombo->currentText()].is_valid) {
        QMessageBox::warning(this, "Warning", "You need to load a valid PT4 database first");
        return;
    }

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setViewMode(QFileDialog::Detail);

    dialog.exec();

    QMap <QString, QString> regs;
    QSet <QString> reg_types;
    reg_types.insert("unknown reg");

    foreach (auto path, dialog.selectedFiles()) {
        QFile f (path);
        f.open (QFile::ReadOnly);

        bool first_line = true;

        while (!f.atEnd()) {
            QString line = f.readLine();

            // Handle XML documents
            if (first_line && line.startsWith("<?xml"))
            {
                f.close();
                ps_notes_reader_t reader (path);
                reader.read();

                if (reader.has_data()) {
                    if(reader.get_warnings().size() != 0) {
                        QMessageBox::warning(this, "Warning", reader.get_warnings());
                    }

                    m_notes_form = new PSNotesForm(reader.get_labels(), reader.get_players());
                    m_notes_form->exec();

                    foreach (auto label, m_notes_form->reg_labels) {
                        QString label_comment;
                        foreach (auto ps_label, *reader.get_labels())
                            if (ps_label.id == label)
                                label_comment = ps_label.comment;

                        reg_types.insert(label_comment);

                        foreach (auto player, reader.get_players()->value(label))
                            regs [player] = label_comment;
                    }

                    delete m_notes_form;
                }
                else
                    QMessageBox::warning(this, "Error", "No data found in " + path + ", error messages : \n" + reader.get_warnings());

                break;
            }

            if (line.contains(" $$type$$ ")) {
                auto list = line.trimmed().split(" $$type$$ ");
                if (list.size() != 2)
                    continue;

                regs [list[0].toLower()] = list [1].toLower();
                reg_types.insert(list[1].toLower());
            }
            else
                regs [line.trimmed().toLower()] = "unknown reg";
        }
    }

    // Update in memory tourneys
    database_t & database = m_databases [ui->databaseCombo->currentText()];
    QMutableListIterator<tourney_t> ti(database.tournies);

    while (ti.hasNext()) {
        auto & tourney = ti.next();
        QMutableMapIterator<QString, player_t> pi(tourney.players);

        while (pi.hasNext()) {
            auto & player = pi.next().value();
            if (regs.contains(player.name))
                player.type = regs [player.name];
        }

        qApp->processEvents();
    }

    // Update player types
    auto & model = database.player_type_model;
    QSet<QString> model_types;
    for (int i = 0; i < model->rowCount(); i++) {
        QString reg_type = model->itemFromIndex(model->index(i, 0))->text();
        model_types.insert(reg_type);
    }

    foreach (auto type, reg_types) {
        if (!model_types.contains(type.toLower()))
        {
            QStandardItem *item = new QStandardItem(type.toLower());
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);

            model->appendRow(item);
            if (type != "recreational")
                database.expression_player_type_model->appendRow(item->clone());
        }
    }

 //   QMessageBox::information(this, "Info", "Updated " + QString::number(count) + " out of " + QString::number(regs.count()) + " player types");
}

void MainWindow::on_regSaveButton_clicked()
{
    if (m_regs.count() == 0) {
        QMessageBox::warning(this, "Warning", "Nothing to save, no regs currently loaded");
        return;
    }

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.exec();

    if (dialog.selectedFiles().size() == 0)
        return;

    QString path = dialog.selectedFiles()[0];
    if (QFileInfo(path).exists()) {
        auto answer = QMessageBox::question(this, "Confirmation", "Are you sure you want to overwrite \"" + path + "\"?");
        if (answer == QMessageBox::No)
            return;
    }

    QFile file(path);
    file.open (QFile::WriteOnly);
    QTextStream stream (&file);

    bool save_player_types = QMessageBox::question(this, "Dialog", "Save player types for each player?") == QMessageBox::Yes;

    foreach (auto reg, m_regs) {
        if (save_player_types)
            stream << reg.first.toLower() << " $$type$$ " << reg.second.toLower() << "\n";
        else
            stream << reg.first.toLower() << "\n";
    }

    QMessageBox::information(this, "Info", QString::number(m_regs.count()) + " regs have been saved to \"" + path + "\"");
}

/**
 * Fails if
 *  -Time Interval is invalid
 *  -Num of Regular types == 0
 */
void MainWindow::on_refreshBtutton_clicked()
{
    int zero_reg_count = 0;
    int one_reg_count = 0;
    int two_reg_count = 0;

    int zero_reg_chips = 0;
    int one_reg_chips = 0;
    int two_reg_chips = 0;

    // labels and players names
    QSet<QString> reg_types;
    QSet<QString> regulars;
    QSet<QString> recreationals;

    auto & database = m_databases [ui->databaseCombo->currentText()];
    auto & model = database.player_type_model;

    // Load regular players models
    for (int i = 0; i < model->rowCount(); i++) {
        auto item = model->itemFromIndex(model->index(i, 0));
        if (item->checkState() == Qt::Checked)
            reg_types.insert (item->text());
    }

    qDebug() << "reg_types on database = " << reg_types;

    // Asserts
    if (reg_types.count() == 0) {
        QMessageBox::warning(this, "Warning", "You need to check reg types in the list to update stats");
        return;
    }

    if (ui->to_date_edit->date() < ui->from_date_edit->date())
    {
        QMessageBox::warning(this, "Warning", "Invalid time interval (to-date earlier than from-date)");
        return;
    }

    int invalid_tournies = 0;
    // Only check tourneys where hero plays, and 3-player tournaments
    foreach (auto tourney, database.tournies) {
        if (tourney.players.count() != 3) {
            invalid_tournies += 1;
            continue;
        }

        if (!tourney.players.contains(database.hero))
            continue;

        if (!check_tourney_constraints(tourney))
            continue;

        int regs = 0;
        foreach (auto player, tourney.players) {
            // check toLower and check is regular
            if (player.name != database.hero.toLower() && reg_types.contains (player.type.toLower())) {
                regs += 1;
                regulars.insert(player.name);
            }
            else {
                recreationals.insert(player.name);
                //if (player.type != "recreational")
                //    qDebug() << player.type;
            }
        }

        if (regs == 0) {
            zero_reg_count ++;
            zero_reg_chips += tourney.players[database.hero].cev;
        }
        else if (regs == 1) {
            one_reg_count ++;
            one_reg_chips += tourney.players[database.hero].cev;
        }
        else {
            two_reg_count ++;
            two_reg_chips += tourney.players[database.hero].cev;
        }
    }

    int total_game_count = zero_reg_count + one_reg_count + two_reg_count;

    if (total_game_count == 0) {
        QMessageBox::warning(this, "Warning", "No tournaments found in the selected time interval or for the selected stakes");
        return;
    }

    ui->total_games_label->setText("Total Games : " + QString::number(total_game_count));
    ui->recCountLabel->setText(QString::number(recreationals.count()) + " recreationals found");
    ui->regCountLabel->setText(QString::number(regulars.count()) + " regulars found");

    if (zero_reg_count) {
        int percentage = zero_reg_count * 100 / total_game_count;
        int cev = zero_reg_chips / zero_reg_count;
        ui->zeroRegLabel->setText(QString("%1 0-reg games (%2%) | cEV = %3").arg(zero_reg_count).arg(percentage).arg(cev));
    }
    else
        ui->zeroRegLabel->setText("0 0-reg games");

    if (one_reg_count) {
        int percentage = one_reg_count * 100 / total_game_count;
        int cev = one_reg_chips / one_reg_count;
        ui->oneRegLabel->setText(QString("%1 1-reg games (%2%) | cEV = %3").arg(one_reg_count).arg(percentage).arg(cev));
    }
    else
        ui->oneRegLabel->setText("0 1-reg games");

    if (two_reg_count) {
        int percentage = two_reg_count * 100 / total_game_count;
        int cev = two_reg_chips / two_reg_count;
        ui->twoRegLabel->setText(QString("%1 2-reg games (%2%) | cEV = %3").arg(two_reg_count).arg(percentage).arg(cev));
    }
    else
        ui->twoRegLabel->setText("0 2-reg games");

    if (invalid_tournies)
        QMessageBox::warning(this, "Warning", QString::number(invalid_tournies) + " invalid tournies found, report this please");
}

void MainWindow::on_dbSplitButton_clicked()
{
    QString prefix = ui->dbPrefixEdit->text();

    if (ui->databaseCombo->currentText() == "") {
        QMessageBox::warning(this, "Warning", "You must first select a database");
        return;
    }

    if (prefix == "") {
        QMessageBox::warning(this, "Warning", "You must type in a database prefix");
        return;
    }

    if (m_loading_tournies) {
        QMessageBox::warning(this, "Warning", "Can not split while loading tournaments");
        return;
    }

    auto databases = list_databases();
    bool ok = false;

    if (ui->zero_reg_checkbox->checkState() == Qt::Checked) {
        ok = true;
        if (databases.contains(prefix + "_0regs")) {
            QMessageBox::warning(this, "Warning", "Invalid prefix, split would override existing databases");
            return;
        }
    }

    if (ui->one_reg_checkbox->checkState() == Qt::Checked) {
        ok = true;

        if(databases.contains(prefix + "_1regs")) {
            QMessageBox::warning(this, "Warning", "Invalid prefix, split would override existing databases");
            return;
        }

    }

    if (ui->two_reg_checkbox->checkState() == Qt::Checked) {
        ok = true;

        if(databases.contains(prefix + "_2regs")) {
            QMessageBox::warning(this, "Warning", "Invalid prefix, split would override existing databases");
            return;
        }
    }

    if (!ok) {
        QMessageBox::warning(this, "Warning", "You must select at least one of the 0-reg, 1-reg or 2-reg checkboxes");
        return;
    }

    auto & database = m_databases [ui->databaseCombo->currentText()];
    auto & model = database.player_type_model;
    QSet<QString> reg_types;

    for (int i = 0; i < model->rowCount(); i++) {
        auto item = model->itemFromIndex(model->index(i, 0));
        if (item->checkState() == Qt::Checked)
            reg_types.insert (item->text());
    }

    if (reg_types.count() == 0) {
        QMessageBox::warning(this, "Warning", "You need to check reg types in the list before splitting");
        return;
    }

    QList<int> zero_reg_ids;
    QList<int> one_reg_ids;
    QList<int> two_reg_ids;

    foreach (auto & tourney, database.tournies) {
        if (tourney.players.count() != 3) {
            continue;
        }

        if (!tourney.players.contains(database.hero))
            continue;

        int regs = 0;
        foreach (auto player, tourney.players) {
            if (player.name.toLower() != database.hero.toLower() && reg_types.contains (player.type.toLower()))
                regs += 1;
        }

        if (regs == 0)
            zero_reg_ids.append(tourney.id);
        else if (regs == 1)
            one_reg_ids.append(tourney.id);
        else
            two_reg_ids.append(tourney.id);

        qApp->processEvents();
    }

    if (QMessageBox::Yes != QMessageBox::question(this, "Dialog",
                                                  QString("Found %1 0-reg games , %2 1-reg games, %3 2-reg games, are you sure you want to split?")
                                                  .arg(zero_reg_ids.count()).arg(one_reg_ids.count()).arg(two_reg_ids.count())))
        return;

    QString src_db = ui->databaseCombo->currentText();

    QTemporaryDir temp;

    if (!temp.isValid()) {
        QMessageBox::warning(this, "Error", "Failed to create temp dir, can not clone");
        return;
    }

    QDir dump_file (temp.path().append("/dump"));
    if (dump_file.exists())
        dump_file.removeRecursively();

    CustomProcessDialog dump_db_dialog("Dumping source database", m_settings.get_debug_mode(), this);
    dump_db_dialog.add_env_var("PGPASSWORD", m_settings.get_db_pass());
    if (dump_db_dialog.start_process(m_settings.get_psql_util("pg_dump"),
                                     QStringList() << "--no-password" << "--username=" + m_settings.get_db_user()
                                     << "--file=" + dump_file.path() << "--format=d" << "-j2" << src_db))
        dump_db_dialog.exec();
    else {
        QMessageBox::warning(this, "Error", "Failed to start pg_dump process");
        return;
    }

    if (dump_db_dialog.proc_ref().exitCode() != 0) {
        QMessageBox::warning(this, "Error", "Failed to dump source database");
        return;
    }

    if (zero_reg_ids.count() > 0 && ui->zero_reg_checkbox->checkState() == Qt::Checked) {
        if (!split_db(src_db, prefix + "_0regs", dump_file.path(), one_reg_ids + two_reg_ids))
            return;
    }
    if (one_reg_ids.count() > 0 && ui->one_reg_checkbox->checkState() == Qt::Checked) {
        qDebug() << "Splitting 1_reg db, keeping tournaments : " << one_reg_ids;

        if (!split_db(src_db, prefix + "_1regs", dump_file.path(), zero_reg_ids + two_reg_ids))
            return;
    }
    if (two_reg_ids.count() > 0 && ui->two_reg_checkbox->checkState() == Qt::Checked) {
        if (!split_db(src_db, prefix + "_2regs", dump_file.path(), zero_reg_ids + one_reg_ids))
            return;
    }
}

QList<QString> MainWindow::list_databases() {
    QList<QString> result;

    CustomProcessDialog dialog(QString ("Listing databases"), m_settings.get_debug_mode(), this);
    dialog.add_env_var("PGPASSWORD", m_settings.get_db_pass());
    if (dialog.start_process(m_settings.get_psql_util("psql"),
                             QStringList() << "--username=" + m_settings.get_db_user() << "--list" << "-P" << "format=unaligned" << "-P" << "tuples_only" << "-P" << "fieldsep=,"))
        dialog.exec();
    else {
        QMessageBox::warning(this, "Error", "Failed to list databases, contact support");
        exit (1);
    }

    QTextStream stream (dialog.get_stdout());
    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.startsWith("template0,"))
            break;

        result.append(line.split(',') [0]);
    }

    return result;
}

void MainWindow::on_getExpressionButton_clicked()
{
    QString position = ui->positionCombo->currentText();
    QString player_types;

    if (position == "Small Blind")
        position = "9";
    else if (position == "Big Blind")
        position = "8";
    else
        position = "0";

    auto model = (QStandardItemModel*)ui->expressionRegTypes->model();

    for (int i = 0; i < model->rowCount(); i++) {
        auto item = model->itemFromIndex(model->index(i, 0));
        if (item->checkState() == Qt::Checked) {
            if (player_types.count() == 0)
                player_types += "p.custom_type=$_$" + item->text() + "$_$";
            else
                player_types += " OR p.custom_type=$_$" + item->text() + "$_$";
        }
    }

    if (player_types.count() == 0)
    {
        QMessageBox::warning(this, "Warning", "You must select a player type for this filter");
        return;
    }

    QString expression;
    expression += "tourney_hand_player_statistics.id_hand IN (";
    expression += "SELECT thps.id_hand FROM tourney_hand_player_statistics thps, player p ";
    expression += "WHERE thps.id_player=p.id_player AND thps.position=" + position + " AND (" +  player_types + "))";

    qApp->clipboard()->setText(expression);
    QMessageBox::information(this, "", "Copied expression to clipboard");
    //QMessageBox mb(QMessageBox::NoIcon, "Expression filter", expression, QMessageBox::NoButton, this);
    //mb.setTextInteractionFlags(Qt::TextSelectableByMouse);
    //mb.exec();
}

void MainWindow::on_reg_load_pt4_button_clicked()
{
    if (!m_db.isOpen() || !m_databases [ui->databaseCombo->currentText()].is_valid) {
        QMessageBox::warning(this, "Warning", "You need to load a valid PT4 database first");
        return;
    }

    QSqlQuery q;
    if (!q.exec("SELECT id_color, COUNT(id_color) FROM notes GROUP BY id_color ORDER BY id_color ASC")) {
        QMessageBox::warning(this, "Warning", "Failed to get PT4 notes | " + q.lastError().text());
        return;
    }

    QList<QPair<int, int>> labels;

    while (q.next())
        labels.append(qMakePair(q.value(0).toInt(), q.value(1).toInt()));

    PT4Loader loader(&labels);

    loader.exec();
    loader.update_reg_labels();

    if (loader.reg_labels.count() == 0) {
        QMessageBox::information(this, "Info", "No changes have been made");
        return;
    }

    foreach (int id, loader.reg_labels) {
        q.prepare("UPDATE player SET custom_type='regular' FROM notes WHERE id_x=id_player AND id_color=?");
        q.addBindValue(id);
        q.exec();
    }

    if (!q.exec("SELECT player_name_search FROM player WHERE custom_type='regular'")) {
        QMessageBox::warning(this, "Warning", "Failed to get reg list | " + q.lastError().text());
        return;
    }

    QSet<QString> regs;

    while(q.next())
        regs.insert(q.value(0).toString());

    // Update in memory tourneys
    database_t & database = m_databases [ui->databaseCombo->currentText()];
    QMutableListIterator<tourney_t> ti(database.tournies);

    while (ti.hasNext()) {
        auto & tourney = ti.next();
        QMutableMapIterator<QString, player_t> pi(tourney.players);

        while (pi.hasNext()) {
            auto & player = pi.next().value();
            if (regs.contains(player.name))
                player.type = "regular";
        }

        qApp->processEvents();
    }

    // Update player types
    auto & model = database.player_type_model;
    bool has_it = false;
    for (int i = 0; i < model->rowCount(); i++)
        if (model->itemFromIndex(model->index(i, 0))->text() == "regular")
            has_it = true;

    if (!has_it) {
        QStandardItem *item = new QStandardItem("regular");
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);

        model->appendRow(item);
        database.expression_player_type_model->appendRow(item->clone());
    }

    QMessageBox::information(this, "Info", "Database update finished");
}


void MainWindow::on_extractTimesButton_clicked()
{
    if (ui->databaseCombo->currentText() == "") {
        QMessageBox::warning(this, "Warning", "You must first select a database");
        return;
    }
    auto & database = m_databases [ui->databaseCombo->currentText()];
    auto & model = database.player_type_model;

    QSet<QString> reg_types;
    QSet<QString> regulars;
    QSet<QString> players;
    //map datetime -> num tournies
    QMap<QDateTime, int> num_tournies;
    //map datetime -> num reg players
    QMap<QDateTime, int> num_regs;
//    QMap<QString, QVector<interval_t>> times;

    // Load regular players models
    for (int i = 0; i < model->rowCount(); i++) {
        auto item = model->itemFromIndex(model->index(i, 0));
        if (item->checkState() == Qt::Checked) {
            reg_types.insert (item->text());
            qDebug() << "Item text -> " << item->text() << '\n';
        }
    }

    // Asserts
    if (reg_types.count() == 0) {
        QMessageBox::warning(this, "Warning", "You need to check reg types in the list to update stats");
        return;
    }

    if (ui->to_date_edit->date() < ui->from_date_edit->date())
    {
        QMessageBox::warning(this, "Warning", "Invalid time interval (to-date earlier than from-date)");
        return;
    }

    if (ui->ultra_checkBox->checkState() != Qt::Checked && ui->hyper_checkBox->checkState() != Qt::Checked)
    {
        QMessageBox::warning(this, "Warning", "Select Hyper or Ultra");
        return;
    }

    int invalid_tournies = 0;

    int total_tournies_count = 0;

    foreach (auto tourney, database.tournies) {
        if (tourney.players.count() != 3) {
            invalid_tournies += 1;
            continue;
        }

        if (!check_tourney_constraints(tourney))
            continue;
        // count valid tournies
        total_tournies_count++;

        QDateTime dateTime = tourney.ts_start;
        QDateTime dateTime2 = tourney.ts_end;
        num_tournies[dateTime]++;
        // uncomment for count twice if ENDING TIME distinct of START TIME
        //if (dateTime.time().hour() != dateTime2.time().hour())
          //  num_tournies[dateTime2]++;

        // Check for each player if it is regular then save the tourney hours
        foreach (auto player, tourney.players) {
            // skip hero
            if (player.name.toLower() == database.hero.toLower())
                continue;
            // regulars
            if (reg_types.contains (player.type.toLower())) {
                num_regs[dateTime]++;
            // uncomment for count twice if ENDING TIME distinct of START TIME
            //    if (dateTime.time().hour() != dateTime2.time().hour())
              //      num_regs[dateTime2]++;
            }
        }
    }

    if (!total_tournies_count) {
        QMessageBox::warning(this, "Warning", "No tournaments found in the selected time interval or for the selected stakes");
        return;
    }

    QString today = QDateTime::currentDateTime().time().toString();

    // print graphs
    chart_window_tmp = new ChartWindow();
    chart_window_tmp->draw(num_tournies, "Number of tournies", "tournies_info"+today+".txt");
    chart_window_abs = new ChartWindow();
    chart_window_abs->draw(num_regs, "Number of regulars", "regs_info"+today+".txt");
    chart_window_rel = new ChartWindow();
    chart_window_rel->draw(num_tournies, num_regs, "Percentage of regulars", "regs-tournies_info"+today+".txt");

    if (invalid_tournies) {
        QMessageBox::warning(this, "Warning", QString::number(invalid_tournies) + " invalid tournies found, report this please");
    }

}

/**
 * @brief save data on a path
 * @param data[d][h] = amount of data at day d and hour h
 * @param path to save data
 */
void MainWindow::save_data_dayhour(double ** data, QString path) {
    QFile time_file(path);
    // Fails
    if (!time_file.open(QIODevice::WriteOnly) || QIODevice::Text)
        return ;
    QTextStream out(&time_file);

    // for each (regular) player, print timetables
    for(int i = 1; i <= 7; ++i) {
        out << "---\n" << getDay(i) << '\n';
        for(int j = 0; j < 24; ++j) {
            if (data[i][j])
                out << "At " << j << ":00 there were " << data[i][j] << " (average)\n";
        }
    }
    time_file.close();
}

bool MainWindow::check_tourney_constraints(tourney_t & tourney) {
    // assert date
    if (!(ui->from_date_edit->date() <= tourney.ts_start.date() && tourney.ts_start.date() <= ui->to_date_edit->date()))
        return false;
    // assert buyin
    if (!(ui->min_stake_combo->currentText().toFloat() <= tourney.amt_buyin && tourney.amt_buyin <= ui->max_stake_combo->currentText().toFloat()))
        return false;
    // assert hyper or ultra button
    if (!((ui->ultra_checkBox->checkState() == Qt::Checked && tourney.description == ULTRA_DESCRIPTION)
       || (ui->hyper_checkBox->checkState() == Qt::Checked && tourney.description == HYPER_DESCRIPTION)))
        return false;
    return true;
}

