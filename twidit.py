import sys
import os
import subprocess
import tempfile
import platform
import shutil
from pathlib import Path

from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QWidget,
    QStatusBar, QSplitter, QTextEdit, QHBoxLayout,
    QPushButton, QLineEdit, QLabel, QDockWidget,
    QGridLayout, QSizePolicy, QScrollArea, QFrame,
    QToolBar, QTabWidget, QTabBar, QMenuBar, QToolButton,
    QFileDialog, QMessageBox, QInputDialog
)
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QTextCursor, QKeyEvent, QPainter, QTextCharFormat,
    QBrush, QPen, QPalette, QIcon, QPixmap, QMouseEvent,
    QDragEnterEvent, QDropEvent
)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer, QThread, QPoint, QSize, QUrl


# ----------------------------------------------------------------------
# Кнопка запуска (круглая)
# ----------------------------------------------------------------------
class RunButton(QPushButton):
    def __init__(self, parent=None):
        super().__init__(parent)
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


# ----------------------------------------------------------------------
# Кастомный редактор с автозакрытием скобок и Ctrl+Click для @include
# ----------------------------------------------------------------------
class CustomScintilla(QsciScintilla):
    gotoDefinitionRequested = pyqtSignal(str, int)  # путь, строка

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMouseTracking(True)
        self.ctrlPressed = False
        self.main_window = None  # будет установлено при создании

    def set_main_window(self, window):
        self.main_window = window

    def keyPressEvent(self, event):
        # Автозакрытие парных символов

        if event.key() == Qt.Key.Key_ParenLeft:          # (
            self.insert_pair('(', ')')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_BracketLeft:      # [
            self.insert_pair('[', ']')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_BraceLeft:        # {
            self.insert_pair('{', '}')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_QuoteDbl:         # "
            self.insert_pair('"', '"')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_Apostrophe:       # '
            self.insert_pair("'", "'")
            event.accept()
            return
        elif event.key() == Qt.Key.Key_Control:
            self.ctrlPressed = True
            super().keyPressEvent(event)
        else:
            super().keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.ctrlPressed = False
        super().keyReleaseEvent(event)

    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.ctrlPressed:
            pos = event.pos()
            # Получаем позицию в тексте по координатам мыши
            position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
            line, col = self.lineIndexFromPosition(position)
            if line >= 0 and col >= 0:
                if self.isIncludeLine(line):
                    path = self.extractIncludePath(line)
                    if path:
                        self.gotoDefinitionRequested.emit(path, line)
                        event.accept()
                        return
        super().mousePressEvent(event)

    def wheelEvent(self, event):
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0:
                if self.main_window:
                    self.main_window.zoom_in()
            else:
                if self.main_window:
                    self.main_window.zoom_out()
            event.accept()
        else:
            super().wheelEvent(event)

    def insert_pair(self, open_char, close_char):
        line, col = self.getCursorPosition()
        self.insertAt(open_char + close_char, line, col)
        self.setCursorPosition(line, col + 1)

    def isIncludeLine(self, line):
        text = self.text(line).strip()
        return text.startswith('@include')

    def extractIncludePath(self, line):
        text = self.text(line).strip()
        if not text.startswith('@include'):
            return None
        # Поиск пути в кавычках или угловых скобках
        start_quote = text.find('"')
        if start_quote != -1:
            end_quote = text.find('"', start_quote + 1)
            if end_quote != -1:
                return text[start_quote+1:end_quote]
        start_angle = text.find('<')
        if start_angle != -1:
            end_angle = text.find('>', start_angle + 1)
            if end_angle != -1:
                return text[start_angle+1:end_angle]
        return None


