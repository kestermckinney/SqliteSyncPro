import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    title: "Setup Complete"

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
                color: Material.color(Material.Green, Material.Shade50)

                RowLayout {
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                        leftMargin: 32
                    }
                    spacing: 12

                    Text {
                        text: "✓"
                        font.pixelSize: 28
                        color: Material.color(Material.Green, Material.Shade600)
                    }

                    ColumnLayout {
                        spacing: 2
                        Text {
                            text: "Setup Complete"
                            font.pixelSize: 20
                            font.weight: Font.Medium
                            color: Material.color(Material.Green, Material.Shade800)
                        }
                        Text {
                            text: "Your PostgreSQL database is ready for SQLSync."
                            font.pixelSize: 13
                            color: Material.color(Material.Green, Material.Shade700)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 32
                spacing: 20

                // ── Credentials ─────────────────────────────────────────────
                Text {
                    text: "Generated Credentials"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: Material.color(Material.BlueGrey, Material.Shade800)
                }

                Text {
                    Layout.fillWidth: true
                    text: "Save these values — the passwords cannot be recovered after " +
                          "you close this window."
                    font.pixelSize: 13
                    color: Material.color(Material.Red, Material.Shade600)
                    wrapMode: Text.WordWrap
                }

                // Authenticator password row
                Rectangle {
                    Layout.fillWidth: true
                    radius: 4
                    color: Material.color(Material.BlueGrey, Material.Shade50)
                    border.color: Material.color(Material.BlueGrey, Material.Shade200)
                    height: authRow.implicitHeight + 16

                    RowLayout {
                        id: authRow
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 12
                        }
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "AUTHENTICATOR PASSWORD"
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                color: Material.color(Material.BlueGrey, Material.Shade500)
                            }
                            Text {
                                text: Admin.authenticatorPassword
                                font.pixelSize: 13
                                font.family: "monospace"
                                Layout.fillWidth: true
                                wrapMode: Text.WrapAnywhere
                            }
                        }
                        Button {
                            text: "Copy"
                            flat: true
                            onClicked: Admin.copyToClipboard(Admin.authenticatorPassword)
                        }
                    }
                }

                // JWT secret row
                Rectangle {
                    Layout.fillWidth: true
                    radius: 4
                    color: Material.color(Material.BlueGrey, Material.Shade50)
                    border.color: Material.color(Material.BlueGrey, Material.Shade200)
                    height: jwtRow.implicitHeight + 16

                    RowLayout {
                        id: jwtRow
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 12
                        }
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "JWT SECRET"
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                color: Material.color(Material.BlueGrey, Material.Shade500)
                            }
                            Text {
                                text: Admin.jwtSecret
                                font.pixelSize: 13
                                font.family: "monospace"
                                Layout.fillWidth: true
                                wrapMode: Text.WrapAnywhere
                            }
                        }
                        Button {
                            text: "Copy"
                            flat: true
                            onClicked: Admin.copyToClipboard(Admin.jwtSecret)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Material.color(Material.BlueGrey, Material.Shade200)
                }

                // ── PostgREST config ─────────────────────────────────────────
                Text {
                    text: "PostgREST Configuration"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: Material.color(Material.BlueGrey, Material.Shade800)
                }

                Text {
                    Layout.fillWidth: true
                    text: "Paste this block into your postgrest.conf file:"
                    font.pixelSize: 13
                    color: Material.color(Material.BlueGrey, Material.Shade600)
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 4
                    color: "#1a1a2e"
                    height: confText.implicitHeight + 56

                    Text {
                        id: confText
                        anchors {
                            left: parent.left
                            right: copyConfBtn.left
                            top: parent.top
                            margins: 12
                        }
                        text: Admin.postgrestConfig()
                        font.pixelSize: 12
                        font.family: "monospace"
                        color: "#e0e0e0"
                        wrapMode: Text.WrapAnywhere
                    }

                    Button {
                        id: copyConfBtn
                        anchors {
                            right: parent.right
                            top: parent.top
                            margins: 6
                        }
                        text: "Copy"
                        flat: true
                        Material.foreground: "#90caf9"
                        onClicked: Admin.copyToClipboard(Admin.postgrestConfig())
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Material.color(Material.BlueGrey, Material.Shade200)
                }

                // ── Next steps ───────────────────────────────────────────────
                Text {
                    text: "Next Steps"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: Material.color(Material.BlueGrey, Material.Shade800)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            "Install PostgREST if you haven't already (postgrest.org)",
                            "Create your postgrest.conf using the block above",
                            "Start PostgREST: postgrest postgrest.conf",
                            "Add sync users using the Users page →"
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: (index + 1) + "."
                                font.pixelSize: 13
                                color: Material.accentColor
                                Layout.preferredWidth: 20
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

                // Buttons
                RowLayout {
                    Layout.fillWidth: true

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Manage Users →"
                        Material.background: Material.accentColor
                        Material.foreground: "white"
                        onClicked: root.goToPage(4)
                    }
                }
            }
        }
    }
}
