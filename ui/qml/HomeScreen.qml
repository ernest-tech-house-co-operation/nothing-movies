import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: homeScreen
    color: "#0d0d0d"

    // Placeholder data -- swap `model:` below for a real C++ model
    // (search_aggregator::searchAll results exposed via a QAbstractListModel)
    // once the backend is wired into QML.
    ListModel {
        id: placeholderMovies
        ListElement { title: "Movie One";   year: "2023" }
        ListElement { title: "Movie Two";   year: "2024" }
        ListElement { title: "Movie Three"; year: "2022" }
        ListElement { title: "Movie Four";  year: "2021" }
        ListElement { title: "Movie Five";  year: "2024" }
        ListElement { title: "Movie Six";   year: "2020" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Text {
            text: "Home"
            color: "white"
            font.pixelSize: 28
            font.bold: true
        }

        GridView {
            id: movieGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: 160
            cellHeight: 240
            clip: true

            model: placeholderMovies

            delegate: MovieCard {
                width: movieGrid.cellWidth - 12
                height: movieGrid.cellHeight - 12
                movieTitle: model.title
                movieYear: model.year
            }
        }
    }
}