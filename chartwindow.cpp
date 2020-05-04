#include "chartwindow.h"

#include <QSet>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QTime>
#include <QtCharts>
#include <QtGlobal>

const int DAYS = 7;
const int HOURS = 24;
const QString TOURNIES_FILE = "tournies_times.txt";

ChartWindow::ChartWindow() {

}

/**
  @brief invert the map into a data[d][h] array for drawing on a graph
 * @param data (absolute data)
 * @return data[d][h] = number of data at that day/hour
 */
double ** ChartWindow::count_data(QMap<QDateTime, int> data) {

    // times has all the matchings
    // lets invert the matchings
    QMap<QDate, int> dates[DAYS+1][HOURS];
    // dates[d][h][date] = set of data at that hour/day/date
    QMapIterator<QDateTime, int> i(data);
    while (i.hasNext()) {
        i.next();
        int day = i.key().date().dayOfWeek();
        int hour = i.key().time().hour();
        // add data to that date
        dates[day][hour][i.key().date()] += i.value();
    }
    // extract the mean, players[d][h] = mean of players at that day/hour
    double ** datadh = new double * [DAYS+1];
    for(int i = 1; i <= DAYS; ++i)
        datadh[i] = new double[HOURS];
    for(int i = 1; i <= DAYS; ++i)
    for(int j = 0; j < HOURS; ++j) {
        datadh[i][j] = 0;
    }
    for(int i = 1; i <= DAYS; ++i)
    for(int j = 0; j < HOURS; ++j) {
        int count = 0; // number of distinct dates at the same hour/day
        // (then divide by count for the mean)
        int sum = 0; // number of data at that time
        QMapIterator<QDate, int> it(dates[i][j]);
        while (it.hasNext()) {
            it.next();
            count++;
            sum += it.value();
        }
        if (count) {
            datadh[i][j] = sum / (double)count;
          //  qDebug() << sum << '/' << count << '\n';
          //  qDebug() << i << ' ' << j << ' ' << players[i][j] << '\n';
        }
    }
    return datadh;
}

/**
 * @brief invert the maps into a data[d][h] arrays, and combine them for drawing
 * relative data on a graph
 * @param data_abs number of tournies
 * @param data_rel number of reg players
 * @return data[d][h] = mean of regulars / (2*tournies) at that day/hour
 * Example: Given a day d and an hour h, there are 3 tables A, B, C
 * A has 2 regs
 * B and C have 1 reg (may be the same regulars)
 * Then we count 4 regulars and 3 tables -> 4/3*2 = 4/6 are regs
 */
double ** ChartWindow::count_data(QMap<QDateTime, int> data_abs, QMap<QDateTime, int> data_rel) {
    QMap<QDate, int> dates_abs[DAYS+1][HOURS];
    QMap<QDate, int> dates_rel[DAYS+1][HOURS];
    // dates[d][h][date] = number of tournies at that hour/day/date
    QMapIterator<QDateTime, int> i_abs(data_abs);
    while (i_abs.hasNext()) {
        i_abs.next();
        int day = i_abs.key().date().dayOfWeek();
        int hour = i_abs.key().time().hour();
        dates_abs[day][hour][i_abs.key().date()] += i_abs.value();

        //qDebug() << day << ' ' << hour << ' ' << i.key().date() << ' ' << dates[day][hour][i.key().date()] << '\n';
    }
    // dates[d][h][date] = number of regs at that hour/day/date
    QMapIterator<QDateTime, int> i_rel(data_rel);
    while (i_rel.hasNext()) {
        i_rel.next();
        int day = i_rel.key().date().dayOfWeek();
        int hour = i_rel.key().time().hour();
        dates_rel[day][hour][i_rel.key().date()] += i_rel.value();

        //qDebug() << day << ' ' << hour << ' ' << i.key().date() << ' ' << dates[day][hour][i.key().date()] << '\n';
    }
    // extract the mean, players[d][h] = mean of players at that day/hour
    double ** datadh = new double * [DAYS+1];
    for(int i = 1; i <= DAYS; ++i)
        datadh[i] = new double[HOURS];
    for(int i = 1; i <= DAYS; ++i)
    for(int j = 0; j < HOURS; ++j) {
        datadh[i][j] = 0;
    }
    for(int i = 1; i <= DAYS; ++i)
    for(int j = 0; j < HOURS; ++j) {
        double sum = 0;
        int total_abs = 0; // number of players at that time
        int total_rel = 0; // number of special (reg) players at that time
        int count = 0; // number of distinct dates at the same hour/day
        QMapIterator<QDate, int> it(dates_abs[i][j]);
        while (it.hasNext()) {
            it.next();
            total_rel += dates_rel[i][j][it.key()];
            total_abs += it.value();
            sum += (double)total_rel / ((double)total_abs*2.0);
            count++;
        }
        if (count) {
            datadh[i][j] = 100.0 * sum / (double)count;
        }
    }
    return datadh;
}

