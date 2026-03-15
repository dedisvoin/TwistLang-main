import sys
import os
import subprocess
import platform
import shutil
import re
from datetime import datetime


from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QStatusBar,
    QPushButton, QLabel, QTabWidget, QTabBar, QToolButton,
    QFileDialog, QMessageBox, QToolTip,
)
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QPainter, QPen, QIcon, QPixmap, QMouseEvent,
)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer, QSize


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
    contentChanged = pyqtSignal()  # сигнал об изменении содержимого

    INDICATOR_ERROR = 8
    WARNING_ERROR = 9

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMouseTracking(True)
        self.ctrlPressed = False
        self.main_window = None
        self.last_save_time = datetime.now()
        self.filename = None

        self.errors = []  # (line, start_col, end_col, message)

        self.setup_error_indicator()
        self.textChanged.connect(self.on_text_changed)
        self.current_tooltip_msg = None

    def set_main_window(self, window):
        self.main_window = window

    def setup_error_indicator(self):
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.INDICATOR_ERROR
        )
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.WARNING_ERROR
        )
        self.setIndicatorForegroundColor(QColor("#f35815"), self.INDICATOR_ERROR)
        self.setIndicatorForegroundColor(QColor("#ffc413"), self.WARNING_ERROR)
        self.SendScintilla(QsciScintilla.SCI_SETINDICATORCURRENT, self.INDICATOR_ERROR)
        self.SendScintilla(QsciScintilla.SCI_SETINDICATORCURRENT, self.WARNING_ERROR)

    def set_errors(self, errors):
        self.clear_errors()
        for error in errors:
            self.add_error(*error)

    def on_text_changed(self):
        self.clear_errors()
        self.contentChanged.emit()

    def clear_errors(self):
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.INDICATOR_ERROR)
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.WARNING_ERROR)
        self.errors.clear()

    def add_error(self, line, start_col, end_col, message, type_):
        start_col += 1
        end_col += 1
        self.fillIndicatorRange(line, start_col, line, end_col, self.INDICATOR_ERROR if type_ == 0 else self.WARNING_ERROR)
        self.errors.append((line, start_col, end_col, message))

    def get_error_at_position(self, line, col):
        for err_line, start_col, end_col, message in self.errors:
            if err_line == line and start_col <= col < end_col:
                return message
        return None

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_ParenLeft:
            self.insert_pair('(', ')')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_BracketLeft:
            self.insert_pair('[', ']')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_BraceLeft:
            self.insert_pair('{', '}')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_QuoteDbl:
            self.insert_pair('"', '"')
            event.accept()
            return
        elif event.key() == Qt.Key.Key_Apostrophe:
            self.insert_pair("'", "'")
            event.accept()
            return
        elif event.key() == Qt.Key.Key_Control:
            self.ctrlPressed = True
            super().keyPressEvent(event)
        else:
            super().keyPressEvent(event)

        self.contentChanged.emit()

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.ctrlPressed = False
        super().keyReleaseEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        pos = event.pos()
        position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
        line, col = self.lineIndexFromPosition(position)

        error_msg = self.get_error_at_position(line, col) if line >= 0 and col >= 0 else None

        if error_msg != self.current_tooltip_msg:
            if error_msg:
                QToolTip.showText(event.globalPosition().toPoint(), error_msg, self)
            else:
                QToolTip.hideText()
            self.current_tooltip_msg = error_msg

        super().mouseMoveEvent(event)

    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.ctrlPressed:
            pos = event.pos()
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
        return text.startswith('#include')

    def extractIncludePath(self, line):
        text = self.text(line).strip()
        if not text.startswith('#include'):
            return None
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
# Лексер TwistLang (Catppuccin Mocha / Latte)
# ----------------------------------------------------------------------
class TwistLangLexer(QsciLexerCustom):
    def __init__(self, parent=None, dark_theme=False, font_size=12):
        super().__init__(parent)
        self.dark_theme = dark_theme
        self.font_size = font_size
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
        self.STYLE_NAMESPACE_ID = 11
        self.STYLE_SPECIAL = 12
        self.STYLE_OBJECT = 13
        self.setup_styles()

        self.keywords = {
            'if', 'else', 'for', 'while', 'let', 'in', 'and', 'or',
            'ret', 'assert', 'lambda', 'do',
            'struct', 'namespace', 'func', 'continue;', 'break;'
        }
        self.modifiers = {'const', 'static', 'global', 'final', 'private'}
        self.types = {'Int', 'Bool', 'String', 'Char', 'Null', 'Double',
                      'Namespace', 'Func', 'Lambda', 'auto', "Type"}
        self.literals = {'true', 'false', 'null', 'self'}
        self.directives = {'#define', '#macro', '#include'}
        self.special_keywords = {'new', 'del', 'typeof', 'sizeof', 'out', 'outln', 'input'}

    def set_theme(self, dark):
        self.dark_theme = dark
        self.setup_styles()

    def set_font_size(self, size):
        self.font_size = size
        self.setup_styles()

    def wordCharacters(self):
        return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"

    def setup_styles(self):
        if self.dark_theme:
            bg_color = QColor("#1e1e2e")
            fg_color = QColor("#cdd6f4")
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
                self.STYLE_OBJECT: QColor("#f2cdcd"),
            }
        else:
            bg_color = QColor("#eff1f5")
            fg_color = QColor("#4c4f69")
            colors = {
                self.STYLE_KEYWORD: QColor("#859900"),      # зелёный
                self.STYLE_TYPE: QColor("#b58900"),         # жёлтый
                self.STYLE_COMMENT: QColor("#93a1a1"),      # светло-серый
                self.STYLE_STRING: QColor("#2aa198"),       # бирюзовый
                self.STYLE_NUMBER: QColor("#d33682"),       # розовый
                self.STYLE_OPERATOR: QColor("#657b83"),     # основной текст
                self.STYLE_FUNCTION: QColor("#268bd2"),     # синий
                self.STYLE_MODIFIER: QColor("#6c71c4"),     # фиолетовый
                self.STYLE_DIRECTIVE: QColor("#cb4b16"),    # оранжевый
                self.STYLE_LITERAL: QColor("#dc322f"),      # красный
                self.STYLE_NAMESPACE_ID: QColor("#268bd2"), # синий
                self.STYLE_SPECIAL: QColor("#d33682"),      # розовый
                self.STYLE_OBJECT: QColor("#b58900"),       # жёлтый
            }

        self.setDefaultPaper(bg_color)
        self.setDefaultColor(fg_color)

        safe_font = self.get_safe_font("Consolas", self.font_size)
        self.setFont(safe_font)

        for style, color in colors.items():
            self.setColor(color, style)

        italic_font = self.get_safe_font("Consolas", self.font_size)
        italic_font.setItalic(True)
        self.setFont(italic_font, self.STYLE_COMMENT)

        bold_font = self.get_safe_font("Consolas", self.font_size)
        bold_font.setBold(True)
        bold_font.setItalic(True)
        self.setFont(bold_font, self.STYLE_KEYWORD)

        underline_font = self.get_safe_font("Consolas", self.font_size)
        underline_font.setUnderline(True)
        self.setFont(underline_font, self.STYLE_MODIFIER)

        specials_font = self.get_safe_font("Consolas", self.font_size)
        specials_font.setItalic(True)
        self.setFont(specials_font, self.STYLE_SPECIAL)

    def get_safe_font(self, preferred_font, size):
        font = QFont(preferred_font, size)
        return font

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

        pos = start
        expecting_namespace = False

        def _get_char_at(byte_pos):
            if byte_pos >= total_bytes:
                return (None, 0)
            b = text_bytes[byte_pos]
            if b & 0x80 == 0:
                length = 1
            elif b & 0xE0 == 0xC0:
                length = 2
            elif b & 0xF0 == 0xE0:
                length = 3
            elif b & 0xF8 == 0xF0:
                length = 4
            else:
                length = 1

            if byte_pos + length > total_bytes:
                length = 1

            try:
                ch = text_bytes[byte_pos:byte_pos+length].decode('utf-8')
            except UnicodeDecodeError:
                ch = '\ufffd'

            return (ch, length)

        while pos < end:
            ch, ch_len = _get_char_at(pos)
            if ch_len == 0:
                break

            if ch == '/' and pos + ch_len < end:
                next_ch, next_len = _get_char_at(pos + ch_len)
                if next_ch == '/':
                    expecting_namespace = False
                    j = pos
                    while j < end:
                        c, l = _get_char_at(j)
                        if c == '\n':
                            j += l
                            break
                        j += l
                    self.setStyling(j - pos, self.STYLE_COMMENT)
                    pos = j
                    continue

            if ch in ('"', "'"):
                expecting_namespace = False
                quote = ch
                j = pos + ch_len
                escaped = False
                while j < end:
                    c, l = _get_char_at(j)
                    if c == '\\':
                        escaped = not escaped
                    elif c == quote and not escaped:
                        j += l
                        break
                    else:
                        escaped = False
                    j += l
                self.setStyling(j - pos, self.STYLE_STRING)
                pos = j
                continue

            if ch.isdigit() or (ch == '.' and pos + ch_len < end and _get_char_at(pos + ch_len)[0].isdigit()):
                expecting_namespace = False
                if ch.isdigit():
                    j = pos
                    while j < end:
                        c, l = _get_char_at(j)
                        if not (c.isdigit() or c == '.'):
                            break
                        if c == '.':
                            if any(cc == '.' for cc in text_bytes[pos:j].decode('utf-8')):
                                break
                        j += l
                    self.setStyling(j - pos, self.STYLE_NUMBER)
                    pos = j
                    continue
                else:
                    j = pos + ch_len
                    while j < end:
                        c, l = _get_char_at(j)
                        if not c.isdigit():
                            break
                        j += l
                    self.setStyling(j - pos, self.STYLE_NUMBER)
                    pos = j
                    continue

            if ch.isalpha() or ch == '_' or ch == '#':
                j = pos
                while j < end:
                    c, l = _get_char_at(j)
                    if not (c.isalnum() or c == '_' or c == '#'):
                        break
                    j += l

                word = text_bytes[pos:j].decode('utf-8')
                style = self.STYLE_DEFAULT

                if word == "namespace" or word == "struct":
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
                    elif j + 2 <= end and text_bytes[j:j+2] == b'::':
                        style = self.STYLE_NAMESPACE_ID
                    elif j < end and _get_char_at(j)[0] == '(':
                        style = self.STYLE_FUNCTION
                    elif j < end and _get_char_at(j)[0] == '.':
                        style = self.STYLE_OBJECT
                    else:
                        style = self.STYLE_DEFAULT

                self.setStyling(j - pos, style)
                pos = j
                continue

            if ch in '+-*/%=&|^!<>~?.:;(){}[]':
                expecting_namespace = False
                j = pos + ch_len
                if j < end:
                    next_ch, next_len = _get_char_at(j)
                    if (ch == ':' and next_ch == ':') or \
                       (ch == '-' and next_ch == '>') or \
                       (ch == '<' and next_ch == '=') or \
                       (ch == '>' and next_ch == '=') or \
                       (ch == '=' and next_ch == '=') or \
                       (ch == '!' and next_ch == '=') or \
                       (ch == '<' and next_ch == '<') or \
                       (ch == '>' and next_ch == '>'):
                        j += next_len
                self.setStyling(j - pos, self.STYLE_OPERATOR)
                pos = j
                continue

            self.setStyling(ch_len, self.STYLE_DEFAULT)
            pos += ch_len


