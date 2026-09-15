import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Rectangle {
    id: playPanel
    height: column.implicitHeight + 24
    color: "#1a1a1a"
    Material.theme: Material.Dark
    Material.accent: "#00a7ff"

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 10

        Label {
            visible: Nax5Auth.authenticated && (!Nax5Auth.emailVerified || Nax5Auth.accessStatus !== "ACTIVE")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#ffcc80"
            text: !Nax5Auth.emailVerified ? qsTr("Подтвердите email") : qsTr("Аккаунт ещё не активен")
        }

        Label {
            visible: Nax5Session.statusText.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: Nax5Session.statusText
            color: Nax5Session.reserved ? "#b9f6ca" : "#eeeeee"
            font.pixelSize: 18
        }

        Label {
            visible: Nax5Session.reserved && Nax5Session.consoleCode.length > 0
            text: Nax5Session.consoleRegion.length > 0
                  ? qsTr("%1 · %2").arg(Nax5Session.consoleCode).arg(Nax5Session.consoleRegion)
                  : Nax5Session.consoleCode
            color: "#c5c5c5"
        }

        Label {
            visible: !Nax5Session.reserved && Nax5Session.errorMessage.length > 0 && Nax5Session.statusText !== Nax5Session.errorMessage
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#ff8a80"
            text: Nax5Session.errorMessage
        }

        RowLayout {
            spacing: 12

            Button {
                text: Nax5Session.reserving ? qsTr("Ищем...") : (Nax5Session.state === 3 || Nax5Session.state === 4 ? qsTr("Игра идёт") : qsTr("Играть"))
                enabled: Nax5Session.playEnabled
                Material.background: Material.accent
                Material.roundedScale: Material.SmallScale
                onClicked: Nax5Session.play()
            }

            Button {
                text: Nax5Session.cancelling ? qsTr("Освобождаем...") : qsTr("Освободить")
                visible: Nax5Session.reserved || Nax5Session.cancelling
                enabled: Nax5Session.releaseEnabled
                flat: true
                Material.roundedScale: Material.SmallScale
                onClicked: Nax5Session.release()
            }
        }
    }
}
