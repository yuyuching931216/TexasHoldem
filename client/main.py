import sys
from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt6.QtWidgets import QLabel, QPushButton, QTextEdit

class PokerGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("德州撲克")
        self.setGeometry(100, 100, 800, 600)
        self.setupUI()
    
    def setupUI(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        layout = QVBoxLayout()
        
        # 遊戲訊息顯示
        self.game_info = QTextEdit()
        self.game_info.setReadOnly(True)
        layout.addWidget(self.game_info)
        
        # 手牌顯示
        self.hand_label = QLabel("手牌: 等待發牌...")
        layout.addWidget(self.hand_label)
        
        # 動作按鈕
        self.fold_btn = QPushButton("棄牌")
        self.call_btn = QPushButton("跟注")
        self.raise_btn = QPushButton("加注")
        
        layout.addWidget(self.fold_btn)
        layout.addWidget(self.call_btn)
        layout.addWidget(self.raise_btn)
        
        central_widget.setLayout(layout)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = PokerGUI()
    window.show()
    sys.exit(app.exec())