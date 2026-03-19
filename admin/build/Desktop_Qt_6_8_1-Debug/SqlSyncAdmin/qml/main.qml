import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 620
    minimumWidth: 780
    minimumHeight: 540
    title: "SQLSync Administrator"

    Material.theme: Material.Light
    Material.accent: Material.Blue
    Material.primary: Material.BlueGrey

    // Track which wizard step is active (0-based)
    property int currentStep: 0
    readonly property var stepTitles: [
        "Connection",
        "Configure",
        "Install",
        "Summary",
        "Users"
    ]

    // Advance to next page (called from each page's "Next" button)
    function nextPage() {
        if (currentStep < stepTitles.length - 1) {
            currentStep++
            stack.replace(pages[currentStep])
        }
    }

    function goToPage(index) {
        if (index >= 0 && index < stepTitles.length) {
            currentStep = index
            stack.replace(pages[index])
        }
    }

    readonly property var pages: [
        "pages/ConnectionPage.qml",
        "pages/ConfigurePage.qml",
        "pages/InstallPage.qml",
        "pages/SummaryPage.qml",
        "pages/UsersPage.qml"
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        WizardSidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            currentStep: root.currentStep
            stepTitles: root.stepTitles
            onStepClicked: function(index) {
                // Only allow navigating to pages already visited (or Users page
                // once setup is done)
                if (index <= root.currentStep ||
                    (index === 4 && Admin.setupDone)) {
                    root.goToPage(index)
                }
            }
        }

        // Divider
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Material.color(Material.BlueGrey, Material.Shade200)
        }

        // Main content
        StackView {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true

            initialItem: pages[0]
        }
    }

    // Listen for setup completion to auto-advance to Summary
    Connections {
        target: Admin
        function onSetupFinished(success, message) {
            if (success) {
                root.goToPage(3)  // SummaryPage
            }
        }
    }
}
