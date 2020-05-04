#ifndef CHARTWINDOW_H
#define CHARTWINDOW_H
#include <QMainWindow>
#include <QTime>

/**
 * @brief getDay
 * @return a string with the name of the day (1 = "Monday", ... 7 = "Sunday")
 */
static QString getDay(int day) {
    switch (day) {
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        case 7: return "Sunday";
        default: return "Unknown day";
    }
}

/**
 * @brief The ChartWindow class manages the chart printed with players-times data
 */
class ChartWindow : public QMainWindow {
    Q_OBJECT

public: ChartWindow();
    /**
     * @brief show a new window with a graph showing data array
     * @param data[d][h] = amount of data on day d, hour h
     * @param max_data = max value of the data por Y axis
     * @param title of the graph
     * @param tick_count number of ticks on Y axis
     */
    void graphic_hours(double ** data, double max_data, QString title, int tick_count);

    /**
     * @brief show a new window with a graph showing data array
     * @param data[h][d] = amount of data on day d, hour h
     * @param max_data = max value of the data por Y axis
     * @param title of the graph
     * @param tick_count number of ticks on Y axis
     */
    void graphic_hours_transpose(double ** data, double max_data, QString title, int tick_count);

    /**
     * @brief save data on a path
     * @param data[d][h] = amount of data at day d and hour h
     * @param path to save data
     */
    void save_data_dayhour(double ** data, QString path);

    /**
      @brief invert the map into a data[d][h] array for drawing on a graph
     * @param data (absolute data)
     * @return data[d][h] = number of data at that day/hour
     */
    double ** count_data(QMap<QDateTime, int> data);
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
    double ** count_data(QMap<QDateTime, int> data_abs, QMap<QDateTime, int> data_rel);

    /**
     * @brief Chartdraw absolute data
     * @param data map of data
     * @param title of the graph
     * @param path path to log data
     */
    void draw(QMap<QDateTime, int> data, QString title, QString path);
    /**
     * @brief Chartdraw relativa data
     * @param tables map of tables on a date-time
     * @param regs map of regs on a date-time
     * @param title of the graph
     * @param path path to log data
     */
    void draw(QMap<QDateTime, int> tables, QMap<QDateTime, int> regs, QString title, QString path);

};

#endif // CHARTWINDOW_H