# ----------------------------------------------------------------------
# Виджет вкладок с редакторами
# ----------------------------------------------------------------------
class EditorTabWidget(QTabWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTabsClosable(False)
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
        
        # Определяем цвет крестика в зависимости от текущей темы
        main_window = self.window()
        if hasattr(main_window, 'dark_theme') and not main_window.dark_theme:
            # Светлая тема - темный крестик
            painter.setPen(QPen(QColor("#4c4f69"), 2))
        else:
            # Темная тема - светлый крестик
            painter.setPen(QPen(QColor("#cdd6f4"), 2))
        
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
            filename = getattr(editor, 'filename', None)

            if filename:
                main_window = self.window()
                if hasattr(main_window, 'stop_language_server'):
                    main_window.stop_language_server(filename)

            if editor.isModified():
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
        self.status_bar.setStyleSheet("""
                QStatusBar {
                    background-color: #2d2d3a;
                    color: #ffffff;
                    border-top: 1px solid #4c4f69;
                }
                QStatusBar::item {
                    border: none;
                }
            """)
        self.setStatusBar(self.status_bar)

        self.ls_processes = {}  # ключ: путь к файлу, значение: процесс
        self.current_file = None
        self.dark_theme = False
        self.global_font_size = 12  # глобальный размер шрифта

        self.autosave_timer = QTimer(self)
        self.autosave_timer.timeout.connect(self.autosave_all_files)
        self.autosave_interval = 100
        self.autosave_timer.start(self.autosave_interval)

        self.error_check_timer = QTimer(self)
        self.error_check_timer.timeout.connect(self.check_for_errors)
        self.error_check_timer.start(300)

        self.create_menu()
        self.setup_shortcuts()

        self.setAcceptDrops(True)
        self.tab_widget.currentChanged.connect(self.on_tab_changed)

        self.autosave_label = QLabel("⚡ Auto-save: ON")
        self.autosave_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.status_bar.addPermanentWidget(self.autosave_label)

        self.error_label = QLabel("✓ No errors")
        self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.status_bar.addPermanentWidget(self.error_label)

        self.ls_status_label = QLabel("⚙ LS: idle")
        self.ls_status_label.setStyleSheet("color: #89b4fa; padding: 2px 5px;")
        self.status_bar.addPermanentWidget(self.ls_status_label)

        self.autosave_count = 0

    def update_all_close_icons(self):
        """Обновить иконки закрытия на всех вкладках при смене темы"""
        for i in range(self.tab_widget.count()):
            tab_bar = self.tab_widget.tabBar()
            btn = tab_bar.tabButton(i, QTabBar.ButtonPosition.RightSide)
            if btn and isinstance(btn, QToolButton):
                btn.setIcon(self.tab_widget._create_close_icon())

    def toggle_theme(self):
        self.dark_theme = not self.dark_theme
        self.update_all_editor_themes()
        self.update_application_palette()
        self.status_bar.showMessage(f"Theme switched to {'dark' if self.dark_theme else 'light'}", 2000)

    def update_all_editor_themes(self):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                # Сохраняем текст и позицию
                text = editor.text()
                line, col = editor.getCursorPosition()
                scroll_pos = editor.SendScintilla(editor.SCI_GETFIRSTVISIBLELINE)

                # Создаём новый лексер с текущими темой и размером шрифта
                new_lexer = TwistLangLexer(editor, self.dark_theme, self.global_font_size)
                editor.setLexer(new_lexer)

                # Восстанавливаем текст и позицию
                editor.setText(text)
                editor.setCursorPosition(line, col)
                editor.SendScintilla(editor.SCI_SETFIRSTVISIBLELINE, scroll_pos)

                # Применяем остальные настройки темы
                self.apply_editor_theme(editor)
                self.update_autocompletion_icons(editor)
                editor.setMarginsFont(new_lexer.font(new_lexer.STYLE_DEFAULT))
                self.update_margin_width(editor)

                editor.repaint()
        self.update_all_close_icons()

    def apply_editor_theme(self, editor: CustomScintilla):
        if self.dark_theme:
            editor.setCaretForegroundColor(QColor("#ffffff"))
            editor.setCaretLineBackgroundColor(QColor("#313244"))
            editor.setMarginsBackgroundColor(QColor("#181825"))
            editor.setMarginsForegroundColor(QColor("#6c7086"))
            editor.setSelectionBackgroundColor(QColor("#585b70"))
            editor.setSelectionForegroundColor(QColor("#ffffff"))
            editor.setMatchedBraceBackgroundColor(QColor("#f5c2e755"))
            editor.setMatchedBraceForegroundColor(QColor("#f5c2e7"))
        else:
            editor.setCaretForegroundColor(QColor("#1e1e2e"))
            editor.setCaretLineBackgroundColor(QColor("#e6e9ef"))
            editor.setMarginsBackgroundColor(QColor("#dce0e8"))
            editor.setMarginsForegroundColor(QColor("#6c6f85"))
            editor.setSelectionBackgroundColor(QColor("#acb0be"))
            editor.setSelectionForegroundColor(QColor("#1e1e2e"))
            editor.setMatchedBraceBackgroundColor(QColor("#8839ef55"))
            editor.setMatchedBraceForegroundColor(QColor("#8839ef"))
        editor.update()

    def update_application_palette(self):
        app = QApplication.instance()
        palette = app.palette()
        if self.dark_theme:
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
            
            # Статус бар для тёмной темы
            self.status_bar.setStyleSheet("""
                QStatusBar {
                    background-color: #181825;
                    color: #cdd6f4;
                    border-top: 1px solid #313244;
                }
                QStatusBar::item {
                    border: none;
                }
            """)
        else:
            palette.setColor(palette.ColorRole.Window, QColor("#eff1f5"))
            palette.setColor(palette.ColorRole.WindowText, QColor("#4c4f69"))
            palette.setColor(palette.ColorRole.Base, QColor("#e6e9ef"))
            palette.setColor(palette.ColorRole.AlternateBase, QColor("#dce0e8"))
            palette.setColor(palette.ColorRole.ToolTipBase, QColor("#ccd0da"))
            palette.setColor(palette.ColorRole.ToolTipText, QColor("#4c4f69"))
            palette.setColor(palette.ColorRole.Text, QColor("#4c4f69"))
            palette.setColor(palette.ColorRole.Button, QColor("#ccd0da"))
            palette.setColor(palette.ColorRole.ButtonText, QColor("#4c4f69"))
            palette.setColor(palette.ColorRole.BrightText, QColor("#dc8a78"))
            palette.setColor(palette.ColorRole.Link, QColor("#1e66f5"))
            palette.setColor(palette.ColorRole.Highlight, QColor("#acb0be"))
            palette.setColor(palette.ColorRole.HighlightedText, QColor("#1e1e2e"))
            
            # Статус бар для светлой темы (тёмный)
            self.status_bar.setStyleSheet("""
                QStatusBar {
                    background-color: #2d2d3a;
                    color: #ffffff;
                    border-top: 1px solid #4c4f69;
                }
                QStatusBar::item {
                    border: none;
                }
            """)
        
        app.setPalette(palette)
        
        # Обновляем цвета текста в лейблах статус-бара для лучшей читаемости на тёмном фоне
        self.autosave_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.ls_status_label.setStyleSheet("color: #89b4fa; padding: 2px 5px;")

    def resizeEvent(self, event):
        super().resizeEvent(event)
        button_size = self.run_button.size()
        margin = 20
        x = self.width() - button_size.width() - margin
        y = self.height() - button_size.height() - margin - self.status_bar.height()
        self.run_button.move(x, y)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if os.path.isfile(file_path):
                self.open_file(file_path)
        event.acceptProposedAction()

    def create_menu(self):
        menubar = self.menuBar()

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

        autosave_menu = file_menu.addMenu("Auto-save")
        self.autosave_action = QAction("Enable Auto-save", self)
        self.autosave_action.setCheckable(True)
        self.autosave_action.setChecked(True)
        self.autosave_action.triggered.connect(self.toggle_autosave)
        autosave_menu.addAction(self.autosave_action)

        autosave_menu.addSeparator()
        interval_menu = autosave_menu.addMenu("Interval")
        intervals = [
            ("0.5 seconds", 500),
            ("1 seconds", 1000),
            ("1 minute", 60000),
            ("2 minutes", 120000),
            ("5 minutes", 300000)
        ]
        for name, ms in intervals:
            action = QAction(name, self)
            action.setCheckable(True)
            action.setData(ms)
            if ms == self.autosave_interval:
                action.setChecked(True)
            action.triggered.connect(lambda checked, ms=ms: self.set_autosave_interval(ms))
            interval_menu.addAction(action)

        autosave_menu.addSeparator()
        save_now_action = QAction("Save Now", self)
        save_now_action.setShortcut(QKeySequence("Ctrl+Shift+S"))
        save_now_action.triggered.connect(lambda: self.autosave_all_files(manual=True))
        autosave_menu.addAction(save_now_action)

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

        view_menu = menubar.addMenu("View")
        zoom_in_action = QAction("Zoom In", self)
        zoom_in_action.setShortcut(QKeySequence("Ctrl+="))
        zoom_in_action.triggered.connect(self.zoom_in)
        view_menu.addAction(zoom_in_action)

        zoom_out_action = QAction("Zoom Out", self)
        zoom_out_action.setShortcut(QKeySequence("Ctrl+-"))
        zoom_out_action.triggered.connect(self.zoom_out)
        view_menu.addAction(zoom_out_action)

        view_menu.addSeparator()
        theme_action = QAction("Toggle Dark/Light Theme", self)
        theme_action.setShortcut(QKeySequence("Ctrl+T"))
        theme_action.triggered.connect(self.toggle_theme)
        view_menu.addAction(theme_action)

        run_menu = menubar.addMenu("Run")
        run_action = QAction("Run Code", self)
        run_action.setShortcut(QKeySequence("F5"))
        run_action.triggered.connect(self.run_current_file)
        run_menu.addAction(run_action)

        go_to_action = QAction("Go to Include", self)
        go_to_action.setShortcut(QKeySequence("F12"))
        go_to_action.triggered.connect(self.goto_include_under_cursor)
        run_menu.addAction(go_to_action)

        run_menu.addSeparator()
        clear_errors_action = QAction("Clear Errors", self)
        clear_errors_action.setShortcut(QKeySequence("Ctrl+E"))
        clear_errors_action.triggered.connect(self.clear_all_errors)
        run_menu.addAction(clear_errors_action)

    def setup_shortcuts(self):
        QShortcut(QKeySequence("F8"), self).activated.connect(self.goto_next_error)
        QShortcut(QKeySequence("Shift+F8"), self).activated.connect(self.goto_prev_error)

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
        self.apply_editor_theme(editor)

    def setup_editor_widget(self, editor):
        editor.setUtf8(True)
        safe_font = self.get_safe_font("Consolas", self.global_font_size)
        editor.setFont(safe_font)
        # Устанавливаем такой же шрифт для номеров строк
        editor.setMarginsFont(safe_font)
        editor.setCaretLineVisible(True)
        editor.setMarginType(0, QsciScintilla.MarginType.NumberMargin)
        self.update_margin_width(editor)
        editor.setBraceMatching(QsciScintilla.BraceMatch.SloppyBraceMatch)
        editor.setIndentationsUseTabs(False)
        editor.setTabWidth(4)
        editor.setIndentationGuides(True)
        editor.textChanged.connect(lambda: self.update_margin_width(editor))

        lexer = TwistLangLexer(editor, self.dark_theme, self.global_font_size)
        editor.setLexer(lexer)

        editor.setAutoCompletionSource(QsciScintilla.AutoCompletionSource.AcsAPIs)
        editor.setAutoCompletionThreshold(1)
        editor.setAutoCompletionCaseSensitivity(False)
        editor.setAutoCompletionReplaceWord(False)

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
        img_keyword = self.create_type_pixmap("K", lexer.color(lexer.STYLE_KEYWORD), size)
        img_modifier = self.create_type_pixmap("M", lexer.color(lexer.STYLE_MODIFIER), size)
        img_type = self.create_type_pixmap("T", lexer.color(lexer.STYLE_TYPE), size)
        img_literal = self.create_type_pixmap("L", lexer.color(lexer.STYLE_LITERAL), size)
        img_directive = self.create_type_pixmap("D", lexer.color(lexer.STYLE_DIRECTIVE), size)
        img_special = self.create_type_pixmap("S", lexer.color(lexer.STYLE_SPECIAL), size)
        img_function = self.create_type_pixmap("F", lexer.color(lexer.STYLE_FUNCTION), size)

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

        for word in lexer.directives:
            if word == '#macro':
                api.add(word + " : Define a macro: #macro function name and arguments = body ?5 ")
            elif word == '#define':
                api.add(word + " : Define a constant: #define name = value ?5 ")
            elif word == '#include':
                api.add(word + " : Include another file: #include \"filename\" ?5 ")

        api.prepare()
        lexer.setAPIs(api)

    def update_autocompletion_icons(self, editor):
        lexer = editor.lexer()
        if not isinstance(lexer, TwistLangLexer):
            return
        size = lexer.font_size
        img_keyword = self.create_type_pixmap("K", lexer.color(lexer.STYLE_KEYWORD), size)
        img_modifier = self.create_type_pixmap("M", lexer.color(lexer.STYLE_MODIFIER), size)
        img_type = self.create_type_pixmap("T", lexer.color(lexer.STYLE_TYPE), size)
        img_literal = self.create_type_pixmap("L", lexer.color(lexer.STYLE_LITERAL), size)
        img_directive = self.create_type_pixmap("D", lexer.color(lexer.STYLE_DIRECTIVE), size)
        img_special = self.create_type_pixmap("S", lexer.color(lexer.STYLE_SPECIAL), size)
        img_function = self.create_type_pixmap("F", lexer.color(lexer.STYLE_FUNCTION), size)

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
            for font_name in ["Courier New", "DejaVu Sans Mono", "Monospace"]:
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
        self.tab_widget.setTabText(index, base + ('*' if modified else ''))

    def get_error_filename(self, file_path):
        if not file_path:
            return None
        base_name = os.path.splitext(os.path.basename(file_path))[0]
        return f"{base_name}_ls.dbg"

    def parse_error_file(self, error_file_path):
        errors = []
        if not os.path.exists(error_file_path):
            return errors
        try:
            with open(error_file_path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    match = re.match(
                        r"pif:\s*'((?:[a-zA-Z]:)?[^:]+?)':(\d+):(\d+):(\d+):(\d+)\s+message:\s*(.+)",
                        line
                    )
                    if match:
                        line_num = int(match.group(2))
                        pos = int(match.group(3))
                        length = int(match.group(4))
                        t = int(match.group(5))
                        message = match.group(6).strip()

                        errors.append((
                            line_num - 1,
                            pos - 1,
                            (pos - 1) + length,
                            message,
                            t
                        ))
        except Exception as e:
            print(f"Error parsing {error_file_path}: {e}")
        return errors

    def check_for_errors(self):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and editor.filename:
                error_filename = self.get_error_filename(editor.filename)
                if error_filename and os.path.exists(error_filename):
                    errors = self.parse_error_file(error_filename)
                    editor.set_errors(errors)
                else:
                    editor.clear_errors()

        current = self.current_editor()
        if current and current.filename:
            err_count = len(current.errors)
            if err_count > 0:
                self.error_label.setText(f"✗ {err_count} error(s)")
                self.error_label.setStyleSheet("color: #f38ba8; padding: 2px 5px;")
            else:
                self.error_label.setText("✓ No errors")
                self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        else:
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")

    def check_file_errors(self, editor):
        if not editor or not editor.filename:
            return

        error_filename = self.get_error_filename(editor.filename)
        if error_filename and os.path.exists(error_filename):
            errors = self.parse_error_file(error_filename)
            editor.set_errors(errors)
        else:
            editor.clear_errors()

        if editor == self.current_editor():
            err_count = len(editor.errors)
            if err_count > 0:
                self.error_label.setText(f"✗ {err_count} error(s)")
                self.error_label.setStyleSheet("color: #f38ba8; padding: 2px 5px;")
            else:
                self.error_label.setText("✓ No errors")
                self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")

    def start_language_server(self, file_path):
        if not file_path:
            return

        if file_path in self.ls_processes:
            proc = self.ls_processes[file_path]
            if proc.poll() is None:
                return
            else:
                del self.ls_processes[file_path]

        try:
            creationflags = subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0
            ls_path = os.path.join('bin', 'twist-ls')
            if platform.system() == "Windows" and not os.path.exists(ls_path):
                ls_path += '.exe'

            process = subprocess.Popen(
                [ls_path, '--file', file_path, '-d'],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                creationflags=creationflags
            )

            self.ls_processes[file_path] = process
            print(f"Started LS for {os.path.basename(file_path)}")
            self.update_ls_status()

            editor = self.current_editor()
            if editor and editor.filename == file_path:
                QTimer.singleShot(500, lambda: self.check_file_errors(editor))

        except Exception as e:
            print(f"Failed to start LS for {file_path}: {e}")

    def stop_language_server(self, file_path):
        if file_path not in self.ls_processes:
            return

        try:
            process = self.ls_processes[file_path]
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()

            print(f"Stopped LS for {os.path.basename(file_path)}")
            del self.ls_processes[file_path]
            self.update_ls_status()
        except Exception as e:
            print(f"Failed to stop LS for {file_path}: {e}")

    def stop_all_language_servers(self):
        for file_path in list(self.ls_processes.keys()):
            self.stop_language_server(file_path)

    def update_ls_status(self):
        if self.current_file and self.current_file in self.ls_processes:
            proc = self.ls_processes[self.current_file]
            if proc.poll() is None:
                self.ls_status_label.setText(f"⚙ LS: active for {os.path.basename(self.current_file)}")
                self.ls_status_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
            else:
                del self.ls_processes[self.current_file]
                self.ls_status_label.setText("⚙ LS: idle")
                self.ls_status_label.setStyleSheet("color: #89b4fa; padding: 2px 5px;")
        else:
            self.ls_status_label.setText("⚙ LS: idle")
            self.ls_status_label.setStyleSheet("color: #89b4fa; padding: 2px 5px;")

    def on_tab_changed(self, index):
        editor = self.tab_widget.widget(index)
        if not isinstance(editor, CustomScintilla):
            return

        new_file = editor.filename
        if new_file == self.current_file:
            return

        if self.current_file:
            self.stop_language_server(self.current_file)

        if new_file:
            self.start_language_server(new_file)

        self.current_file = new_file
        self.update_ls_status()

        if new_file:
            self.check_file_errors(editor)

    def new_file(self):
        self.add_editor_tab(CustomScintilla(), filename=None, title="Untitled")

    def open_file_dialog(self):
        filename, _ = QFileDialog.getOpenFileName(
            self, "Open File", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            self.open_file(filename)

    def open_file(self, filename):
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

            if self.tab_widget.currentWidget() == editor:
                self.start_language_server(filename)
                self.current_file = filename
                self.update_ls_status()

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
        self.save_editor_as(self.current_editor())

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

    def save_editor_to_file(self, editor: CustomScintilla, filename):
        try:
            text = editor.text()
            cleaned_text = text.replace('\x00', '')
            normalized = cleaned_text.replace('\r\n', '\n').replace('\r', '\n')

            with open(filename, 'w', encoding='utf-8', newline='') as f:
                f.write(normalized)

            editor.setModified(False)
            editor.last_save_time = datetime.now()
            self.status_bar.showMessage(f"Saved: {filename}", 2000)

            if filename == self.current_file and filename in self.ls_processes:
                self.stop_language_server(filename)
                QTimer.singleShot(100, lambda: self.start_language_server(filename))

        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not save file: {e}")

    def toggle_autosave(self, checked):
        if checked:
            self.autosave_timer.start(self.autosave_interval)
            self.autosave_label.setText(f"⚡ Auto-save: ON ({self.autosave_interval//1000}s)")
            self.autosave_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
            self.status_bar.showMessage("Auto-save enabled", 2000)
        else:
            self.autosave_timer.stop()
            self.autosave_label.setText("⚡ Auto-save: OFF")
            self.autosave_label.setStyleSheet("color: #f38ba8; padding: 2px 5px;")
            self.status_bar.showMessage("Auto-save disabled", 2000)

    def set_autosave_interval(self, ms):
        self.autosave_interval = ms
        if self.autosave_action.isChecked():
            self.autosave_timer.start(ms)

        status = "ON" if self.autosave_action.isChecked() else "OFF"
        self.autosave_label.setText(f"⚡ Auto-save: {status} ({ms//1000}s)")

        interval_menu = self.sender().parent()
        for action in interval_menu.actions():
            if action.data() == ms:
                action.setChecked(True)
            elif action.data() is not None:
                action.setChecked(False)

        self.status_bar.showMessage(f"Auto-save interval set to {ms//1000} seconds", 2000)

    def autosave_all_files(self, manual=False):
        if not self.autosave_action.isChecked() and not manual:
            return

        saved_count = 0
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                filename = getattr(editor, 'filename', None)
                if filename and editor.isModified():
                    try:
                        self.save_editor_to_file(editor, filename)
                        saved_count += 1
                    except Exception as e:
                        print(f"Auto-save error for {filename}: {e}")

        if saved_count > 0:
            self.autosave_count += 1
            if manual:
                self.status_bar.showMessage(f"Manual save: {saved_count} file(s) saved", 2000)
            else:
                self.status_bar.showMessage(f"Auto-saved {saved_count} file(s) [{self.autosave_count}]", 1500)
                self.autosave_label.setStyleSheet("color: #f9e2af; padding: 2px 5px;")
                QTimer.singleShot(1000, lambda: self.autosave_label.setStyleSheet(
                    "color: #a6e3a1; padding: 2px 5px;" if self.autosave_action.isChecked()
                    else "color: #f38ba8; padding: 2px 5px;"
                ))
        elif manual:
            self.status_bar.showMessage("No files to save", 2000)

    def clear_all_errors(self):
        editor = self.current_editor()
        if editor:
            editor.clear_errors()
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
            self.status_bar.showMessage("Errors cleared", 2000)

    def goto_next_error(self):
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.status_bar.showMessage("No errors", 2000)
            return

        current_line, current_col = editor.getCursorPosition()
        for error in editor.errors:
            if error[0] > current_line or (error[0] == current_line and error[1] > current_col):
                editor.setCursorPosition(error[0], error[1])
                editor.ensureLineVisible(error[0])
                self.status_bar.showMessage(f"Error at line {error[0] + 1}", 2000)
                return
        self.status_bar.showMessage("No more errors", 2000)

    def goto_prev_error(self):
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.status_bar.showMessage("No errors", 2000)
            return

        current_line, current_col = editor.getCursorPosition()
        for error in reversed(editor.errors):
            if error[0] < current_line or (error[0] == current_line and error[1] < current_col):
                editor.setCursorPosition(error[0], error[1])
                editor.ensureLineVisible(error[0])
                self.status_bar.showMessage(f"Error at line {error[0] + 1}", 2000)
                return
        self.status_bar.showMessage("No previous errors", 2000)

    def close_current_tab(self):
        index = self.tab_widget.currentIndex()
        if index >= 0:
            self.tab_widget.close_tab(index)

    def close_all_tabs(self):
        while self.tab_widget.count() > 0:
            self.tab_widget.close_tab(0)

    def closeEvent(self, event):
        self.stop_all_language_servers()
        super().closeEvent(event)

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

        self.save_editor_to_file(editor, filename)

        try:
            if platform.system() == "Windows":
                subprocess.Popen(f'start cmd /k bin\\twistc --file "{filename}"', shell=True)
                self.status_bar.showMessage(f"Running {os.path.basename(filename)} in new cmd window...", 3000)
            elif platform.system() == "Linux":
                terminals = ['gnome-terminal', 'xterm', 'konsole', 'xfce4-terminal']
                term_cmd = next((term for term in terminals if shutil.which(term)), None)
                if term_cmd:
                    if term_cmd == 'gnome-terminal':
                        subprocess.Popen([term_cmd, '--', 'bash', '-c', f'twistc --file "{filename}"; read -p "Press enter..."'])
                    else:
                        subprocess.Popen([term_cmd, '-e', f'twistc --file "{filename}"; read -p "Press enter..."'])
                else:
                    subprocess.Popen(['twistc', '--file', filename])
                self.status_bar.showMessage(f"Running {os.path.basename(filename)}...", 3000)
            else:
                QMessageBox.information(self, "Not Fully Supported",
                                        f"Please run manually:\ntwistc --file {filename}",
                                        QMessageBox.StandardButton.Ok)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run: {e}")

    def open_include_file(self, path, line):
        if not path:
            return
        editor = self.sender() if isinstance(self.sender(), CustomScintilla) else self.current_editor()
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

    def apply_global_font_to_all_editors(self):
        """Применить глобальный размер шрифта ко всем редакторам"""
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                lexer = editor.lexer()
                if isinstance(lexer, TwistLangLexer):
                    lexer.set_font_size(self.global_font_size)
                    editor.setFont(lexer.font(lexer.STYLE_DEFAULT))
                    editor.setMarginsFont(lexer.font(lexer.STYLE_DEFAULT))
                    self.update_margin_width(editor)
                    self.update_autocompletion_icons(editor)
                    editor.recolor()
                    editor.repaint()

    def zoom_in(self):
        if self.global_font_size < 40:
            self.global_font_size += 1
            self.apply_global_font_to_all_editors()
            self.status_bar.showMessage(f"Font size: {self.global_font_size}pt", 2000)

    def zoom_out(self):
        if self.global_font_size > 7:
            self.global_font_size -= 1
            self.apply_global_font_to_all_editors()
            self.status_bar.showMessage(f"Font size: {self.global_font_size}pt", 2000)


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
    palette.setColor(palette.ColorRole.Window, QColor("#eff1f5"))
    palette.setColor(palette.ColorRole.WindowText, QColor("#4c4f69"))
    palette.setColor(palette.ColorRole.Base, QColor("#e6e9ef"))
    palette.setColor(palette.ColorRole.AlternateBase, QColor("#dce0e8"))
    palette.setColor(palette.ColorRole.ToolTipBase, QColor("#ccd0da"))
    palette.setColor(palette.ColorRole.ToolTipText, QColor("#4c4f69"))
    palette.setColor(palette.ColorRole.Text, QColor("#4c4f69"))
    palette.setColor(palette.ColorRole.Button, QColor("#ccd0da"))
    palette.setColor(palette.ColorRole.ButtonText, QColor("#4c4f69"))
    palette.setColor(palette.ColorRole.BrightText, QColor("#dc8a78"))
    palette.setColor(palette.ColorRole.Link, QColor("#1e66f5"))
    palette.setColor(palette.ColorRole.Highlight, QColor("#acb0be"))
    palette.setColor(palette.ColorRole.HighlightedText, QColor("#1e1e2e"))
    
    app.setPalette(palette)

    editor = TwistLangEditor()
    editor.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()