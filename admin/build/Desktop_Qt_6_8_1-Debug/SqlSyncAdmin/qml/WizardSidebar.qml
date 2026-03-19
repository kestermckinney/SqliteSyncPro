import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: Material.color(Material.BlueGrey, Material.Shade800)

    property int currentStep: 0
    property var stepTitles: []

    signal stepClicked(int index)

    ColumnLayout {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 24
        }
        spacing: 0

        // App title
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.bottomMargin: 32
            text: "SQLSync\nAdministrator"
            font.pixelSize: 16
            font.weight: Font.Medium
            color: "white"
            lineHeight: 1.3
        }

        // Step entries
        Repeater {
            model: sidebar.stepTitles

            delegate: ItemDelegate {
                id: stepItem
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                enabled: index <= sidebar.currentStep ||
                         (index === 4 && (Admin?.setupDone ?? false))

                onClicked: sidebar.stepClicked(index)

                // Active indicator bar
                Rectangle {
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: 4
                    color: Material.accentColor
                    visible: index === sidebar.currentStep
                }

                RowLayout {
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        leftMargin: 16
                        rightMargin: 12
                    }
                    spacing: 12

                    // Step circle
                    Rectangle {
                        width: 26
                        height: 26
                        radius: 13
                        color: {
                            if (index < sidebar.currentStep)
                                return Material.color(Material.Green, Material.Shade400)
                            if (index === sidebar.currentStep)
                                return Material.accentColor
                            return Material.color(Material.BlueGrey, Material.Shade600)
                        }

                        Text {
                            anchors.centerIn: parent
                            text: index < sidebar.currentStep ? "✓" : (index + 1)
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: "white"
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        font.pixelSize: 13
                        color: index === sidebar.currentStep
                               ? "white"
                               : (stepItem.enabled
                                  ? Qt.rgba(1, 1, 1, 0.7)
                                  : Qt.rgba(1, 1, 1, 0.35))
                        elide: Text.ElideRight
                    }
                }

                background: Rectangle {
                    color: index === sidebar.currentStep
                           ? Qt.rgba(1, 1, 1, 0.08)
                           : (stepItem.hovered && stepItem.enabled
                              ? Qt.rgba(1, 1, 1, 0.05)
                              : "transparent")
                }
            }
        }
    }

    // Version watermark
    Text {
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 12
        }
        text: "v0.1.0"
        font.pixelSize: 11
        color: Qt.rgba(1, 1, 1, 0.3)
    }
}
