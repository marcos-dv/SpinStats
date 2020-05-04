#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include "settingsform.h"
#include "chartwindow.h"
#include <QStandardItemModel>
#include <memory>

/**
 * Main class of the project where most of the events happen.
 * Defines data structs and User Interface.
 */

/**
 * @brief Poker player struct, name, label, expected value CEV
 */

struct player_t {
    // player's name
    QString name;
    // player's label -> recreational, regular...
    QString type;
    int cev;

    player_t () {}
    player_t (QString name, QString type) : name(name), type(type) {}
    player_t (QString & player_name) : name (player_name), type ("recreational") {}

    bool operator==(const player_t & player) {
        return player.name == this->name;
    }
};

/**
 * @brief The tourney_t struct, players, date, buyin, prizepool
 */
struct tourney_t {
    int id;
    float amt_buyin;
    float amt_prize_pool;

    tourney_t () {
        id = 0;
        amt_buyin = 0;
        amt_prize_pool = 0;
    }

    /**
     * @brief match name -> player
     */
    QMap<QString, player_t> players;

    QDateTime ts_start;
    QDateTime ts_end;

    QString description;

};

// tourney types
const QString HYPER_DESCRIPTION = "STT (3 max) Hyper-Turbo SNG LOTTERY";
const QString ULTRA_DESCRIPTION = "STT (3 max) Ultra-Turbo SNG LOTTERY";

/**
 * @brief The database_t struct, tournies, labels of regular players
 * hero name.
 */
struct database_t {
    QString name;
    QString hero;
    QList<tourney_t> tournies;
    QList<QString> reg_labels;
    std::shared_ptr<QStandardItemModel> player_type_model;
    std::shared_ptr<QStandardItemModel> expression_player_type_model;
    bool is_valid;

    database_t ():
        player_type_model(new QStandardItemModel),
        expression_player_type_model(new QStandardItemModel),
        is_valid (false)
    {}
};

/**
 * UI
 */
namespace Ui {
    class MainWindow;
}

/**
 * @brief The MainWindow class handles user actions and
 * process access to the database.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

/**
  * User performing actions
  *
  */
private slots:
    //void on_pushButton_clicked();

    void launch_settings();

    void showEvent(QShowEvent *event);

    void on_databaseCombo_currentIndexChanged(const QString &arg1);

    void on_regLoadButton_clicked();

    void on_regSaveButton_clicked();

    void on_refreshBtutton_clicked();

    void on_dbSplitButton_clicked();

    void on_getExpressionButton_clicked();

    void on_reg_load_pt4_button_clicked();

    /**
     * @brief on_extractTimesButton_clicked
     * Extract times from:
     * -Number of tournies
     * -Number of regulars playing (maybe repeating)
     * -Number of regulars / (2*tourney)
     * (Last one for the density of reg players -> each tourney has 3 players
     * but one of them is the hero)
     * Only counts START TIME of tournies
     */
    void on_extractTimesButton_clicked();

private:
    Ui::MainWindow * ui;
    // absolute chart counting regular players at a time
    ChartWindow * chart_window_abs;
    // relative chart regular players vs total players at a time
    ChartWindow * chart_window_rel;
    // temporal chart
    ChartWindow * chart_window_tmp;

    QSqlDatabase m_db;

    SettingsForm m_settings;

    bool m_loading_tournies;

    PSNotesForm * m_notes_form;

    /**
     * @brief m_regs, (name, type) of a regular player
     */

    QSet<QPair<QString, QString>> m_regs;

    QMap <QString, database_t> m_databases;

    bool split_db (QString in, QString out, QString dump_file, QList<int> tourney_ids);

    int get_cev(const QList<tourney_t> & tournies);

    void show_textbox(QString);
    void get_tourney_info (tourney_t &);
    void add_tourney_info_to_table (const tourney_t &);

    // check if tourney asserts small-big blinds, date, and description (hyper or ultra)
    bool check_tourney_constraints(tourney_t &);

    /**
     * @brief save data on a path
     * @param data[d][h] = amount of data at day d and hour h
     * @param path to save data
     */
    void save_data_dayhour(double ** data, QString path);

    void clear_info();

    void log(QString msg);

    QList<QString> list_databases();
};

#endif // MAINWINDOW_H