# ----------------------------------------------------------------------
# Лексер TwistLang (Catppuccin Mocha)
# ----------------------------------------------------------------------
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
        self.STYLE_NAMESPACE_ID = 11   # для имён пространств имён (в объявлениях и при использовании)
        self.STYLE_SPECIAL = 12
        self.STYLE_OBJECT = 13          # для object.attr

        self.font_size = 12
        self.setup_styles()

        # Ключевые слова
        self.keywords = {
            'if', 'else', 'for', 'while', 'let', 'in', 'and', 'or',
            'ret', 'assert', 'lambda',
            'struct', 'namespace', 'func', 'continue;', 'break;'
        }

        # Модификаторы переменных
        self.modifiers = {
            'const', 'static', 'global', 'final', 'private'
        }

        # Типы
        self.types = {
            'Int', 'Bool', 'String', 'Char', 'Null', 'Double',
            'Namespace', 'Func', 'Lambda', 'auto', "Type"
        }

        # Литералы
        self.literals = {
            'true', 'false', 'null', 'self'
        }

        # Директивы
        self.directives = {
            '#define', '#macro', '#include'
        }

        # Специальные ключевые слова
        self.special_keywords = {'new', 'del', 'typeof', 'sizeof', 'out', 'outln', 'input'}

    def wordCharacters(self):
        # Возвращаем символы, которые считаются частью слова (включая '#')
        return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"

    def setup_styles(self):
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
            self.STYLE_OPERATOR: QColor("#6c7086"),
            self.STYLE_FUNCTION: QColor("#89b4fa"),
            self.STYLE_MODIFIER: QColor("#b4befe"),
            self.STYLE_DIRECTIVE: QColor("#f5e0dc"),
            self.STYLE_LITERAL: QColor("#f9e2af"),
            self.STYLE_NAMESPACE_ID: QColor("#94e2d5"),
            self.STYLE_SPECIAL: QColor("#f2b5b5"),
            self.STYLE_OBJECT: QColor("#f2cdcd")      # светло‑розовый для object.attr
        }

        for style, color in colors.items():
            self.setColor(color, style)

        # Шрифт для комментариев (курсив)
        italic_font = self.get_safe_font("Consolas", self.font_size)
        italic_font.setItalic(True)
        self.setFont(italic_font, self.STYLE_COMMENT)

        # Жирный шрифт для ключевых слов
        bold_font = self.get_safe_font("Consolas", self.font_size)
        bold_font.setBold(True)
        bold_font.setItalic(True)
        self.setFont(bold_font, self.STYLE_KEYWORD)

        # Подчёркнутый для модификаторов
        underline_font = self.get_safe_font("Consolas", self.font_size)
        underline_font.setUnderline(True)
        self.setFont(underline_font, self.STYLE_MODIFIER)

        # Курсив для специальных
        specials_font = self.get_safe_font("Consolas", self.font_size)
        specials_font.setItalic(True)
        self.setFont(specials_font, self.STYLE_SPECIAL)

    def get_safe_font(self, preferred_font, size):
        font = QFont(preferred_font, size)
        return font

    def change_font_size(self, delta):
        self.font_size = max(7, min(40, self.font_size + delta))
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
            self.STYLE_NAMESPACE_ID: "Namespace",
            self.STYLE_SPECIAL: "Special",
            self.STYLE_OBJECT: "Object"
        }
        return descriptions.get(style, "")

    def styleText(self, start, end):
        editor = self.editor()
        if not editor:
            return

        text = editor.text()
        if not text:
            return

        text_bytes = text.encode('utf-8')
        total_bytes = len(text_bytes)

        start = max(0, start)
        end = min(total_bytes, end)

        if start >= end:
            return

        self.startStyling(start)

        segment = text_bytes[start:end]
        seg_len = len(segment)
        pos = 0

        expecting_namespace = False

        while pos < seg_len:
            b = segment[pos]

            # Определяем длину символа UTF-8
            if b & 0x80 == 0:
                char_len = 1
            elif b & 0xE0 == 0xC0:
                char_len = 2
            elif b & 0xF0 == 0xE0:
                char_len = 3
            elif b & 0xF8 == 0xF0:
                char_len = 4
            else:
                char_len = 1

            try:
                ch = segment[pos:pos+char_len].decode('utf-8')
                next_ch = segment[pos+char_len:pos+char_len+1].decode('utf-8')
            except:
                ch = '\ufffd'

            # --- Комментарий ---
            if ch == '/' and next_ch == '/':
                expecting_namespace = False
                j = pos
                while j < seg_len and segment[j:j+1] != b'\n':
                    bj = segment[j]
                    if bj & 0x80 == 0:
                        j += 1
                    elif bj & 0xE0 == 0xC0:
                        j += 2
                    elif bj & 0xF0 == 0xE0:
                        j += 3
                    elif bj & 0xF8 == 0xF0:
                        j += 4
                    else:
                        j += 1
                self.setStyling(j - pos, self.STYLE_COMMENT)
                pos = j
                continue

            # --- Строковые литералы ---
            if ch in ('"', "'"):
                expecting_namespace = False
                quote = ch
                j = pos + char_len
                escaped = False
                while j < seg_len:
                    bj = segment[j]
                    if bj & 0x80 == 0:
                        cur_len = 1
                    elif bj & 0xE0 == 0xC0:
                        cur_len = 2
                    elif bj & 0xF0 == 0xE0:
                        cur_len = 3
                    elif bj & 0xF8 == 0xF0:
                        cur_len = 4
                    else:
                        cur_len = 1
                    try:
                        cur_ch = segment[j:j+cur_len].decode('utf-8')
                    except:
                        cur_ch = '\ufffd'

                    if cur_ch == '\\':
                        escaped = not escaped
                    elif cur_ch == quote and not escaped:
                        j += cur_len
                        break
                    else:
                        escaped = False
                    j += cur_len
                else:
                    j = seg_len
                self.setStyling(j - pos, self.STYLE_STRING)
                pos = j
                continue

            # --- Числа ---
            if ch.isdigit() or (ch == '.' and pos + char_len < seg_len and segment[pos+char_len:pos+char_len+1].isdigit()):
                expecting_namespace = False
                j = pos
                if ch.isdigit():
                    j += char_len
                    while j < seg_len:
                        bj = segment[j]
                        if bj & 0x80 != 0:
                            break
                        next_ch = segment[j:j+1].decode('ascii')
                        if not (next_ch.isdigit() or next_ch == '.'):
                            break
                        j += 1
                else:  # случай ".цифра"
                    j = pos + char_len
                    while j < seg_len:
                        bj = segment[j]
                        if bj & 0x80 != 0:
                            break
                        next_ch = segment[j:j+1].decode('ascii')
                        if not next_ch.isdigit():
                            break
                        j += 1
                self.setStyling(j - pos, self.STYLE_NUMBER)
                pos = j
                continue

            # --- Идентификаторы (буквы, _) ---
            if ch.isalpha() or ch == '_' or ch == '#':
                j = pos + char_len
                while j < seg_len:
                    bj = segment[j]
                    if bj & 0x80 == 0:
                        cur_len = 1
                        cur_ch = segment[j:j+1].decode('ascii')
                    else:
                        if bj & 0xE0 == 0xC0:
                            cur_len = 2
                        elif bj & 0xF0 == 0xE0:
                            cur_len = 3
                        elif bj & 0xF8 == 0xF0:
                            cur_len = 4
                        else:
                            cur_len = 1
                        try:
                            cur_ch = segment[j:j+cur_len].decode('utf-8')
                        except:
                            cur_ch = '\ufffd'

                    if not (cur_ch.isalnum() or cur_ch == '_' or cur_ch == '@'):
                        break
                    j += cur_len

                word = segment[pos:j].decode('utf-8', errors='replace')
                style = self.STYLE_DEFAULT

                if word == "namespace":
                    expecting_namespace = True
                    style = self.STYLE_KEYWORD
                elif word == "struct":
                    expecting_namespace = True
                    style = self.STYLE_KEYWORD
                elif expecting_namespace:
                    style = self.STYLE_NAMESPACE_ID
                    expecting_namespace = False
                else:
                    if word in self.keywords:
                        style = self.STYLE_KEYWORD
                    elif word in {'continue', 'break'}:
                        style = self.STYLE_KEYWORD
                    elif word in self.special_keywords:
                        style = self.STYLE_SPECIAL
                    elif word in self.modifiers:
                        style = self.STYLE_MODIFIER
                    elif word in self.types:
                        style = self.STYLE_TYPE
                    elif word in self.literals:
                        style = self.STYLE_LITERAL
                    elif word.startswith('#'):
                        style = self.STYLE_DIRECTIVE
                    # Если после слова идут два двоеточия, это имя пространства имён
                    elif j + 2 <= seg_len and segment[j:j+2] == b'::':
                        style = self.STYLE_NAMESPACE_ID
                    # Если следующий символ — открывающая скобка, это функция
                    elif j < seg_len and segment[j:j+1] == b'(':
                        style = self.STYLE_FUNCTION
                    # Если следующий символ — точка, это имя объекта
                    elif j < seg_len and segment[j:j+1] == b'.':
                        style = self.STYLE_OBJECT
                    else:
                        style = self.STYLE_DEFAULT

                self.setStyling(j - pos, style)
                pos = j
                continue

            # --- Операторы ---
            if ch in '+-*/%=&|^!<>~?.:;(){}[]':
                expecting_namespace = False
                j = pos + char_len
                operators = '+-*/%=&|^!<>~?.:;(){}[]->::'
                while j < seg_len:
                    bj = segment[j]
                    if bj & 0x80 != 0:
                        break
                    next_ch = segment[j:j+1].decode('ascii')
                    if next_ch not in operators:
                        break
                    j += 1
                self.setStyling(j - pos, self.STYLE_OPERATOR)
                pos = j
                continue

            # --- Всё остальное ---
            self.setStyling(char_len, self.STYLE_DEFAULT)
            pos += char_len

