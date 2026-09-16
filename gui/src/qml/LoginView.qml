import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Item {
    id: loginRoot
    Material.theme: Material.Dark
    Material.accent: "#00a7ff"

    function submit() {
        if (Nax5Auth.authenticating)
            return
        Nax5Auth.login(emailField.text.trim(), passwordField.text)
        passwordField.text = ""
    }

    Keys.onEscapePressed: root.showConfirmDialog(qsTr("Quit"), qsTr("Are you sure you want to quit?"), () => Qt.quit())

    Pane {
        anchors.fill: parent

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 80, 460)
            spacing: 18

            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 220
                Layout.preferredHeight: 52
                fillMode: Image.PreserveAspectFit
                source: "qrc:/icons/nax5-logo.svg"
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Войдите в аккаунт")
                color: "#c5c5c5"
            }

            Label {
                text: qsTr("Email")
            }

            TextField {
                id: emailField
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhEmailCharactersOnly
                Keys.onReturnPressed: passwordField.forceActiveFocus()
                enabled: !Nax5Auth.authenticating
            }

            Label {
                text: qsTr("Пароль")
            }

            TextField {
                id: passwordField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                enabled: !Nax5Auth.authenticating
                Keys.onReturnPressed: loginRoot.submit()
            }

            Label {
                id: errorLabel
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#ff8a80"
                visible: text.length > 0
                text: Nax5Auth.errorMessage
            }

            Button {
                id: loginButton
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                text: Nax5Auth.authenticating ? qsTr("Вход...") : qsTr("Войти")
                enabled: !Nax5Auth.authenticating && emailField.text.trim().length > 0 && passwordField.text.length > 0
                Material.background: Material.accent
                Material.roundedScale: Material.SmallScale
                onClicked: loginRoot.submit()
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Нет аккаунта?")
                color: "#c5c5c5"
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "cloudgta6.com/register"
                color: Material.accent
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally("https://cloudgta6.com/register/")
                }
            }
        }
    }

    Component.onCompleted: emailField.forceActiveFocus()
}
