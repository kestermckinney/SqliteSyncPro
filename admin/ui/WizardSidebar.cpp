#include "WizardSidebar.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

const QStringList WizardSidebar::kStepTitles = {
    QStringLiteral("Connection"),
    QStringLiteral("Configure"),
    QStringLiteral("Install"),
    QStringLiteral("Summary"),
    QStringLiteral("Users"),
    QStringLiteral("Remove All")
};

WizardSidebar::WizardSidebar(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void WizardSidebar::setCurrentStep(int step)
{
    m_currentStep = step;
    update();
}

void WizardSidebar::setSetupDone(bool done)
{
    m_setupDone = done;
    update();
}

QRect WizardSidebar::stepRect(int index) const
{
    return QRect(0, kHeaderHeight + index * kStepHeight, width(), kStepHeight);
}

void WizardSidebar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background: BlueGrey 800
    p.fillRect(rect(), QColor(0x37, 0x47, 0x4f));

    // App title
    QFont titleFont = font();
    titleFont.setPixelSize(15);
    titleFont.setWeight(QFont::Medium);
    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(QRect(20, 20, width() - 20, kHeaderHeight - 20),
               Qt::AlignTop | Qt::TextWordWrap,
               QStringLiteral("SQLSync\nAdministrator"));

    for (int i = 0; i < kStepTitles.size(); ++i) {
        const QRect sr       = stepRect(i);
        const bool isRemoveAll = (i == kStepTitles.size() - 1); // last entry is danger action
        const bool isActive  = (i == m_currentStep);
        const bool isDone    = (i < m_currentStep) && !isRemoveAll;
        const bool isEnabled = isRemoveAll || (i <= m_currentStep) || (i == 4 && m_setupDone);
        const bool isHovered = (i == m_hoveredStep) && isEnabled && !isActive;

        // Draw a separator line before "Remove All"
        if (isRemoveAll) {
            p.setPen(QColor(255, 255, 255, 40));
            p.drawLine(12, sr.top() - 8, width() - 12, sr.top() - 8);
        }

        // Row background
        if (isActive && isRemoveAll)
            p.fillRect(sr, QColor(183, 28, 28, 60)); // dark red tint
        else if (isActive)
            p.fillRect(sr, QColor(255, 255, 255, 20));
        else if (isHovered && isRemoveAll)
            p.fillRect(sr, QColor(183, 28, 28, 35));
        else if (isHovered)
            p.fillRect(sr, QColor(255, 255, 255, 13));

        // Active indicator bar on left edge
        if (isActive)
            p.fillRect(QRect(0, sr.top(), 4, sr.height()),
                       isRemoveAll ? QColor(0xe5, 0x39, 0x35)  // red
                                   : QColor(0x21, 0x96, 0xf3)); // blue

        // Circle / icon
        const int cx = 30;
        const int cy = sr.center().y();
        const int r  = 13;

        if (isRemoveAll) {
            // Red circle with warning symbol
            p.setBrush(isActive ? QColor(0xc6, 0x28, 0x28) : QColor(0x8d, 0x1a, 0x1a));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPoint(cx, cy), r, r);

            QFont iconFont = font();
            iconFont.setPixelSize(13);
            iconFont.setWeight(QFont::Bold);
            p.setFont(iconFont);
            p.setPen(QColor(0xff, 0xcc, 0x00)); // amber warning color
            p.drawText(QRect(cx - r, cy - r, r * 2, r * 2),
                       Qt::AlignCenter, QStringLiteral("\u26a0"));
        } else {
            QColor circleColor;
            if (isDone)
                circleColor = QColor(0x66, 0xbb, 0x6a);
            else if (isActive)
                circleColor = QColor(0x21, 0x96, 0xf3);
            else
                circleColor = QColor(0x54, 0x6e, 0x7a);

            p.setBrush(circleColor);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPoint(cx, cy), r, r);

            QFont circleFont = font();
            circleFont.setPixelSize(11);
            circleFont.setWeight(QFont::Medium);
            p.setFont(circleFont);
            p.setPen(Qt::white);
            p.drawText(QRect(cx - r, cy - r, r * 2, r * 2),
                       Qt::AlignCenter,
                       isDone ? QStringLiteral("\u2713") : QString::number(i + 1));
        }

        // Step title
        QFont labelFont = font();
        labelFont.setPixelSize(13);
        if (isRemoveAll)
            labelFont.setWeight(QFont::Medium);
        p.setFont(labelFont);

        QColor labelColor;
        if (isRemoveAll) {
            labelColor = isActive ? QColor(0xff, 0x8a, 0x80)  // light red when active
                                  : QColor(0xef, 0x9a, 0x9a); // muted red otherwise
        } else if (isActive) {
            labelColor = Qt::white;
        } else if (isEnabled) {
            labelColor = QColor(255, 255, 255, 178);
        } else {
            labelColor = QColor(255, 255, 255, 89);
        }

        p.setPen(labelColor);
        p.drawText(QRect(cx + r + 10, sr.top(), width() - cx - r - 20, sr.height()),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   kStepTitles.at(i));
    }

    // Version watermark
    QFont versionFont = font();
    versionFont.setPixelSize(11);
    p.setFont(versionFont);
    p.setPen(QColor(255, 255, 255, 77));
    p.drawText(QRect(0, height() - 28, width(), 20),
               Qt::AlignCenter,
               QStringLiteral("v0.1.0"));
}

void WizardSidebar::mousePressEvent(QMouseEvent *event)
{
    for (int i = 0; i < kStepTitles.size(); ++i) {
        if (stepRect(i).contains(event->pos())) {
            emit stepClicked(i);
            return;
        }
    }
}

void WizardSidebar::mouseMoveEvent(QMouseEvent *event)
{
    int hovered = -1;
    for (int i = 0; i < kStepTitles.size(); ++i) {
        if (stepRect(i).contains(event->pos())) {
            hovered = i;
            break;
        }
    }
    if (hovered != m_hoveredStep) {
        m_hoveredStep = hovered;
        update();
        setCursor(hovered >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
}

void WizardSidebar::leaveEvent(QEvent *)
{
    m_hoveredStep = -1;
    setCursor(Qt::ArrowCursor);
    update();
}