# ----------------------------------------------------------------------
# Виджет вкладок с редакторами
# ----------------------------------------------------------------------
class EditorTabWidget(QTabWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTabsClosable(False)  # отключаем стандартные кнопки
        self.setMovable(True)

    def addTab(self, widget, title):
        index = super().addTab(widget, title)
        self._add_close_button(index)
        return index

    def insertTab(self, index, widget, title):
        index = super().insertTab(index, widget, title)
        self._add_close_button(index)
        return index

    def _add_close_button(self, index):
        btn = QToolButton(self.tabBar())
        btn.setIcon(self._create_close_icon())
        btn.setIconSize(QSize(16, 16))
        btn.setStyleSheet("""
            QToolButton {
                background-color: transparent;
                border: none;
                padding: 1px;
            }
            QToolButton:hover {
                background-color: rgba(255, 255, 255, 30);
                border-radius: 2px;
            }
            QToolButton:pressed {
                background-color: rgba(255, 255, 255, 60);
            }
        """)
        btn.clicked.connect(lambda checked, idx=index: self._on_close_clicked(idx))
        self.tabBar().setTabButton(index, QTabBar.ButtonPosition.RightSide, btn)

    def _create_close_icon(self):
        pixmap = QPixmap(16, 16)
        pixmap.fill(Qt.GlobalColor.transparent)
        painter = QPainter(pixmap)
        painter.setPen(QPen(QColor("#cdd6f4"), 2))  # цвет крестика
        painter.drawLine(4, 4, 12, 12)
        painter.drawLine(12, 4, 4, 12)
        painter.end()
        return QIcon(pixmap)

    def _on_close_clicked(self, index):
        self.close_tab(index)

    def close_tab(self, index):
        widget = self.widget(index)
        if isinstance(widget, CustomScintilla):
            editor = widget
            if editor.isModified():
                filename = getattr(editor, 'filename', None)
                name = os.path.basename(filename) if filename else "Untitled"
                reply = QMessageBox.question(
                    self,
                    "Save Changes",
                    f"Save changes to {name}?",
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No | QMessageBox.StandardButton.Cancel
                )
                if reply == QMessageBox.StandardButton.Cancel:
                    return
                elif reply == QMessageBox.StandardButton.Yes:
                    main_window = self.window()
                    if hasattr(main_window, 'save_editor_to_file'):
                        if filename:
                            main_window.save_editor_to_file(editor, filename)
                        else:
                            main_window.save_editor_as(editor)
        self.removeTab(index)
        widget.deleteLater()


# ----------------------------------------------------------------------
# Главное окно редактора
# ----------------------------------------------------------------------
class TwistLangEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setGeometry(100, 100, 1200, 800)
        self.setWindowTitle("TwistLang Editor")

        self.tab_widget = EditorTabWidget(self)
        self.setCentralWidget(self.tab_widget)

        self.run_button = RunButton(self)
        self.run_button.clicked.connect(self.run_current_file)

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)

        self.create_menu()
        self.setup_shortcuts()
        self.apply_stylesheet()

        # Включаем приём файлов через Drag & Drop
        self.setAcceptDrops(True)

        # Создаём пустую вкладку при старте
        self.new_file()

    def apply_stylesheet(self):
        self.setStyleSheet("""
            QMainWindow {
                background-color: #1e1e2e;
            }
            QTabWidget::pane {
                border: 1px solid #313244;
                background-color: #1e1e2e;
            }
            QTabBar::tab {
                background-color: #181825;
                color: #cdd6f4;
                padding: 6px 10px;
                margin-right: 2px;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }
            QTabBar::tab:selected {
                background-color: #313244;
            }
            QTabBar::tab:hover {
                background-color: #45475a;
            }
            QTabBar::close-button {
                width: 20px;
                height: 20px;
                subcontrol-position: right;
            }
            QTabBar::close-button:hover {
                background-color: rgba(255, 255, 255, 30);
            }
            QTabBar::close-button:pressed {
                background-color: rgba(255, 255, 255, 60);
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

    # --- Drag & Drop ---
    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent):
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if os.path.isfile(file_path):
                self.open_file(file_path)
        event.acceptProposedAction()

    def create_menu(self):
        menubar = self.menuBar()

        # File
        file_menu = menubar.addMenu("File")
        new_action = QAction("New", self)
        new_action.setShortcut(QKeySequence("Ctrl+N"))
        new_action.triggered.connect(self.new_file)
        file_menu.addAction(new_action)

        open_action = QAction("Open...", self)
        open_action.setShortcut(QKeySequence("Ctrl+O"))
        open_action.triggered.connect(self.open_file_dialog)
        file_menu.addAction(open_action)

        save_action = QAction("Save", self)
        save_action.setShortcut(QKeySequence("Ctrl+S"))
        save_action.triggered.connect(self.save_current_file)
        file_menu.addAction(save_action)

        save_as_action = QAction("Save As...", self)
        save_as_action.setShortcut(QKeySequence("Ctrl+Shift+S"))
        save_as_action.triggered.connect(self.save_current_file_as)
        file_menu.addAction(save_as_action)

        file_menu.addSeparator()
        close_action = QAction("Close", self)
        close_action.setShortcut(QKeySequence("Ctrl+W"))
        close_action.triggered.connect(self.close_current_tab)
        file_menu.addAction(close_action)

        close_all_action = QAction("Close All", self)
        close_all_action.triggered.connect(self.close_all_tabs)
        file_menu.addAction(close_all_action)

        file_menu.addSeparator()
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

        # Edit
        edit_menu = menubar.addMenu("Edit")
        undo_action = QAction("Undo", self)
        undo_action.setShortcut(QKeySequence("Ctrl+Z"))
        undo_action.triggered.connect(lambda: self.current_editor().undo())
        edit_menu.addAction(undo_action)

        redo_action = QAction("Redo", self)
        redo_action.setShortcut(QKeySequence("Ctrl+Y"))
        redo_action.triggered.connect(lambda: self.current_editor().redo())
        edit_menu.addAction(redo_action)

        edit_menu.addSeparator()
        cut_action = QAction("Cut", self)
        cut_action.setShortcut(QKeySequence("Ctrl+X"))
        cut_action.triggered.connect(lambda: self.current_editor().cut())
        edit_menu.addAction(cut_action)

        copy_action = QAction("Copy", self)
        copy_action.setShortcut(QKeySequence("Ctrl+C"))
        copy_action.triggered.connect(lambda: self.current_editor().copy())
        edit_menu.addAction(copy_action)

        paste_action = QAction("Paste", self)
        paste_action.setShortcut(QKeySequence("Ctrl+V"))
        paste_action.triggered.connect(lambda: self.current_editor().paste())
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
        run_action.triggered.connect(self.run_current_file)
        run_menu.addAction(run_action)

        go_to_action = QAction("Go to Include", self)
        go_to_action.setShortcut(QKeySequence("F12"))
        go_to_action.triggered.connect(self.goto_include_under_cursor)
        run_menu.addAction(go_to_action)

    def setup_shortcuts(self):
        pass

    # --- Управление вкладками и редакторами ---
    def current_editor(self) -> CustomScintilla:
        return self.tab_widget.currentWidget()

    def add_editor_tab(self, editor: CustomScintilla, filename: str = None, title: str = "Untitled"):
        editor.set_main_window(self)
        index = self.tab_widget.addTab(editor, title)
        self.tab_widget.setCurrentIndex(index)
        editor.filename = filename
        editor.modificationChanged.connect(lambda modified: self.update_tab_title(editor, modified))
        editor.gotoDefinitionRequested.connect(self.open_include_file)
        self.setup_editor_widget(editor)

    def setup_editor_widget(self, editor):
        editor.setUtf8(True)
        editor.setPaper(QColor("#1e1e2e"))
        editor.setColor(QColor("#cdd6f4"))

        safe_font = self.get_safe_font("Consolas", 12)
        editor.setFont(safe_font)

        editor.setCaretForegroundColor(QColor("#ffffff"))
        editor.setCaretLineVisible(True)
        editor.setCaretLineBackgroundColor(QColor("#313244"))

        editor.setMarginsBackgroundColor(QColor("#181825"))
        editor.setMarginsForegroundColor(QColor("#6c7086"))
        editor.setMarginType(0, QsciScintilla.MarginType.NumberMargin)

        self.update_margin_width(editor)

        editor.setBraceMatching(QsciScintilla.BraceMatch.SloppyBraceMatch)
        editor.setMatchedBraceBackgroundColor(QColor("#f5c2e755"))
        editor.setMatchedBraceForegroundColor(QColor("#f5c2e7"))

        editor.setIndentationsUseTabs(False)
        editor.setTabWidth(4)
        editor.setIndentationGuides(True)

        editor.setSelectionBackgroundColor(QColor("#585b70"))
        editor.setSelectionForegroundColor(QColor("#ffffff"))

        editor.textChanged.connect(lambda: self.update_margin_width(editor))

        # Лексер
        lexer = TwistLangLexer(editor)
        editor.setLexer(lexer)

        # Включение автодополнения
        editor.setAutoCompletionSource(QsciScintilla.AutoCompletionSource.AcsAPIs)
        editor.setAutoCompletionThreshold(1)
        editor.setAutoCompletionCaseSensitivity(False)
        editor.setAutoCompletionReplaceWord(False)

        

        # Автодополнение с иконками
        self.setup_autocompletion_icons(editor, lexer)

    
    def create_type_pixmap(self, text, color, size):
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        painter = QPainter(pixmap)
        painter.setPen(QPen(color))
        painter.setFont(QFont("Arial", size, QFont.Weight.Bold))
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, text)
        painter.end()
        return pixmap

    def setup_autocompletion_icons(self, editor, lexer: TwistLangLexer):
        size = lexer.font_size
        img_keyword = self.create_type_pixmap("K", QColor("#f5c2e7"), size)
        img_modifier = self.create_type_pixmap("M", QColor("#b4befe"), size)
        img_type = self.create_type_pixmap("T", QColor("#cba6f7"), size)
        img_literal = self.create_type_pixmap("L", QColor("#f9e2af"), size)
        img_directive = self.create_type_pixmap("D", QColor("#f5e0dc"), size)
        img_special = self.create_type_pixmap("S", QColor("#f2b5b5"), size)
        img_function = self.create_type_pixmap("F", QColor("#89b4fa"), size)

        editor.registerImage(1, img_keyword)
        editor.registerImage(2, img_modifier)
        editor.registerImage(3, img_type)
        editor.registerImage(4, img_literal)
        editor.registerImage(5, img_directive)
        editor.registerImage(6, img_special)
        editor.registerImage(7, img_function)

        api = QsciAPIs(lexer)
        
        for word in lexer.keywords:
            api.add(word + "?1")
        for word in lexer.modifiers:
            api.add(word + "?2")
        for word in lexer.types:
            api.add(word + "?3")
        for word in lexer.literals:
            api.add(word + "?4")
        for word in lexer.special_keywords:
            api.add(word + "?6")
        

        # Добавляем описания (calltips) для директив
        for word in lexer.directives:
            if word == '#macro':
                api.add(word + " : Define a macro: #macro function name and arguments = body ?5 ")
            elif word == '#define':
                api.add(word + " : Define a constant: #define name = value ?5 ")
            elif word == '#include':
                api.add(word + " : Include another file: #include \"filename\" ?5 ")

        # Подготавливаем API и устанавливаем его в лексер (ОДИН раз!)
        api.prepare()
        lexer.setAPIs(api)

    def update_autocompletion_icons(self, editor):
        """Обновить иконки автодополнения для указанного редактора (после изменения масштаба)."""
        lexer = editor.lexer()
        if not isinstance(lexer, TwistLangLexer):
            return
        size = lexer.font_size
        img_keyword = self.create_type_pixmap("K", QColor("#f5c2e7"), size)
        img_modifier = self.create_type_pixmap("M", QColor("#b4befe"), size)
        img_type = self.create_type_pixmap("T", QColor("#cba6f7"), size)
        img_literal = self.create_type_pixmap("L", QColor("#f9e2af"), size)
        img_directive = self.create_type_pixmap("D", QColor("#f5e0dc"), size)
        img_special = self.create_type_pixmap("S", QColor("#f2b5b5"), size)
        img_function = self.create_type_pixmap("F", QColor("#89b4fa"), size)

        editor.registerImage(1, img_keyword)
        editor.registerImage(2, img_modifier)
        editor.registerImage(3, img_type)
        editor.registerImage(4, img_literal)
        editor.registerImage(5, img_directive)
        editor.registerImage(6, img_special)
        editor.registerImage(7, img_function)

    def update_margin_width(self, editor):
        lines = editor.lines()
        if lines == 0:
            lines = 1
        digits = len(str(lines))
        sample = "9" * (digits + 2)
        editor.setMarginWidth(0, sample)

    def get_safe_font(self, preferred_font, size):
        font = QFont(preferred_font, size)
        if not font.exactMatch():
            safe_fonts = ["Courier New", "DejaVu Sans Mono", "Monospace"]
            for font_name in safe_fonts:
                font = QFont(font_name, size)
                if font.exactMatch():
                    return font
        return font

    def update_tab_title(self, editor, modified):
        index = self.tab_widget.indexOf(editor)
        if index == -1:
            return
        base = self.tab_widget.tabText(index)
        if base.endswith('*'):
            base = base[:-1]
        if modified:
            self.tab_widget.setTabText(index, base + '*')
        else:
            self.tab_widget.setTabText(index, base)

    # --- Действия с файлами ---
    def new_file(self):
        editor = CustomScintilla()
        self.add_editor_tab(editor, filename=None, title="Untitled")

    def open_file_dialog(self):
        filename, _ = QFileDialog.getOpenFileName(
            self, "Open File", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            self.open_file(filename)

    def open_file(self, filename):
        # Проверяем, не открыт ли уже
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and getattr(editor, 'filename', None) == filename:
                self.tab_widget.setCurrentIndex(i)
                return
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                content = f.read()
            editor = CustomScintilla()
            editor.setText(content)
            editor.setModified(False)
            self.add_editor_tab(editor, filename=filename, title=os.path.basename(filename))
            self.status_bar.showMessage(f"Opened: {filename}", 2000)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not open file: {e}")

    def save_current_file(self):
        editor = self.current_editor()
        if not editor:
            return
        filename = getattr(editor, 'filename', None)
        if filename:
            self.save_editor_to_file(editor, filename)
        else:
            self.save_current_file_as()

    def save_current_file_as(self):
        editor = self.current_editor()
        if not editor:
            return
        self.save_editor_as(editor)

    def save_editor_as(self, editor):
        filename, _ = QFileDialog.getSaveFileName(
            self, "Save File As", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            if not filename.endswith('.twist'):
                filename += '.twist'
            self.save_editor_to_file(editor, filename)
            editor.filename = filename
            index = self.tab_widget.indexOf(editor)
            if index != -1:
                self.tab_widget.setTabText(index, os.path.basename(filename) + ('*' if editor.isModified() else ''))

    def save_editor_to_file(self, editor, filename):
        try:
            text = editor.text()
            cleaned_text = text.replace('\x00', '')
            normalized = cleaned_text.replace('\r\n', '\n').replace('\r', '\n')
            if os.linesep != '\n':
                normalized = normalized.replace('\n', os.linesep)
            with open(filename, 'w', encoding='utf-8', newline='') as f:
                f.write(normalized)
            editor.setModified(False)
            self.status_bar.showMessage(f"Saved: {filename}", 2000)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not save file: {e}")

    def close_current_tab(self):
        index = self.tab_widget.currentIndex()
        if index >= 0:
            self.tab_widget.close_tab(index)

    def close_all_tabs(self):
        while self.tab_widget.count() > 0:
            self.tab_widget.close_tab(0)

    # --- Запуск кода ---
    def run_current_file(self):
        editor = self.current_editor()
        if not editor:
            return
        filename = getattr(editor, 'filename', None)
        if not filename:
            reply = QMessageBox.question(
                self,
                "Save File",
                "File is not saved. Save before running?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            if reply == QMessageBox.StandardButton.Yes:
                self.save_current_file()
                filename = getattr(editor, 'filename', None)
                if not filename:
                    return
            else:
                return
        # Сохраняем перед запуском
        self.save_editor_to_file(editor, filename)

        if not shutil.which("twistc"):
            QMessageBox.critical(
                self,
                "Compiler not found",
                "The 'twistc' compiler is not in PATH. Please install it or add to PATH.",
                QMessageBox.StandardButton.Ok
            )
            return

        try:
            if platform.system() == "Windows":
                cmd = f'start cmd /k twistc --file "{filename}"'
                subprocess.Popen(cmd, shell=True)
                self.status_bar.showMessage(f"Running {os.path.basename(filename)} in new cmd window...", 3000)
            elif platform.system() == "Linux":
                terminals = ['gnome-terminal', 'xterm', 'konsole', 'xfce4-terminal']
                term_cmd = None
                for term in terminals:
                    if shutil.which(term):
                        term_cmd = term
                        break
                if term_cmd:
                    if term_cmd == 'gnome-terminal':
                        subprocess.Popen([term_cmd, '--', 'bash', '-c', f'twistc --file "{filename}"; read -p "Press enter..."'])
                    else:
                        subprocess.Popen([term_cmd, '-e', f'twistc --file "{filename}"; read -p "Press enter..."'])
                else:
                    subprocess.Popen(['twistc', '--file', filename])
                self.status_bar.showMessage(f"Running {os.path.basename(filename)}...", 3000)
            else:  # macOS и др.
                QMessageBox.information(
                    self,
                    "Not Fully Supported",
                    f"Please run manually:\ntwistc --file {filename}",
                    QMessageBox.StandardButton.Ok
                )
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run: {e}")

    # --- Переход по @include ---
    def open_include_file(self, path, line):
        if not path:
            return
        editor = self.sender()
        if not isinstance(editor, CustomScintilla):
            editor = self.current_editor()
        if not editor:
            return
        current_file = getattr(editor, 'filename', None)
        base_dir = os.path.dirname(current_file) if current_file else os.getcwd()
        full_path = os.path.join(base_dir, path)
        if os.path.exists(full_path):
            self.open_file(full_path)
        elif os.path.exists(path):
            self.open_file(path)
        else:
            QMessageBox.warning(self, "File not found", f"Cannot find include file: {path}")

    def goto_include_under_cursor(self):
        editor = self.current_editor()
        if not editor:
            return
        line, _ = editor.getCursorPosition()
        if editor.isIncludeLine(line):
            path = editor.extractIncludePath(line)
            if path:
                self.open_include_file(path, line)
            else:
                self.status_bar.showMessage("No valid include path found", 2000)
        else:
            self.status_bar.showMessage("Not an @include line", 2000)
   
    def zoom_in(self):
        editor = self.current_editor()
        if not editor:
            return
        lexer = editor.lexer()
        if isinstance(lexer, TwistLangLexer):
            new_size = lexer.change_font_size(1)
            default_font = lexer.font(lexer.STYLE_DEFAULT)
            editor.setFont(default_font)
            self.update_margin_width(editor)
            self.update_autocompletion_icons(editor)
            self.status_bar.showMessage(f"Font size: {new_size}pt", 2000)

    def zoom_out(self):
        editor = self.current_editor()
        if not editor:
            return
        lexer = editor.lexer()
        if isinstance(lexer, TwistLangLexer):
            new_size = lexer.change_font_size(-1)
            default_font = lexer.font(lexer.STYLE_DEFAULT)
            editor.setFont(default_font)
            self.update_margin_width(editor)
            self.update_autocompletion_icons(editor)
            self.status_bar.showMessage(f"Font size: {new_size}pt", 2000)


# ----------------------------------------------------------------------
# Запуск приложения
# ----------------------------------------------------------------------
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
    editor.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
