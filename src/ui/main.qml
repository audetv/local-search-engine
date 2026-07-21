import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 960
    height: 720
    visible: true
    title: "Local Search Engine"

    // Свойства приложения
    property string currentTheme: "light"
    property int fontSize: 16
    property string fontFamily: "Arial"
    property bool showFormatting: true
    property string searchMode: "and"  // and, or, phrase
    
    // Поисковый запрос
    property string searchQuery: ""
    
    // Модель результатов
    property var searchResults: []
    
    // Цвета темы
    readonly property color lightBg: "#ffffff"
    readonly property color lightText: "#2c3e50"
    readonly property color lightMeta: "#7f8c8d"
    readonly property color lightHighlight: "#e74c3c"
    readonly property color darkBg: "#2c3e50"
    readonly property color darkText: "#ecf0f1"
    readonly property color darkMeta: "#95a5a6"
    readonly property color darkHighlight: "#e74c3c"
    
    function currentBg() { return currentTheme === "light" ? lightBg : darkBg }
    function currentText() { return currentTheme === "light" ? lightText : darkText }
    function currentMeta() { return currentTheme === "light" ? lightMeta : darkMeta }
    function currentHighlight() { return currentTheme === "light" ? lightHighlight : darkHighlight }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Панель поиска
        SearchBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
        }
        
        // Панель инструментов
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: currentTheme === "light" ? "#f0f0f0" : "#3d566e"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8
                
                // Режим поиска
                ComboBox {
                    id: modeCombo
                    model: ["AND", "OR", "Фраза"]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        if (currentIndex === 0) searchMode = "and"
                        else if (currentIndex === 1) searchMode = "or"
                        else searchMode = "phrase"
                    }
                }
                
                // Размер шрифта
                Label { text: "Aa" }
                Slider {
                    from: 12; to: 32
                    value: fontSize
                    onValueChanged: fontSize = value
                }
                
                // Шрифт
                ComboBox {
                    id: fontCombo
                    model: ["Arial", "Times New Roman", "Georgia", "Verdana"]
                    onCurrentTextChanged: fontFamily = currentText
                }
                
                // Тема
                Button {
                    text: currentTheme === "light" ? "🌙" : "☀️"
                    onClicked: currentTheme = currentTheme === "light" ? "dark" : "light"
                }
                
                // Скрыть форматирование
                Button {
                    text: showFormatting ? "¶" : "¶̶"
                    checkable: true
                    onClicked: showFormatting = !showFormatting
                }
                
                Item { Layout.fillWidth: true }
                
                // Настройки
                Button {
                    text: "⚙"
                    onClicked: settingsDialog.open()
                }
                
                // Индексация
                Button {
                    text: "+"
                    onClicked: indexerDialog.open()
                }
            }
        }
        
        // Список результатов
        ListView {
            id: resultsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            model: searchResults
            delegate: ResultDelegate {}
            
            ScrollBar.vertical: ScrollBar {}
        }
        
        // Статусная строка
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: currentTheme === "light" ? "#e8e8e8" : "#34495e"
            
            Label {
                anchors.centerIn: parent
                text: searchResults.length + " результатов"
                color: currentMeta()
            }
        }
    }
    
    // Функция поиска (будет подключена к C++)
    function performSearch(query) {
        searchQuery = query
        // Будет вызывать C++ контроллер
        console.log("Search:", query, "Mode:", searchMode)
    }
}