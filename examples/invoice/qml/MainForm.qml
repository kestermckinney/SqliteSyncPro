import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    // ── Expose controls for main.qml to wire up ──────────────────────────────
    property alias settingsButton: settingsButton
    property alias syncButton:     syncButton
    property alias statusLabel:    statusLabel
    property alias busyIndicator:  busyIndicator
    property alias sourceView:     sourceView
    property alias destView:       destView

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── App header ───────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: Material.color(Material.BlueGrey, Material.Shade800)

            ColumnLayout {
                anchors {
                    left:           parent.left
                    verticalCenter: parent.verticalCenter
                    leftMargin:     24
                }
                spacing: 2

                Label {
                    text:            "Invoice Sync Demo"
                    font.pixelSize:  20
                    font.weight:     Font.Medium
                    color:           "white"
                }
                Label {
                    text:           "SqliteSyncPro — two-device push / pull simulation"
                    font.pixelSize: 12
                    color:          Material.color(Material.BlueGrey, Material.Shade300)
                }
            }
        }

        // ── Toolbar ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           52
            color:            Material.color(Material.BlueGrey, Material.Shade50)
            border.color:     Material.color(Material.BlueGrey, Material.Shade200)
            border.width:     1

            RowLayout {
                anchors {
                    fill:        parent
                    leftMargin:  16
                    rightMargin: 16
                }
                spacing: 12

                Button {
                    id:   settingsButton
                    text: "Settings\u2026"
                    flat: true
                }

                Item { Layout.fillWidth: true }

                BusyIndicator {
                    id:                    busyIndicator
                    running:               false
                    Layout.preferredWidth: 32
                    Layout.preferredHeight:32
                }

                Button {
                    id:                  syncButton
                    text:                "Run Test"
                    Material.background: Material.accentColor
                    Material.foreground: "white"
                }
            }
        }

        // ── Status bar ───────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height:           30
            color:            Material.color(Material.BlueGrey, Material.Shade100)

            Label {
                id:             statusLabel
                anchors {
                    left:           parent.left
                    right:          parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin:     16
                    rightMargin:    16
                }
                text:           "Ready — configure settings and click Run Test."
                font.pixelSize: 12
                color:          Material.color(Material.BlueGrey, Material.Shade700)
                elide:          Text.ElideRight
            }
        }

        // ── Split panel: Source | Destination ────────────────────────────────
        RowLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing:           0

            // Source panel
            ColumnLayout {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                spacing:           0

                Rectangle {
                    Layout.fillWidth: true
                    height:           40
                    color:            Material.color(Material.Teal, Material.Shade600)

                    RowLayout {
                        anchors {
                            fill:        parent
                            leftMargin:  16
                            rightMargin: 16
                        }
                        Label {
                            text:           "SOURCE DATABASE"
                            font.pixelSize: 13
                            font.weight:    Font.Medium
                            color:          "white"
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text:           "pushed to server \u2191"
                            font.pixelSize: 11
                            color:          Material.color(Material.Teal, Material.Shade100)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    color:             "white"
                    clip:              true

                    ListView {
                        id:           sourceView
                        anchors.fill: parent
                        clip:         true
                    }

                    Label {
                        anchors.centerIn: parent
                        visible:          sourceView.count === 0
                        text:             "No records yet — click Run Test"
                        color:            Material.color(Material.BlueGrey, Material.Shade400)
                        font.pixelSize:   13
                    }
                }
            }

            // Divider
            Rectangle {
                Layout.fillHeight: true
                width:             1
                color:             Material.color(Material.BlueGrey, Material.Shade300)
            }

            // Destination panel
            ColumnLayout {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                spacing:           0

                Rectangle {
                    Layout.fillWidth: true
                    height:           40
                    color:            Material.color(Material.Indigo, Material.Shade600)

                    RowLayout {
                        anchors {
                            fill:        parent
                            leftMargin:  16
                            rightMargin: 16
                        }
                        Label {
                            text:           "DESTINATION DATABASE"
                            font.pixelSize: 13
                            font.weight:    Font.Medium
                            color:          "white"
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text:           "\u2193 pulled from server"
                            font.pixelSize: 11
                            color:          Material.color(Material.Indigo, Material.Shade100)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    color:             "white"
                    clip:              true

                    ListView {
                        id:           destView
                        anchors.fill: parent
                        clip:         true
                    }

                    Label {
                        anchors.centerIn: parent
                        visible:          destView.count === 0
                        text:             "No records yet — click Run Test"
                        color:            Material.color(Material.BlueGrey, Material.Shade400)
                        font.pixelSize:   13
                    }
                }
            }
        }
    }
}
