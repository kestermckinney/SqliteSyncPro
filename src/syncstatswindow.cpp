// Copyright (C) 2026 Paul McKinney
#include "syncstatswindow.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QHideEvent>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QPointF>
#include <QTextCursor>
#include <QPainter>

SyncStatsWindow::SyncStatsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Sync Network Stats"));
    resize(750, 480);
    setWindowFlags(Qt::Window);

    // ── Series ─────────────────────────────────────────────────────────────
    m_uploadSeries   = new QLineSeries;
    m_downloadSeries = new QLineSeries;

    m_uploadSeries->setName(tr("Upload (KB)"));
    m_downloadSeries->setName(tr("Download (KB)"));

    QPen uploadPen(QColor(0xFF, 0xA5, 0x00));  // orange
    uploadPen.setWidth(2);
    m_uploadSeries->setPen(uploadPen);

    QPen downloadPen(QColor(0x00, 0xD7, 0xFF)); // cyan
    downloadPen.setWidth(2);
    m_downloadSeries->setPen(downloadPen);

    // ── Chart ──────────────────────────────────────────────────────────────
    auto *chart = new QChart;
    chart->addSeries(m_uploadSeries);
    chart->addSeries(m_downloadSeries);

    chart->setBackgroundBrush(QBrush(QColor(0x1E, 0x1E, 0x1E)));
    chart->setPlotAreaBackgroundBrush(QBrush(QColor(0x12, 0x12, 0x12)));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setMargins(QMargins(6, 6, 6, 6));

    // X axis – wall-clock time labels
    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat(QStringLiteral("HH:mm:ss"));
    m_axisX->setTickCount(7);
    m_axisX->setLabelsColor(QColor(0xAA, 0xAA, 0xAA));
    m_axisX->setGridLineColor(QColor(0x33, 0x33, 0x33));
    m_axisX->setTitleText(tr("Time  (older \u2190 \u2192 newer)"));
    m_axisX->setTitleBrush(QBrush(QColor(0xAA, 0xAA, 0xAA)));
    m_axisX->setLinePen(QPen(QColor(0x44, 0x44, 0x44)));

    // Initialise range to now..now+1s so the axis is valid before data arrives
    const QDateTime now = QDateTime::currentDateTime();
    m_axisX->setRange(now, now.addSecs(1));

    // Y axis – KB per cycle
    m_axisY = new QValueAxis;
    m_axisY->setRange(0, 10);
    m_axisY->setLabelFormat(QStringLiteral("%.1f"));
    m_axisY->setLabelsColor(QColor(0xAA, 0xAA, 0xAA));
    m_axisY->setGridLineColor(QColor(0x33, 0x33, 0x33));
    m_axisY->setTitleText(tr("KB / cycle"));
    m_axisY->setTitleBrush(QBrush(QColor(0xAA, 0xAA, 0xAA)));
    m_axisY->setLinePen(QPen(QColor(0x44, 0x44, 0x44)));

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_uploadSeries->attachAxis(m_axisX);
    m_uploadSeries->attachAxis(m_axisY);
    m_downloadSeries->attachAxis(m_axisX);
    m_downloadSeries->attachAxis(m_axisY);

    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setBackgroundVisible(false);

    // The chart view fills the available space; the chart scales to it.
    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(QBrush(QColor(0x1E, 0x1E, 0x1E)));

    // ── Scroll bar (history navigation) ────────────────────────────────────
    // Range [0, max(0, total - kVisiblePoints)].
    // At maximum → chart auto-follows newest data.
    m_scrollBar = new QScrollBar(Qt::Horizontal);
    m_scrollBar->setRange(0, 0);
    m_scrollBar->setPageStep(kVisiblePoints);
    m_scrollBar->setSingleStep(1);
    m_scrollBar->setFixedHeight(16);
    m_scrollBar->setStyleSheet(
        QStringLiteral("QScrollBar:horizontal { background: #2A2A2A; height: 16px; }"
                       "QScrollBar::handle:horizontal { background: #555555; min-width: 20px; border-radius: 3px; }"
                       "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"));

    connect(m_scrollBar, &QScrollBar::valueChanged,
            this,        &SyncStatsWindow::onScrollChanged);

    // ── Log area ───────────────────────────────────────────────────────────
    m_logView = new QPlainTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(1000);
    QFont mono(QStringLiteral("Courier New"), 9);
    mono.setFixedPitch(true);
    m_logView->setFont(mono);
    m_logView->setFixedHeight(140);
    m_logView->setStyleSheet(
        QStringLiteral("QPlainTextEdit {"
                       "  background-color: #1E1E1E;"
                       "  color: #D4D4D4;"
                       "  border: none;"
                       "}"));

    // ── Layout ─────────────────────────────────────────────────────────────
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    layout->addWidget(m_chartView, 1);    // chart fills available vertical space
    layout->addWidget(m_scrollBar, 0);    // thin scroll bar just below chart
    layout->addWidget(m_logView,   0);
}

// ---------------------------------------------------------------------------
// Event overrides
// ---------------------------------------------------------------------------

void SyncStatsWindow::closeEvent(QCloseEvent *event)
{
    emit windowClosed();
    QWidget::closeEvent(event);  // hides window → triggers hideEvent → clearData()
}

void SyncStatsWindow::hideEvent(QHideEvent *event)
{
    // Release all data immediately when the window is hidden so memory is freed.
    // The next show() call starts a fresh session.
    clearData();
    QWidget::hideEvent(event);
}

// ---------------------------------------------------------------------------
// Scroll bar
// ---------------------------------------------------------------------------

