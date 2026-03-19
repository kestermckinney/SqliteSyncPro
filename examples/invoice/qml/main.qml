import QtQuick
import QtQuick.Controls.Material
import InvoiceDemo

ApplicationWindow {
    id:      root
    title:   "Invoice Sync Demo"
    width:   1100
    height:  720
    visible: true

    Material.theme:  Material.Light
    Material.accent: Material.Teal

    // ── Shared record delegate ────────────────────────────────────────────────
    Component {
        id: recordDelegate

        Rectangle {
            width:  ListView.view.width
            height: modelData.rowType === "invoice" ? 52 : 40
            color:  modelData.rowType === "invoice"
                        ? (index % 2 === 0 ? "#f0f9f8" : "#e8f5f4")
                        : (index % 2 === 0 ? "white"   : "#fafafa")

            // Left accent stripe — green when synced, amber when pending
            Rectangle {
                width:  4
                height: parent.height
                color:  modelData.synced
                            ? Material.color(Material.Teal,   Material.Shade400)
                            : Material.color(Material.Orange, Material.Shade400)
            }

            Row {
                anchors {
                    left:           parent.left
                    right:          parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin:     modelData.rowType === "invoice" ? 16 : 32
                    rightMargin:    12
                }
                spacing: 0

                // Row content
                Column {
                    width: parent.width - syncBadge.width - 8
                    spacing: 2

                    Label {
                        width:          parent.width
                        text: modelData.rowType === "invoice"
                                  ? "\uD83D\uDCC4  " + modelData.invoiceNumber
                                  : "    \u2192  Line " + modelData.lineNumber
                                    + "  \u2022  " + modelData.description
                        font.pixelSize: modelData.rowType === "invoice" ? 14 : 13
                        font.weight:    modelData.rowType === "invoice" ? Font.Medium : Font.Normal
                        color:          Material.color(Material.BlueGrey, Material.Shade800)
                        elide:          Text.ElideRight
                    }

                    Label {
                        width:          parent.width
                        visible:        text.length > 0
                        text: modelData.rowType === "invoice"
                                  ? modelData.address
                                  : "qty: " + modelData.quantity
                                    + "   price: $" + modelData.price.toFixed(2)
                        font.pixelSize: 11
                        color:          Material.color(Material.BlueGrey, Material.Shade500)
                        elide:          Text.ElideRight
                    }
                }

                // Sync status badge
                Rectangle {
                    id:     syncBadge
                    width:  modelData.synced ? 54 : 62
                    height: 20
                    radius: 10
                    color:  modelData.synced
                                ? Material.color(Material.Teal,   Material.Shade100)
                                : Material.color(Material.Orange, Material.Shade100)
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        anchors.centerIn: parent
                        text:             modelData.synced ? "\u2713 synced" : "\u23F3 pending"
                        font.pixelSize:   10
                        color:            modelData.synced
                                              ? Material.color(Material.Teal,   Material.Shade700)
                                              : Material.color(Material.Orange, Material.Shade700)
                    }
                }
            }

            // Bottom separator
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1
                color:  Material.color(Material.BlueGrey, Material.Shade100)
            }
        }
    }

    // ── Main form ─────────────────────────────────────────────────────────────
    MainForm {
        id:           form
        anchors.fill: parent

        Component.onCompleted: {
            sourceView.model    = Controller.sourceRecords
            sourceView.delegate = recordDelegate
            destView.model      = Controller.destRecords
            destView.delegate   = recordDelegate
        }

        // ── Button handlers ───────────────────────────────────────────────
        settingsButton.onClicked: Controller.showSettings()

        syncButton.onClicked: Controller.runSync()
    }

    // ── React to Controller signals ───────────────────────────────────────────
    Connections {
        target: Controller

        function onStatusChanged() {
            form.statusLabel.text = Controller.statusText
        }

        function onBusyChanged() {
            form.busyIndicator.running = Controller.busy
            form.settingsButton.enabled = !Controller.busy
            form.syncButton.enabled     = !Controller.busy
        }

        function onSourceRecordsChanged() {
            form.sourceView.model = Controller.sourceRecords
        }

        function onDestRecordsChanged() {
            form.destView.model = Controller.destRecords
        }
    }
}
