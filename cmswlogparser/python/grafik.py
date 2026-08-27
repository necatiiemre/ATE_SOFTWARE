#author Buse Nur Pekmezci
import sys
import pandas as pd
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QPushButton,
    QFileDialog, QScrollArea, QHBoxLayout
)
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class PlotApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CSV Parametre Plotter")
        self.resize(900, 600)

        self.layout = QVBoxLayout(self)

        # CSV seçme butonu
        self.load_button = QPushButton("CSV Dosyası Seç")
        self.load_button.clicked.connect(self.load_csv)
        self.layout.addWidget(self.load_button)

        # Parametre butonlarının geleceği scroll alan
        self.scroll = QScrollArea()
        self.scroll_widget = QWidget()
        self.scroll_layout = QHBoxLayout(self.scroll_widget)
        self.scroll.setWidget(self.scroll_widget)
        self.scroll.setWidgetResizable(True)
        self.layout.addWidget(self.scroll)

        # Matplotlib alanı
        self.figure = Figure()
        self.canvas = FigureCanvas(self.figure)
        self.layout.addWidget(self.canvas)

        self.df = None

    def load_csv(self):
        file_name, _ = QFileDialog.getOpenFileName(
            self, "CSV Dosyası Seç", "", "CSV Files (*.csv)"
        )

        if file_name:
            self.df = pd.read_csv(file_name)
            self.create_param_buttons()

    def create_param_buttons(self):
        # Önce eski butonları temizle
        for i in reversed(range(self.scroll_layout.count())):
            self.scroll_layout.itemAt(i).widget().deleteLater()

        # Kolon isimlerine göre buton oluştur
        for col in self.df.columns:
            btn = QPushButton(col)
            btn.clicked.connect(lambda checked, c=col: self.plot_column(c))
            self.scroll_layout.addWidget(btn)

    def plot_column(self, column_name):
        self.figure.clear()
        ax = self.figure.add_subplot(111)

        ax.plot(self.df[column_name])
        ax.set_title(column_name)
        ax.set_xlabel("Index")
        ax.set_ylabel(column_name)

        self.canvas.draw()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = PlotApp()
    window.show()
    sys.exit(app.exec())

