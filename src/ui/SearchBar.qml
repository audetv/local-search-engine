import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: window.currentTheme === "light" ? "#f8f9fa" : "#2c3e50"
    border.color: window.currentTheme === "light" ? "#dee2e6" : "#4a6278"
    border.width: 1
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Введите поисковый запрос..."
            font.pixelSize: 16
            color: window.currentText()
            background: Rectangle {
                color: window.currentBg()
                border.color: window.currentTheme === "light" ? "#ced4da" : "#5a7d9a"
                border.width: 1
                radius: 4
            }
            
            onAccepted: searchButton.clicked()
        }
        
        Button {
            id: searchButton
            text: "Найти"
            onClicked: {
                if (searchField.text.length > 0) {
                    window.performSearch(searchField.text)
                }
            }
        }
    }
}