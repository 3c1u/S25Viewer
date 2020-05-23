import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import QtQuick.Controls 1.4 as C
import QtQuick.Layouts 1.12

import S25Viewer 1.0

import Qt.labs.qmlmodels 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("S25 Viewer")

    LayerModel {
        id: layerModel
        image: theImage
    }

    C.SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            width: 200
            Layout.fillWidth: true
            color: "#000000"

            Image {
                id: theImage
            }
        }
        TableView {
            width: 200
            Layout.maximumWidth: 300

            model: layerModel
        }
    }
}
