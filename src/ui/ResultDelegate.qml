import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    width: ListView.view.width
    height: contentColumn.implicitHeight + 20
    color: index % 2 === 0 ? (window.currentTheme === "light" ? "#fafafa" : "#34495e") 
                            : (window.currentTheme === "light" ? "#f5f5f5" : "#2c3e50")
    
    Column {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        anchors.top: parent.top
        anchors.topMargin: 10
        spacing: 6
        
        // Мета-информация: Автор / Жанр / Название / Контекст
        Row {
            spacing: 6
            
            // Автор (кликабельный фильтр)
            Label {
                text: modelData.author || "Неизвестен"
                font.pixelSize: window.fontSize - 2
                font.family: window.fontFamily
                color: window.currentMeta()
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // TODO: toggle author filter
                        console.log("Filter author:", parent.text)
                    }
                }
            }
            
            Label { text: "/"; color: window.currentMeta() }
            
            // Жанр (кликабельный фильтр)
            Label {
                text: modelData.genre || "Без жанра"
                font.pixelSize: window.fontSize - 2
                font.family: window.fontFamily
                color: window.currentMeta()
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // TODO: toggle genre filter
                        console.log("Filter genre:", parent.text)
                    }
                }
            }
            
            Label { text: "/"; color: window.currentMeta() }
            
            // Название книги (кликабельный фильтр)
            Label {
                text: modelData.title || "Без названия"
                font.pixelSize: window.fontSize - 2
                font.family: window.fontFamily
                font.bold: true
                color: window.currentMeta()
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // TODO: toggle title filter
                        console.log("Filter title:", parent.text)
                    }
                }
            }
            
            Item { Layout.fillWidth: true }
            
            // Контекст (ссылка на книгу)
            Label {
                text: "📖 контекст"
                font.pixelSize: window.fontSize - 2
                font.family: window.fontFamily
                color: "#3498db"
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // TODO: открыть книгу с прокруткой к чанку
                        console.log("Open book context:", modelData.chunkNum)
                    }
                }
            }
        }
        
        // Контент чанка с подсветкой
        Rectangle {
            width: parent.width
            color: "transparent"
            
            Text {
                width: parent.width
                text: window.showFormatting ? (modelData.content || "") 
                                            : (modelData.content || "").replace(/\n/g, " ")
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                font.pixelSize: window.fontSize
                font.family: window.fontFamily
                color: window.currentText()
                
                // Разрешаем выделение и копирование
                selectByMouse: true
                selectionColor: "#4a90d9"
                selectedTextColor: "white"
            }
        }
    }
}