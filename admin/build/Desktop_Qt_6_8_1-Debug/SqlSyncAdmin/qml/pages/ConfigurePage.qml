import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    title: "Review Configuration"

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
                        text: "Review Configuration"
                        font.pixelSize: 20
                        font.weight: Font.Medium
                        color: Material.color(Material.BlueGrey, Material.Shade800)
                    }
                    Text {
                        text: "Confirm the settings below, then click Install to proceed."
                        font.pixelSize: 13
                        color: Material.color(Material.BlueGrey, Material.Shade500)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 32
                spacing: 20

                // Summary card
                Rectangle {
                    Layout.fillWidth: true
                    radius: 6
                    color: Material.color(Material.BlueGrey, Material.Shade50)
                    border.color: Material.color(Material.BlueGrey, Material.Shade200)
                    height: summaryCol.implicitHeight + 24

                    ColumnLayout {
                        id: summaryCol
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            margins: 16
                        }
                        spacing: 10

                        Text {
                            text: "PostgreSQL Connection"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: Material.color(Material.BlueGrey, Material.Shade700)
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 16

                            Text { text: "Host:";     font.pixelSize: 13; color: Material.color(Material.BlueGrey, Material.Shade500) }
                            Text { text: Admin.host + ":" + Admin.port; font.pixelSize: 13 }

                            Text { text: "Database:"; font.pixelSize: 13; color: Material.color(Material.BlueGrey, Material.Shade500) }
                            Text { text: "sqlitesyncpro (auto-created)"; font.pixelSize: 13 }

                            Text { text: "Superuser:"; font.pixelSize: 13; color: Material.color(Material.BlueGrey, Material.Shade500) }
                            Text { text: Admin.superuser; font.pixelSize: 13 }
                        }
                    }
                }

                // What will be installed
                Text {
                    text: "The following will be installed in your PostgreSQL database:"
                    font.pixelSize: 13
                    color: Material.color(Material.BlueGrey, Material.Shade700)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            "pgcrypto extension",
                            "Roles: authenticator, anon, app_user",
                            "JWT secret stored as database GUC",
                            "JWT helper functions (_base64url_encode, _sign_jwt)",
                            "auth_users table with bcrypt password hashing",
                            "sync_data table with Row-Level Security",
                            "Application functions: create_user, rpc_login"
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "•"
                                font.pixelSize: 16
                                color: Material.accentColor
                            }
                            Text {
                                text: modelData
                                font.pixelSize: 13
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                // Warning note
                Rectangle {
                    Layout.fillWidth: true
                    radius: 4
                    color: Material.color(Material.Amber, Material.Shade50)
                    border.color: Material.color(Material.Amber, Material.Shade300)
                    height: noteRow.implicitHeight + 16

                    RowLayout {
                        id: noteRow
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 12
                        }
                        spacing: 8

                        Text {
                            text: "⚠"
                            font.pixelSize: 18
                            color: Material.color(Material.Amber, Material.Shade800)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "The installation is idempotent — running it on an existing " +
                                  "database is safe. Roles and tables that already exist " +
                                  "will be updated, not duplicated."
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            color: Material.color(Material.Amber, Material.Shade900)
                        }
                    }
                }

                // Buttons
                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "← Back"
                        flat: true
                        onClicked: root.goToPage(0)
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Install →"
                        Material.background: Material.accentColor
                        Material.foreground: "white"
                        onClicked: {
                            root.nextPage()          // advance to InstallPage
                            Admin.startSetup()       // kick off the worker
                        }
                    }
                }
            }
        }
    }
}
