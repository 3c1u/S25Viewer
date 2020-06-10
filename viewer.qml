import QtQuick 2.12
import QtQuick.Controls 2.12

import QtQuick.Controls 1.4 as C
import QtQuick.Layouts 1.12

import S25Viewer 1.0

import Qt.labs.qmlmodels 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 640
    height: 480
    title: qsTr("S25 Viewer")

    LayerModel {
        id: layerModel
        image: image
    }

    C.SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            width: 200
            Layout.fillWidth: true
            color: Qt.rgba(0, 0, 0, 0)

            Image {
                id: image
            }

            DropArea {
                anchors.fill: parent
                onDropped: {
                    if (drop.hasUrls) {
                        image.url = drop.urls[0];
                    }
                }
            }
        }

        C.TableView {
            width: 200
            Layout.maximumWidth: 400
            model: layerModel

            C.TableViewColumn {
                role: "layerNo"
                title: qsTr("Layer")
                width: 100
            }
            C.TableViewColumn {
                role: "pictLayer"
                title: qsTr("Pict Layer")
                width: 200
            }

            itemDelegate: TextInput {
                text: styleData.value
                onTextEdited: {
                    layerModel.setData(layerModel.index(styleData.row, styleData.column),
                                       text);
                }
            }
        }
    }
}
