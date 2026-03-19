import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    title: "Installing…"

    // Populated by stepCompleted signals
    ListModel { id: logModel }

    property int completedSteps: 0
    property bool finished: false
    property bool success: false
    property string finishMessage: ""

    Connections {
        target: Admin

        function onStepCompleted(index, ok, stepName, detail) {
            logModel.append({
                "ok":       ok,
                "stepName": stepName,
                "detail":   detail
            })
            if (ok) completedSteps++
            logView.positionViewAtEnd()
        }

        function onSetupFinished(ok, message) {
            finished = true
            success  = ok
            finishMessage = message
            if (!ok) logView.positionViewAtEnd()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Header ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 72
            color: Material.color(Material.BlueGrey, Material.Shade50)

            ColumnLayout {
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                    leftMargin: 32
                }
                spacing: 2
                Text {
                    text: finished
                          ? (success ? "Installation Complete" : "Installation Failed")
                          : "Installing…"
                    font.pixelSize: 20
                    font.weight: Font.Medium
                    color: finished && !success
                           ? Material.color(Material.Red, Material.Shade700)
                           : Material.color(Material.BlueGrey, Material.Shade800)
                }
                Text {
                    text: finished
                          ? (success
                             ? "All steps completed successfully."
                             : finishMessage)
                          : "Please wait while the database is being configured."
                    font.pixelSize: 13
                    color: Material.color(Material.BlueGrey, Material.Shade500)
                }
            }
        }

        // ── Progress bar ─────────────────────────────────────────────────────
        ProgressBar {
            Layout.fillWidth: true
            Layout.leftMargin: 32
            Layout.rightMargin: 32
            Layout.topMargin: 16
            from: 0
            to: Admin.totalSteps
            value: completedSteps
            indeterminate: !finished && completedSteps === 0
        }

        Text {
            Layout.leftMargin: 32
            Layout.topMargin: 4
            text: finished
                  ? (success ? "Done" : "Failed")
                  : (completedSteps + " / " + Admin.totalSteps + " steps")
            font.pixelSize: 12
            color: Material.color(Material.BlueGrey, Material.Shade500)
        }

        // ── Step log ─────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 32
            Layout.topMargin: 12
            radius: 4
            color: "#1a1a2e"

            ListView {
                id: logView
                anchors {
                    fill: parent
                    margins: 12
                }
                model: logModel
                clip: true
                spacing: 2

                delegate: RowLayout {
                    width: logView.width
                    spacing: 8

                    Text {
                        text: model.ok ? "✓" : "✗"
                        font.pixelSize: 13
                        font.family: "monospace"
                        color: model.ok ? "#4caf50" : "#f44336"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: model.ok
                              ? model.stepName
                              : model.stepName + (model.detail.length > 0
                                                  ? "\n  " + model.detail
                                                  : "")
                        font.pixelSize: 12
                        font.family: "monospace"
                        color: model.ok ? "#e0e0e0" : "#ff8a80"
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // ── Bottom bar ───────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 32
            Layout.topMargin: 8
            spacing: 12

            Button {
                text: "Cancel"
                flat: true
                visible: !finished
                onClicked: Admin.cancelSetup()
            }

            Item { Layout.fillWidth: true }

            // "Next" only appears on failure (to go back); on success the
            // main.qml Connections block auto-navigates to SummaryPage.
            Button {
                text: "← Back to Configure"
                flat: true
                visible: finished && !success
                onClicked: root.goToPage(1)
            }
        }
    }
}
