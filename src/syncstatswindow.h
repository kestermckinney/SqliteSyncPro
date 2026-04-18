// Copyright (C) 2026 Paul McKinney
#pragma once

#include "syncresult.h"

#include <QWidget>
#include <QDateTime>
#include <QList>

class QChartView;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;
class QPlainTextEdit;
class QScrollBar;
class QCloseEvent;
class QHideEvent;

/**
 * SyncStatsWindow – floating window that displays a Windows-Performance-Monitor-
 * style scrolling chart (upload KB / download KB per sync cycle) and a text log
 * with per-table byte and row counts.
 *
 * The chart shows a fixed window of kVisiblePoints cycles at a time.  A scroll
 * bar beneath the chart lets the user scroll back through older history.  When
 * the scroll bar is at its maximum the chart auto-follows new data.
 *
 * The X axis shows wall-clock time (HH:mm:ss).  Each Y value is the KB
 * transferred in that single sync cycle — not a cumulative total.
 *
 * Data is cleared automatically whenever the window is hidden or closed so
 * memory is released immediately.  The next showStats(true) call starts a
 * fresh collection.
 */
class SyncStatsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SyncStatsWindow(QWidget *parent = nullptr);

    void addDataPoint(const SyncResult &result);
    void clearData();

signals:
    void windowClosed();

protected:
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onScrollChanged(int value);

private:
    void    updateChartDisplay();
    QString formatBytes(qint64 bytes) const;

    struct DataPoint {
        QDateTime timestamp;
        double    uploadKB   = 0.0;
        double    downloadKB = 0.0;

        struct TableDetail {
            QString tableName;
            qint64  bytesPushed = 0;
            qint64  bytesPulled = 0;
            int     rowsPushed  = 0;
            int     rowsPulled  = 0;
        };
        QList<TableDetail> tables;
    };

    // Number of data points visible in the chart at one time.
    static constexpr int kVisiblePoints = 60;

    // Maximum points retained while the window is open.
    // Cleared entirely on hide/close, so this only caps a very long session.
    static constexpr int kMaxPoints = 500;

    QList<DataPoint> m_data;

    QChartView    *m_chartView      = nullptr;
    QLineSeries   *m_uploadSeries   = nullptr;
    QLineSeries   *m_downloadSeries = nullptr;
    QDateTimeAxis *m_axisX          = nullptr;
    QValueAxis    *m_axisY          = nullptr;
    QScrollBar    *m_scrollBar      = nullptr;
    QPlainTextEdit *m_logView       = nullptr;
};
