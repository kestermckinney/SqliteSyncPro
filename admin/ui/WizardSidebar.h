#pragma once

#include <QWidget>
#include <QStringList>

class WizardSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit WizardSidebar(QWidget *parent = nullptr);

    void setCurrentStep(int step);
    void setSetupDone(bool done);

signals:
    void stepClicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRect stepRect(int index) const;

    int  m_currentStep = 0;
    bool m_setupDone   = false;
    int  m_hoveredStep = -1;

    static const QStringList kStepTitles;
    static constexpr int kStepHeight  = 48;
    static constexpr int kHeaderHeight = 88;
};
