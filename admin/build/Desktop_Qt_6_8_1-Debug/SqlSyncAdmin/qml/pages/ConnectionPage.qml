import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    title: "Connect to PostgreSQL"

    property bool testing: false
    property string statusMessage: ""
    property bool statusIsError: false

    Connections {
        target: Admin
        function onConnectionTestResult(success, message) {
            testing = false
            statusMessage = message
            statusIsError = !success
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Header ──────────────────────────────────────────────────────
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
                        text: "Connect to PostgreSQL"
                        font.pixelSize: 20
                        font.weight: Font.Medium
                        color: Material.color(Material.BlueGrey, Material.Shade800)
                    }
                    Text {
                        text: "Enter your PostgreSQL superuser credentials. The database 'sqlitesyncpro' will be created automatically."
                        font.pixelSize: 13
                        color: Material.color(Material.BlueGrey, Material.Shade500)
                    }
                }
            }

            // ── Form ────────────────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 32
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label { text: "Host" }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "localhost"
                            text: Admin?.host ?? ""
                            onTextChanged: { if (Admin) Admin.host = text }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 100
                        spacing: 4
                        Label { text: "Port" }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "5432"
                            text: Admin?.port ?? 5432
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 1; top: 65535 }
                            onTextChanged: {
                                const v = parseInt(text)
                                if (!isNaN(v) && Admin) Admin.port = v
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label { text: "Superuser" }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "postgres"
                        text: Admin?.superuser ?? ""
                        onTextChanged: { if (Admin) Admin.superuser = text }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label { text: "Password" }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "••••••••"
                        echoMode: TextInput.Password
                        text: Admin?.superPass ?? ""
                        onTextChanged: { if (Admin) Admin.superPass = text }
                    }
                }

                // Status banner (shown only after a test)
                Rectangle {
                    Layout.fillWidth: true
                    height: statusMessage.length > 0 ? statusRow.implicitHeight + 16 : 0
                    visible: statusMessage.length > 0
                    radius: 4
                    color: statusIsError
                           ? Material.color(Material.Red, Material.Shade50)
                           : Material.color(Material.Green, Material.Shade50)
                    border.color: statusIsError
                                  ? Material.color(Material.Red, Material.Shade200)
                                  : Material.color(Material.Green, Material.Shade200)

                    RowLayout {
                        id: statusRow
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 12
                        }
                        spacing: 8
                        Text {
                            text: statusIsError ? "✗" : "✓"
                            font.pixelSize: 16
                            color: statusIsError
                                   ? Material.color(Material.Red, Material.Shade700)
                                   : Material.color(Material.Green, Material.Shade700)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: statusMessage
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            color: statusIsError
                                   ? Material.color(Material.Red, Material.Shade800)
                                   : Material.color(Material.Green, Material.Shade800)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: testing ? "Testing…" : "Test Connection"
                        enabled: !testing &&
                                 (Admin?.host?.length ?? 0) > 0 &&
                                 (Admin?.superPass?.length ?? 0) > 0
                        flat: true
                        onClicked: {
                            if (!Admin) return
                            testing = true
                            statusMessage = ""
                            Admin.testConnection()
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Next →"
                        Material.background: Material.accentColor
                        Material.foreground: "white"
                        enabled: (Admin?.host?.length ?? 0) > 0 &&
                                 (Admin?.superPass?.length ?? 0) > 0
                        onClicked: {
                            if (!Admin) return
                            Admin.saveConnectionSettings()
                            Admin.checkIfAlreadySetup()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: Admin
        function onAlreadySetup(isSetup) {
            if (isSetup) {
                root.goToPage(4)
            } else {
                root.nextPage()
            }
        }
    }
}
