import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Rectangle {
    property int hostIndex: -1
    visible: Chiaki.operatorMode
    height: visible ? column.implicitHeight + 20 : 0
    color: "#221a1a"
    Material.theme: Material.Dark

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        Label {
            text: qsTr("Operator Mode")
            color: "#ffcc80"
            font.bold: true
        }

        Label {
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            text: qsTr("1) Register via original Chiaki flow. 2) Select that PS5 in the list. 3) Enter backend console code. 4) Provision selected host. 5) Test. 6) Activate READY only after a successful test.")
            color: "#bdbdbd"
        }

        Label {
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            text: hostIndex < 0
                  ? qsTr("No console selected. Highlight a registered PS5 below.")
                  : qsTr("Provisioning uses the highlighted console (index %1). Backend code is separate and is not matched by nickname.").arg(hostIndex)
            color: "#ffcc80"
        }

        TextField {
            id: consoleCode
            Layout.fillWidth: true
            placeholderText: qsTr("Backend console code, e.g. PS5-439")
            text: Chiaki.nax5LastOperatorConsoleCode
            onEditingFinished: Chiaki.nax5LastOperatorConsoleCode = text.trim()
        }

        RowLayout {
            Button {
                text: qsTr("Provision selected host")
                onClicked: Chiaki.nax5ProvisionHost(hostIndex, consoleCode.text.trim())
            }
            Button {
                text: qsTr("Test provisioned")
                onClicked: Chiaki.nax5OperatorTest(consoleCode.text.trim())
            }
            Button {
                text: qsTr("Activate READY")
                onClicked: Chiaki.nax5ActivateConsole(consoleCode.text.trim())
            }
        }
    }
}
