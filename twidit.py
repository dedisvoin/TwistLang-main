from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QWidget, 
                            QStatusBar, QSplitter, QTextEdit, QHBoxLayout, 
                            QPushButton, QLineEdit, QLabel, QDockWidget,
                            QGridLayout, QSizePolicy, QScrollArea, QFrame,
                            QToolBar, QTabWidget, QTabBar, QMenuBar, QToolButton,
                            QFileDialog, QMessageBox)
from PyQt6.QtGui import (QColor, QFont, QAction, QKeySequence, QShortcut, 
                         QTextCursor, QKeyEvent, QPainter, QTextCharFormat,
                         QBrush, QPen, QPalette, QIcon, QPixmap)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer, QThread, QPoint, QSize
import sys
import os
import subprocess
import tempfile
import platform


class RunButton(QPushButton):
    """Круглая кнопка запуска кода"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_button()
        
    def setup_button(self):
        """Настройка кнопки"""
        self.setText("▶")
        self.setFixedSize(40, 40)
        
        self.setStyleSheet("""
            QPushButton {
                background-color: #a6e3a1;
                color: #1e1e2e;
                border: none;
                border-radius: 20px;
                font-size: 30px;
            }
            QPushButton:hover {
                background-color: #a0a1a0;
                border: 2px solid #89dceb;
            }
            QPushButton:pressed {
                background-color: #74c7ec;
            }
        """)


class TwistLangLexer(QsciLexerCustom):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # Стили
        self.STYLE_DEFAULT = 0
        self.STYLE_KEYWORD = 1
        self.STYLE_TYPE = 2
        self.STYLE_COMMENT = 3
        self.STYLE_STRING = 4
        self.STYLE_NUMBER = 5
        self.STYLE_OPERATOR = 6
        self.STYLE_FUNCTION = 7
        self.STYLE_MODIFIER = 8
        self.STYLE_DIRECTIVE = 9
        self.STYLE_LITERAL = 10
        
        self.font_size = 12
        self.setup_styles()
        
        # Ключевые слова
        self.keywords = {
            'if', 'else', 'for', 'while', 'let', 'in', 'and', 'or',
            'ret', 'auto', 'assert', 'lambda', 'break', 'continue',
            'out', 'outln', 'struct', 'namespace', 'func', 'return',
            'where', 'println', 'print', 'match', 'case', 'default',
            'import', 'export', 'as', 'from'
        }
        
        # Модификаторы переменных
        self.modifiers = {
            'const', 'static', 'global', 'final', 'private', 'public',
            'protected', 'volatile', 'mutable', 'transient', 'synchronized'
        }
        
        # Типы
        self.types = {
            'Int', 'Bool', 'String', 'Char', 'Null', 'Double',
            'Namespace', 'Func', 'Lambda'
        }
        
        # Литералы
        self.literals = {
            'true', 'false', 'null', 'none', 'self', 'super'
        }
        
        # Директивы
        self.directives = {
            '@define', '@macro', '@include', '@if', '@else',
            '@endif', '@debug', '@test'
        }
        
        # Встроенные функции
        self.functions = {
            'input', 'typeof', 'sizeof', 'exit', 'len', 'range',
            'map', 'filter', 'reduce', 'print', 'println'
        }

    def setup_styles(self):
        """Настройка стилей Catppuccin Mocha"""
        bg_color = QColor("#1e1e2e")
        fg_color = QColor("#cdd6f4")
        
        self.setDefaultPaper(bg_color)
        self.setDefaultColor(fg_color)
        
        safe_font = self.get_safe_font("Consolas", self.font_size)
        self.setFont(safe_font)
        
        colors = {
            self.STYLE_KEYWORD: QColor("#f5c2e7"),
            self.STYLE_TYPE: QColor("#cba6f7"),
            self.STYLE_COMMENT: QColor("#6c7086"),
            self.STYLE_STRING: QColor("#a6e3a1"),
            self.STYLE_NUMBER: QColor("#fab387"),
            self.STYLE_OPERATOR: QColor("#89b4fa"),
            self.STYLE_FUNCTION: QColor("#f38ba8"),
            self.STYLE_MODIFIER: QColor("#b4befe"),
            self.STYLE_DIRECTIVE: QColor("#f5e0dc"),
            self.STYLE_LITERAL: QColor("#f9e2af"),
        }
        
        for style, color in colors.items():
            self.setColor(color, style)
        
        italic_font = self.get_safe_font("Consolas", self.font_size)
        italic_font.setItalic(True)
        self.setFont(italic_font, self.STYLE_COMMENT)
        
        bold_font = self.get_safe_font("Consolas", self.font_size)
        bold_font.setBold(True)
        self.setFont(bold_font, self.STYLE_KEYWORD)
        self.setFont(bold_font, self.STYLE_MODIFIER)
        
    def get_safe_font(self, preferred_font, size):
        font = QFont(preferred_font, size)
        return font

    def change_font_size(self, delta):
        self.font_size = max(10, min(24, self.font_size + delta))
        self.setup_styles()
        return self.font_size

    def language(self):
        return "TwistLang"

    def description(self, style):
        descriptions = {
            self.STYLE_DEFAULT: "Default",
            self.STYLE_KEYWORD: "Keyword",
            self.STYLE_TYPE: "Type",
            self.STYLE_COMMENT: "Comment",
            self.STYLE_STRING: "String",
            self.STYLE_NUMBER: "Number",
            self.STYLE_OPERATOR: "Operator",
            self.STYLE_FUNCTION: "Function",
            self.STYLE_MODIFIER: "Modifier",
            self.STYLE_DIRECTIVE: "Directive",
            self.STYLE_LITERAL: "Literal",
        }
        return descriptions.get(style, "")

    def styleText(self, start, end):
        editor = self.editor()
        if not editor:
            return
        
        text = editor.text()
        if not text:
            return
        
        start = max(0, start)
        end = min(len(text), end)
        
        self.startStyling(start)
        text_to_style = text[start:end]
        
        i = 0
        while i < len(text_to_style):
            char = text_to_style[i]
            
            # Комментарии
            if char == '#':
                j = i
                while j < len(text_to_style) and text_to_style[j] != '\n':
                    j += 1
                self.setStyling(j - i, self.STYLE_COMMENT)
                i = j
                continue
            
            # Строки
            elif char in ('"', "'"):
                j = i + 1
                escaped = False
                while j < len(text_to_style):
                    if text_to_style[j] == '\\':
                        escaped = not escaped
                    elif text_to_style[j] == char and not escaped:
                        break
                    else:
                        escaped = False
                    j += 1
                
                if j < len(text_to_style):
                    j += 1
                
                self.setStyling(j - i, self.STYLE_STRING)
                i = j
                continue
            
            # Числа
            elif char.isdigit():
                j = i
                while j < len(text_to_style) and (text_to_style[j].isdigit() or 
                                                  text_to_style[j] == '.' or 
                                                  text_to_style[j] == 'e' or 
                                                  text_to_style[j] == 'E' or
                                                  text_to_style[j] == '-' or
                                                  text_to_style[j] == '+'):
                    j += 1
                self.setStyling(j - i, self.STYLE_NUMBER)
                i = j
                continue
            
            # Идентификаторы и ключевые слова
            elif char.isalpha() or char == '_' or char == '@':
                j = i
                while j < len(text_to_style) and (text_to_style[j].isalnum() or 
                                                  text_to_style[j] == '_' or 
                                                  text_to_style[j] == '@'):
                    j += 1
                
                word = text_to_style[i:j]
                style = self.STYLE_DEFAULT
                
                if word in self.keywords:
                    style = self.STYLE_KEYWORD
                elif word in self.modifiers:
                    style = self.STYLE_MODIFIER
                elif word in self.types:
                    style = self.STYLE_TYPE
                elif word in self.literals:
                    style = self.STYLE_LITERAL
                elif word in self.functions:
                    style = self.STYLE_FUNCTION
                elif word.startswith('@'):
                    style = self.STYLE_DIRECTIVE
                elif j < len(text_to_style) and text_to_style[j] == '(':
                    style = self.STYLE_FUNCTION
                
                self.setStyling(j - i, style)
                i = j
                continue
            
            # Операторы
            elif char in '+-*/%=&|^!<>~?.:;(){}[]':
                j = i
                operators = '+-*/%=&|^!<>~?.:;(){}[]->::'
                while j < len(text_to_style) and text_to_style[j] in operators:
                    j += 1
                self.setStyling(j - i, self.STYLE_OPERATOR)
                i = j
                continue
            
            else:
                self.setStyling(1, self.STYLE_DEFAULT)
                i += 1


class TwistLangEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.current_file = None
        self.setup_ui()
        self.setup_editor()
        self.create_menu()
        self.setup_shortcuts()
        
    def setup_ui(self):
        self.setGeometry(100, 100, 1200, 800)
        self.setWindowTitle("TwistLang Editor")
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        layout = QVBoxLayout(central_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        
        self.editor = QsciScintilla()
        layout.addWidget(self.editor)
        
        self.lexer = TwistLangLexer(self.editor)
        self.editor.setLexer(self.lexer)
        
        self.run_button = RunButton(self)
        self.run_button.clicked.connect(self.run_twist_code)
        
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        
        self.setStyleSheet("""
            QMainWindow {
                background-color: #1e1e2e;
            }
            QMenuBar {
                background-color: #181825;
                color: #cdd6f4;
                border-bottom: 1px solid #313244;
            }
            QMenuBar::item {
                padding: 5px 10px;
            }
            QMenuBar::item:selected {
                background-color: #585b70;
            }
            QMenu {
                background-color: #181825;
                color: #cdd6f4;
                border: 1px solid #313244;
            }
            QMenu::item {
                padding: 5px 25px;
            }
            QMenu::item:selected {
                background-color: #585b70;
            }
            QStatusBar {
                background-color: #11111b;
                color: #cdd6f4;
                border-top: 1px solid #313244;
            }
        """)
        
    def resizeEvent(self, event):
        super().resizeEvent(event)
        button_size = self.run_button.size()
        margin = 20
        x = self.width() - button_size.width() - margin
        y = self.height() - button_size.height() - margin - self.status_bar.height()
        self.run_button.move(x, y)
        
    def setup_editor(self):
        self.editor.setUtf8(True)
        self.editor.setPaper(QColor("#1e1e2e"))
        self.editor.setColor(QColor("#cdd6f4"))
        
        safe_font = self.get_safe_font("Consolas", 12)
        self.editor.setFont(safe_font)
        
        self.editor.setCaretForegroundColor(QColor("#ffffff"))
        self.editor.setCaretLineVisible(True)
        self.editor.setCaretLineBackgroundColor(QColor("#313244"))
        
        self.editor.setMarginsBackgroundColor(QColor("#181825"))
        self.editor.setMarginsForegroundColor(QColor("#6c7086"))
        self.editor.setMarginType(0, QsciScintilla.MarginType.NumberMargin)
        
        self.update_margin_width()
        
        self.editor.setBraceMatching(QsciScintilla.BraceMatch.SloppyBraceMatch)
        self.editor.setMatchedBraceBackgroundColor(QColor("#f5c2e755"))
        self.editor.setMatchedBraceForegroundColor(QColor("#f5c2e7"))
        
        self.editor.setIndentationsUseTabs(False)
        self.editor.setTabWidth(4)
        self.editor.setIndentationGuides(True)
        
        self.editor.setSelectionBackgroundColor(QColor("#585b70"))
        self.editor.setSelectionForegroundColor(QColor("#ffffff"))
        
        self.editor.textChanged.connect(self.update_margin_width)
        
    def get_safe_font(self, preferred_font, size):
        font = QFont(preferred_font, size)
        if not font.exactMatch():
            safe_fonts = ["Courier New", "DejaVu Sans Mono", "Monospace"]
            for font_name in safe_fonts:
                font = QFont(font_name, size)
                if font.exactMatch():
                    return font
        return font
        
    def update_margin_width(self):
        font_metrics = self.editor.fontMetrics()
        char_width = font_metrics.horizontalAdvance("0")
        margin_width = char_width * 4 + 10
        self.editor.setMarginWidth(0, f"{'0' * 4}")
        
    def create_menu(self):
        menubar = self.menuBar()
        
        # File
        file_menu = menubar.addMenu("File")
        
        new_action = QAction("New", self)
        new_action.setShortcut(QKeySequence("Ctrl+N"))
        new_action.triggered.connect(self.new_file)
        file_menu.addAction(new_action)
        
        open_action = QAction("Open", self)
        open_action.setShortcut(QKeySequence("Ctrl+O"))
        open_action.triggered.connect(self.open_file)
        file_menu.addAction(open_action)
        
        save_action = QAction("Save", self)
        save_action.setShortcut(QKeySequence("Ctrl+S"))
        save_action.triggered.connect(self.save_file)
        file_menu.addAction(save_action)
        
        save_as_action = QAction("Save As", self)
        save_as_action.setShortcut(QKeySequence("Ctrl+Shift+S"))
        save_as_action.triggered.connect(self.save_file_as)
        file_menu.addAction(save_as_action)
        
        file_menu.addSeparator()
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Edit
        edit_menu = menubar.addMenu("Edit")
        
        undo_action = QAction("Undo", self)
        undo_action.setShortcut(QKeySequence("Ctrl+Z"))
        undo_action.triggered.connect(self.editor.undo)
        edit_menu.addAction(undo_action)
        
        redo_action = QAction("Redo", self)
        redo_action.setShortcut(QKeySequence("Ctrl+Y"))
        redo_action.triggered.connect(self.editor.redo)
        edit_menu.addAction(redo_action)
        
        edit_menu.addSeparator()
        
        cut_action = QAction("Cut", self)
        cut_action.setShortcut(QKeySequence("Ctrl+X"))
        cut_action.triggered.connect(self.editor.cut)
        edit_menu.addAction(cut_action)
        
        copy_action = QAction("Copy", self)
        copy_action.setShortcut(QKeySequence("Ctrl+C"))
        copy_action.triggered.connect(self.editor.copy)
        edit_menu.addAction(copy_action)
        
        paste_action = QAction("Paste", self)
        paste_action.setShortcut(QKeySequence("Ctrl+V"))
        paste_action.triggered.connect(self.editor.paste)
        edit_menu.addAction(paste_action)
        
        # View
        view_menu = menubar.addMenu("View")
        
        zoom_in_action = QAction("Zoom In", self)
        zoom_in_action.setShortcut(QKeySequence("Ctrl+="))
        zoom_in_action.triggered.connect(self.zoom_in)
        view_menu.addAction(zoom_in_action)
        
        zoom_out_action = QAction("Zoom Out", self)
        zoom_out_action.setShortcut(QKeySequence("Ctrl+-"))
        zoom_out_action.triggered.connect(self.zoom_out)
        view_menu.addAction(zoom_out_action)
        
        # Run
        run_menu = menubar.addMenu("Run")
        
        run_action = QAction("Run Code", self)
        run_action.setShortcut(QKeySequence("F5"))
        run_action.triggered.connect(self.run_twist_code)
        run_menu.addAction(run_action)
        
    def setup_shortcuts(self):
        self.editor.wheelEvent = self.editor_wheel_event
        
    def editor_wheel_event(self, event):
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0:
                self.zoom_in()
            else:
                self.zoom_out()
            event.accept()
        else:
            QsciScintilla.wheelEvent(self.editor, event)
            
    def zoom_in(self):
        new_size = self.lexer.change_font_size(1)
        font = self.editor.font()
        font.setPointSize(new_size)
        self.editor.setFont(font)
        self.update_margin_width()
        self.status_bar.showMessage(f"Font size: {new_size}pt", 2000)
        
    def zoom_out(self):
        new_size = self.lexer.change_font_size(-1)
        font = self.editor.font()
        font.setPointSize(new_size)
        self.editor.setFont(font)
        self.update_margin_width()
        self.status_bar.showMessage(f"Font size: {new_size}pt", 2000)
        
    def run_twist_code(self):
        """Запускает код TwistLang в отдельном окне cmd"""
        if not self.current_file:
            reply = QMessageBox.question(
                self,
                "Save File",
                "File is not saved. Save before running?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            if reply == QMessageBox.StandardButton.Yes:
                self.save_file()
                if not self.current_file:
                    return
            else:
                return
        
        try:
            if platform.system() == "Windows":
                # Для Windows создаем батник и запускаем его
                bat_content = f'''@echo off
cd /d "{os.path.dirname(self.current_file)}"
echo Running {os.path.basename(self.current_file)}...
echo.
twistc --file "{self.current_file}"
echo.
echo Program finished.
echo Press any key to exit...
pause >nul
'''
                
                # Создаем временный батник
                with tempfile.NamedTemporaryFile(mode='w', suffix='.bat', delete=False) as f:
                    f.write(bat_content)
                    bat_file = f.name
                
                # Запускаем батник в новом окне cmd
                subprocess.Popen(['cmd', '/c', 'start', 'cmd', '/k', bat_file], 
                                shell=False)
                
                # Удаляем батник через 3 секунды (после запуска)
                QTimer.singleShot(3000, lambda: os.remove(bat_file) if os.path.exists(bat_file) else None)
                
                self.status_bar.showMessage(f"Running {os.path.basename(self.current_file)} in new cmd window...", 3000)
                
            else:
                # Для Linux/Mac
                QMessageBox.information(
                    self,
                    "Not Supported",
                    f"Running in terminal is currently only supported on Windows.\n\nFor Linux/Mac, run manually:\ntwistc --file {self.current_file}",
                    QMessageBox.StandardButton.Ok
                )
                
        except Exception as e:
            QMessageBox.critical(
                self,
                "Error",
                f"Failed to run command: {str(e)}",
                QMessageBox.StandardButton.Ok
            )
        
    def new_file(self):
        self.editor.clear()
        self.current_file = None
        self.status_bar.showMessage("New file created", 2000)
        
    def open_file(self):
        filename, _ = QFileDialog.getOpenFileName(
            self, "Open File", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            try:
                with open(filename, 'r', encoding='utf-8') as f:
                    self.editor.setText(f.read())
                self.current_file = filename
                self.status_bar.showMessage(f"Opened: {filename}", 2000)
            except Exception as e:
                self.status_bar.showMessage(f"Error: {str(e)}", 3000)
                
    def save_file(self):
        if self.current_file:
            self.save_to_file(self.current_file)
        else:
            self.save_file_as()
            
    def save_file_as(self):
        filename, _ = QFileDialog.getSaveFileName(
            self, "Save File", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            if not filename.endswith('.twist'):
                filename += '.twist'
            self.save_to_file(filename)
            self.current_file = filename
            
    def save_to_file(self, filename):
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(self.editor.text())
            self.status_bar.showMessage(f"Saved: {filename}", 2000)
        except Exception as e:
            self.status_bar.showMessage(f"Error: {str(e)}", 3000)


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    default_font = QFont("Segoe UI", 10)
    if not default_font.exactMatch():
        default_font = QFont("Arial", 10)
    app.setFont(default_font)
    
    palette = app.palette()
    palette.setColor(palette.ColorRole.Window, QColor("#1e1e2e"))
    palette.setColor(palette.ColorRole.WindowText, QColor("#cdd6f4"))
    palette.setColor(palette.ColorRole.Base, QColor("#181825"))
    palette.setColor(palette.ColorRole.AlternateBase, QColor("#11111b"))
    palette.setColor(palette.ColorRole.ToolTipBase, QColor("#313244"))
    palette.setColor(palette.ColorRole.ToolTipText, QColor("#cdd6f4"))
    palette.setColor(palette.ColorRole.Text, QColor("#cdd6f4"))
    palette.setColor(palette.ColorRole.Button, QColor("#313244"))
    palette.setColor(palette.ColorRole.ButtonText, QColor("#cdd6f4"))
    palette.setColor(palette.ColorRole.BrightText, QColor("#f5e0dc"))
    palette.setColor(palette.ColorRole.Link, QColor("#89b4fa"))
    palette.setColor(palette.ColorRole.Highlight, QColor("#585b70"))
    palette.setColor(palette.ColorRole.HighlightedText, QColor("#ffffff"))
    app.setPalette(palette)
    
    editor = TwistLangEditor()
    
    example_code = """# TwistLang Example
@define VERSION = "1.0.0"

const let PI = 3.14159
global let counter = 0
static let max_users = 100

struct Person {
    let name: String
    let age: Int
    
    func greet() -> Void {
        println("Hello, " + name + "!")
    }
}

func main() -> Int {
    # Create a person
    let person = Person {
        name: "Alice",
        age: 30
    }
    
    # Call method
    person.greet()
    
    # Loop example
    for i in range(1, 6) {
        if i % 2 == 0 {
            println(i, "is even")
        } else {
            println(i, "is odd")
        }
    }
    
    # Return success
    return 0
}

# Entry point
if __name__ == "__main__" {
    exit(main())
}
"""
    
    editor.editor.setText(example_code)
    editor.update_margin_width()
    
    editor.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()