/**
 * @brief show a new window with a graph showing data array
 * @param data[d][h] = amount of data on day d, hour h
 * @param max_data = max value of the data por Y axis
 * @param title of the graph
 * @param tick_count number of ticks on Y axis
 */
void ChartWindow::graphic_hours(double ** data, double max_data, QString title, int tick_count = 10) {
    // Each block composed of 7 sets
    //![1]
    QString days_names[DAYS] = {"L", "M", "X", "J", "V", "S", "D"};
    QBarSet *set[DAYS];

    for(int d = 0; d < DAYS; ++d)
        set[d] = new QBarSet(days_names[d]);

    // Fill each set in each block with data
    for(int d = 0; d < DAYS; ++d) {
        for(int h = 0; h < HOURS; ++h) {
            *set[d] << data[d+1][h];
        }
    }

    // TODO Refactor this code
    //![2]
    QBarSeries *series = new QBarSeries();
    for(int d = 0; d < DAYS; ++d)
        series->append(set[d]);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    //![3]

    //![4]
    // hours
    QStringList categories;
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    categories << "00:00" << "01:00" << "02:00" << "03:00" << "04:00" << "05:00" <<
                   "06:00" << "07:00" << "08:00" << "09:00" << "10:00" << "11:00" <<
                   "12:00" << "13:00" << "14:00" << "15:00" << "16:00" << "17:00" <<
                   "18:00" << "19:00" << "20:00" << "21:00" << "22:00" << "23:00";

    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    qDebug() << "OK X axis";

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0,max_data);
    axisY->setTickCount(qMin(max_data, (double)tick_count));
    axisY->applyNiceNumbers();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    //![4]

    //![5]
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    //![5]

    //![6]
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    //![6]

    //![7]
    setCentralWidget(chartView);
    resize(420, 300);
    show();

}

/**
 * @brief show a new window with a graph showing data array
 * @param data[d][h] = amount of data on day d, hour h
 * @param max_data = max value of the data por Y axis
 * @param title of the graph
 * @param tick_count number of ticks on Y axis
 */
void ChartWindow::graphic_hours_transpose(double ** data, double max_data, QString title, int tick_count = 10) {
    // Each block composed of 24 sets
    //![1]
    QString hours_names[HOURS] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                                "10","11","12","13","14","15","16","17","18","19",
                                "20","21","22","23"};
    QBarSet *set[HOURS];

    for(int h= 0; h < HOURS; ++h)
        set[h] = new QBarSet(hours_names[h]);

    // Fill each set in each block with data
    for(int h = 0; h < HOURS; ++h) {
        for(int d = 1; d <= DAYS; ++d) {
            *set[h] << data[h][d];
        }
    }

    //![2]
    QBarSeries *series = new QBarSeries();
    for(int h = 0; h < HOURS; ++h)
        series->append(set[h]);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    //![3]

    //![4]
    // hours
    QStringList categories;
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    categories << "L" << "M" << "X" << "J" << "V" << "S" << "D";

    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    qDebug() << "OK X axis";

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0,max_data);
    axisY->setTickCount(qMin(max_data, (double)tick_count));
    axisY->applyNiceNumbers();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    //![4]

    //![5]
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    //![5]

    //![6]
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    //![6]

    //![7]
    setCentralWidget(chartView);
    resize(420, 300);
    show();

}



/**
 * @brief Chartdraw absolute data
 * @param data map of data
 * @param title of the graph
 * @param path to log data
 */
void ChartWindow::draw(QMap<QDateTime, int> data, QString title, QString path = TOURNIES_FILE) {
        // dataperinterval[d][h] = number of data at day d, hour h (mean of all dates)
        // 1 <= d <= 7, 0 <= h <= 23
        double ** datadh = count_data(data);
        if (path != "")
            save_data_dayhour(datadh, path);
        // Find max in the range of Y axis
        double maxim_data = 0;
        for(int i = 1; i <= DAYS; ++i)
        for(int j = 0; j < HOURS; ++j) {
            maxim_data = qMax(maxim_data, datadh[i][j]);
        }

        graphic_hours(datadh, maxim_data, title);
        for(int i = 1; i <= DAYS; ++i)
            delete datadh[i];
        delete datadh;

}

/**
 * @brief Chartdraw relativa data
 * @param tables map of tables on a date-time
 * @param regs map of regs on a date-time
 * @param title of the graph
 * @param path path to log data
 */
void ChartWindow::draw(QMap<QDateTime, int> tables, QMap<QDateTime, int> regs, QString title, QString path = TOURNIES_FILE) {
    // players[d][h] = number of players at day d, hour h (mean of all dates)
    // 1 <= d <= 7, 0 <= h <= 23
    double ** data = count_data(tables, regs);
    if (path != "") {
        save_data_dayhour(data, path);
    }

    graphic_hours(data, 100.0, title);

    for(int i = 1; i <= DAYS; ++i)
        delete data[i];
    delete data;
}

/**
 * @brief save data on a path
 * @param data[d][h] = amount of data at day d and hour h
 * @param path to save data
 */
void ChartWindow::save_data_dayhour(double ** data, QString path) {
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



