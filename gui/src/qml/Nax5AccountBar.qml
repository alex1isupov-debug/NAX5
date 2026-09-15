import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

ToolBar {
    id: accountBar
    height: 52
    Material.theme: Material.Dark
    Material.accent: "#00a7ff"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        Label {
            text: "NAX5"
            font.bold: true
        }

        Label {
            text: Nax5Auth.email
            color: "#dddddd"
        }

        Label {
            text: qsTr("Статус: %1").arg(Nax5Auth.accessStatus)
            color: "#bbbbbb"
        }

        Item { Layout.fillWidth: true }

        Button {
            text: qsTr("Сохранить отчёт")
            flat: true
            Material.roundedScale: Material.SmallScale
            onClicked: Nax5Session.saveReport()
        }

        Button {
            text: qsTr("Настройки")
            flat: true
            Material.roundedScale: Material.SmallScale
            onClicked: root.showSettingsDialog()
        }

        Button {
            text: qsTr("Выйти")
            flat: true
            Material.roundedScale: Material.SmallScale
            onClicked: Nax5Session.releaseAndLogout()
        }
    }
}