void SyncStatsWindow::onScrollChanged(int /*value*/)
{
    // User scrolled — just redraw the visible window without touching the
    // scroll bar range or value.
    updateChartDisplay();
}

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

QString SyncStatsWindow::formatBytes(qint64 bytes) const
{
    if (bytes >= 1024 * 1024)
        return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
    if (bytes >= 1024)
        return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 B").arg(bytes);
}

void SyncStatsWindow::addDataPoint(const SyncResult &result)
{
    // Remember whether the scrollbar was tracking the latest data before
    // we append the new point (or if there isn't enough data to scroll yet).
    const bool atEnd = (m_scrollBar->value() >= m_scrollBar->maximum())
                       || (m_scrollBar->maximum() == 0);

    // Build data point — each value is per-cycle bytes for THIS result only.
    DataPoint dp;
    dp.timestamp = QDateTime::currentDateTime();

    for (const TableSyncResult &tr : result.tableResults) {
        dp.uploadKB   += static_cast<double>(tr.bytesPushed) / 1024.0;
        dp.downloadKB += static_cast<double>(tr.bytesPulled) / 1024.0;

        DataPoint::TableDetail detail;
        detail.tableName   = tr.tableName;
        detail.bytesPushed = tr.bytesPushed;
        detail.bytesPulled = tr.bytesPulled;
        detail.rowsPushed  = tr.pushed;
        detail.rowsPulled  = tr.pulled;
        dp.tables.append(detail);
    }

    if (m_data.size() >= kMaxPoints)
        m_data.removeFirst();
    m_data.append(dp);

    // Update scroll bar range
    const int maxScroll = qMax(0, m_data.size() - kVisiblePoints);
    // Block the valueChanged signal while we update the range so onScrollChanged
    // doesn't fire prematurely with a stale value.
    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, maxScroll);
    m_scrollBar->setPageStep(kVisiblePoints);
    if (atEnd)
        m_scrollBar->setValue(maxScroll);  // follow the latest data
    m_scrollBar->blockSignals(false);

    updateChartDisplay();

    // Append to text log
    const qint64 totalUp   = result.totalBytesPushed();
    const qint64 totalDown = result.totalBytesPulled();

    QString logLine = QStringLiteral("[%1]  \u2191 %2  \u2193 %3")
        .arg(dp.timestamp.toString(QStringLiteral("HH:mm:ss")))
        .arg(formatBytes(totalUp))
        .arg(formatBytes(totalDown));

    for (const auto &td : dp.tables) {
        if (td.bytesPushed > 0 || td.bytesPulled > 0
                || td.rowsPushed > 0 || td.rowsPulled > 0) {
            logLine += QStringLiteral("\n  %1 push: %2 rows / %3   pull: %4 rows / %5")
                .arg(td.tableName.leftJustified(28))
                .arg(td.rowsPushed)
                .arg(formatBytes(td.bytesPushed))
                .arg(td.rowsPulled)
                .arg(formatBytes(td.bytesPulled));
        }
    }

    m_logView->appendPlainText(logLine);
    QTextCursor c = m_logView->textCursor();
    c.movePosition(QTextCursor::End);
    m_logView->setTextCursor(c);
}

void SyncStatsWindow::clearData()
{
    m_data.clear();
    m_uploadSeries->clear();
    m_downloadSeries->clear();

    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, 0);
    m_scrollBar->setValue(0);
    m_scrollBar->blockSignals(false);

    const QDateTime now = QDateTime::currentDateTime();
    m_axisX->setRange(now, now.addSecs(1));
    m_axisY->setRange(0, 10);

    m_logView->clear();
}

// ---------------------------------------------------------------------------
// Chart rendering
// ---------------------------------------------------------------------------

void SyncStatsWindow::updateChartDisplay()
{
    const int total = m_data.size();
    if (total == 0)
        return;

    const int start = m_scrollBar->value();
    const int end   = qMin(total, start + kVisiblePoints);

    // Build series points for the visible window
    QList<QPointF> uploadPoints, downloadPoints;
    uploadPoints.reserve(end - start);
    downloadPoints.reserve(end - start);

    double maxVal = 1.0;
    for (int i = start; i < end; ++i) {
        const qreal x = static_cast<qreal>(m_data[i].timestamp.toMSecsSinceEpoch());
        uploadPoints.append({x, m_data[i].uploadKB});
        downloadPoints.append({x, m_data[i].downloadKB});
        maxVal = qMax(maxVal, qMax(m_data[i].uploadKB, m_data[i].downloadKB));
    }

    m_uploadSeries->replace(uploadPoints);
    m_downloadSeries->replace(downloadPoints);

    // X axis: span the visible window; extend the right edge by one estimated
    // interval so the last point isn't crammed against the axis boundary.
    QDateTime xMin = m_data[start].timestamp;
    QDateTime xMax;
    if (end < total) {
        // Next real data point is the natural right boundary
        xMax = m_data[end].timestamp;
    } else if (end - start >= 2) {
        // Estimate the interval from the last two points
        const qint64 intervalMs = m_data[end - 2].timestamp.msecsTo(m_data[end - 1].timestamp);
        xMax = m_data[end - 1].timestamp.addMSecs(qMax(intervalMs, qint64(1000)));
    } else {
        xMax = m_data[end - 1].timestamp.addSecs(30);
    }
    m_axisX->setRange(xMin, xMax);

    // Y axis: per-cycle KB with 20 % headroom; minimum range of 10 KB
    double yMax = maxVal * 1.2;
    if (yMax < 10.0)
        yMax = 10.0;
    m_axisY->setRange(0.0, yMax);
}
