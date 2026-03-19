import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls
import QtQuick.Layouts

Page {
    title: "Sync Users"

    Component.onCompleted: Admin.loadUsers()

    // ── Add user dialog ──────────────────────────────────────────────────────
    Dialog {
        id: addDialog
        title: "Add Sync User"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 400

        property string errorText: ""

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            errorText = ""
            if (!Admin) return
            Admin.addUser(addUsernameField.text.trim(), addPassField.text)
            addUsernameField.text = ""
            addPassField.text = ""
        }
        onRejected: {
            addUsernameField.text = ""
            addPassField.text = ""
            errorText = ""
        }

        ColumnLayout {
            width: parent.width
            spacing: 12

            Label { text: "Username (PostgreSQL role name)" }
            TextField {
                id: addUsernameField
                Layout.fillWidth: true
                placeholderText: "e.g. alice"
                inputMethodHints: Qt.ImhLowercaseOnly
            }

            Label { text: "Password (min. 8 characters)" }
            TextField {
                id: addPassField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "••••••••"
            }

            Text {
                Layout.fillWidth: true
                text: "Usernames must start with a letter or underscore,\n" +
                      "followed by letters, digits, or underscores only."
                font.pixelSize: 11
                color: Material.color(Material.BlueGrey, Material.Shade500)
                wrapMode: Text.WordWrap
            }

            Text {
                visible: addDialog.errorText.length > 0
                text: addDialog.errorText
                color: Material.color(Material.Red, Material.Shade700)
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
    }

    // ── Edit user dialog ─────────────────────────────────────────────────────
    Dialog {
        id: editDialog
        title: "Change Password"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 400

        property string targetUsername: ""
        property string errorText: ""

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            errorText = ""
            if (!Admin) return
            Admin.editUser(targetUsername, editPassField.text)
            editPassField.text = ""
        }
        onRejected: {
            editPassField.text = ""
            errorText = ""
        }

        ColumnLayout {
            width: parent.width
            spacing: 12

            Label { text: "User" }
            TextField {
                Layout.fillWidth: true
                text: editDialog.targetUsername
                readOnly: true
                background: Rectangle {
                    color: Material.color(Material.BlueGrey, Material.Shade100)
                    radius: 4
                }
            }

            Label { text: "New Password (min. 8 characters)" }
            TextField {
                id: editPassField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "••••••••"
            }

            Text {
                visible: editDialog.errorText.length > 0
                text: editDialog.errorText
                color: Material.color(Material.Red, Material.Shade700)
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
    }

    // ── Confirm remove dialog ────────────────────────────────────────────────
    Dialog {
        id: removeDialog
        title: "Remove User"
        modal: true
        anchors.centerIn: Overlay.overlay

        property string targetUsername: ""

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            if (Admin) Admin.removeUser(removeDialog.targetUsername)
        }

        Text {
            text: "Remove <b>" + removeDialog.targetUsername + "</b>?<br>" +
                  "The PostgreSQL role and all sync data will be deleted. This cannot be undone."
            wrapMode: Text.WordWrap
            width: 340
        }
    }

    // ── Snackbar ─────────────────────────────────────────────────────────────
    property string snackText: ""
    property bool snackError: false

    Timer {
        id: snackTimer
        interval: 3500
        onTriggered: snackText = ""
    }

    Connections {
        target: Admin

        function onUserAdded(success, message) {
            if (!success) addDialog.errorText = message
            snackText  = success ? "User added successfully." : ("Error: " + message)
            snackError = !success
            snackTimer.restart()
        }

        function onUserRemoved(success, message) {
            snackText  = success ? (message + " removed.") : ("Error: " + message)
            snackError = !success
            snackTimer.restart()
        }

        function onUserEdited(success, message) {
            if (!success) editDialog.errorText = message
            snackText  = success ? "Password updated." : ("Error: " + message)
            snackError = !success
            snackTimer.restart()
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

            RowLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 32
                    rightMargin: 32
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "Sync Users"
                        font.pixelSize: 20
                        font.weight: Font.Medium
                        color: Material.color(Material.BlueGrey, Material.Shade800)
                    }
                    Text {
                        text: "Each user is a PostgreSQL role with access to sync_data."
                        font.pixelSize: 13
                        color: Material.color(Material.BlueGrey, Material.Shade500)
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "+ Add User"
                    Material.background: Material.accentColor
                    Material.foreground: "white"
                    onClicked: {
                        addDialog.errorText = ""
                        addDialog.open()
                    }
                }
            }
        }

        // ── User list ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 32
            radius: 6
            border.color: Material.color(Material.BlueGrey, Material.Shade200)
            clip: true

            ColumnLayout {
                anchors.centerIn: parent
                visible: !Admin || Admin.users.length === 0
                spacing: 8

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "👤"
                    font.pixelSize: 36
                }
                Text {
                    text: "No sync users yet"
                    font.pixelSize: 15
                    color: Material.color(Material.BlueGrey, Material.Shade400)
                }
                Text {
                    text: "Click \"+ Add User\" to create one."
                    font.pixelSize: 13
                    color: Material.color(Material.BlueGrey, Material.Shade400)
                }
            }

            ListView {
                anchors.fill: parent
                model: Admin ? Admin.users : []
                visible: Admin && Admin.users.length > 0
                clip: true

                header: Rectangle {
                    width: ListView.view.width
                    height: 36
                    color: Material.color(Material.BlueGrey, Material.Shade100)

                    RowLayout {
                        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }

                        Text {
                            Layout.fillWidth: true
                            text: "Username (PostgreSQL Role)"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: Material.color(Material.BlueGrey, Material.Shade600)
                        }
                        Text {
                            Layout.preferredWidth: 180
                            text: "Created"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: Material.color(Material.BlueGrey, Material.Shade600)
                        }
                        Item { Layout.preferredWidth: 160 }
                    }
                }

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 48
                    color: index % 2 === 0 ? "white"
                                           : Material.color(Material.BlueGrey, Material.Shade50)

                    RowLayout {
                        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            text: modelData.username
                            font.pixelSize: 13
                            font.family: "monospace"
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.preferredWidth: 180
                            text: new Date(modelData.created_at).toLocaleDateString()
                            font.pixelSize: 12
                            color: Material.color(Material.BlueGrey, Material.Shade500)
                        }

                        Button {
                            Layout.preferredWidth: 70
                            text: "Edit"
                            flat: true
                            onClicked: {
                                editDialog.targetUsername = modelData.username
                                editDialog.errorText = ""
                                editDialog.open()
                            }
                        }

                        Button {
                            Layout.preferredWidth: 80
                            text: "Remove"
                            flat: true
                            Material.foreground: Material.color(Material.Red, Material.Shade600)
                            onClicked: {
                                removeDialog.targetUsername = modelData.username
                                removeDialog.open()
                            }
                        }
                    }

                    Rectangle {
                        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                        height: 1
                        color: Material.color(Material.BlueGrey, Material.Shade100)
                    }
                }
            }
        }

        // ── Snackbar ─────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: snackText.length > 0 ? 48 : 0
            visible: snackText.length > 0
            color: snackError
                   ? Material.color(Material.Red, Material.Shade700)
                   : Material.color(Material.Green, Material.Shade700)

            Behavior on height { NumberAnimation { duration: 150 } }

            Text {
                anchors.centerIn: parent
                text: snackText
                color: "white"
                font.pixelSize: 13
            }
        }
    }
}
