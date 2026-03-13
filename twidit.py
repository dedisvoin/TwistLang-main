import sys
import os
import subprocess
import tempfile
import platform
import shutil
import re
from pathlib import Path
from datetime import datetime

from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QWidget,
    QStatusBar, QSplitter, QTextEdit, QHBoxLayout,
    QPushButton, QLineEdit, QLabel, QDockWidget,
    QGridLayout, QSizePolicy, QScrollArea, QFrame,
    QToolBar, QTabWidget, QTabBar, QMenuBar, QToolButton,
    QFileDialog, QMessageBox, QInputDialog, QToolTip,
)
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QTextCursor, QKeyEvent, QPainter, QTextCharFormat,
    QBrush, QPen, QPalette, QIcon, QPixmap, QMouseEvent,
    QDragEnterEvent, QDropEvent, 
)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer, QThread, QPoint, QSize, QUrl, QRect


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
    
    # Константа для индикатора ошибок
    INDICATOR_ERROR = 8  # Номер индикатора для ошибок
    WARNING_ERROR = 9

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMouseTracking(True)
        self.ctrlPressed = False
        self.main_window = None  # будет установлено при создании
        self.last_save_time = datetime.now()
        self.filename = None
        
        # Список ошибок для текущего файла
        self.errors = []  # Каждая ошибка: (line, start_col, end_col, message)
        
        # Настройка индикатора для ошибок
        self.setup_error_indicator()
        
        # Подключаем сигнал для очистки ошибок при изменении текста
        self.textChanged.connect(self.on_text_changed)

        self.current_tooltip_msg = None

    def set_main_window(self, window):
        self.main_window = window

    def setup_error_indicator(self):
        """Настройка индикатора для отображения ошибок"""
        # Сначала проверяем, какие индикаторы доступны
        # Используем другой номер индикатора (21 - обычно свободен)
        
        
        # Устанавливаем индикатор с явными параметрами
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.INDICATOR_ERROR
        )
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.WARNING_ERROR
        )
        
        
        # Устанавливаем цвет с полной прозрачностью фона и ярким красным
        self.setIndicatorForegroundColor(QColor("#f35815"), self.INDICATOR_ERROR)  # Ярко-красный для теста
        self.setIndicatorForegroundColor(QColor("#ffc413"), self.WARNING_ERROR)  # Ярко-красный для теста
        
       
        
        # Включаем индикаторы
        self.SendScintilla(QsciScintilla.SCI_SETINDICATORCURRENT, self.INDICATOR_ERROR)
        self.SendScintilla(QsciScintilla.SCI_SETINDICATORCURRENT, self.WARNING_ERROR)
        

    def set_errors(self, errors):
        """Установить список ошибок"""
        print(f"Setting {len(errors)} errors")  # Отладка
        self.clear_errors()
        for i, error in enumerate(errors):
            print(f"  Error {i}: {error}")  # Отладка
            self.add_error(*error)

    def on_text_changed(self):
        """Вызывается при изменении текста - очищаем старые ошибки"""
        self.clear_errors()
        self.contentChanged.emit()

    def clear_errors(self):
        """Очистить все индикаторы ошибок"""
        # Очищаем индикаторы во всём документе
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.INDICATOR_ERROR)
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.WARNING_ERROR)
        self.errors.clear()

    def add_error(self, line, start_col, end_col, message, type_):
        # Смещение на один символ вправо (как требует пользователь)
        start_col += 1
        end_col += 1

        
        # Добавляем индикатор
        self.fillIndicatorRange(line, start_col, line, end_col, self.INDICATOR_ERROR if type_ == 0 else self.WARNING_ERROR)
        # Сохраняем информацию об ошибке (уже со скорректированными координатами)
        self.errors.append((line, start_col, end_col, message))

    def set_errors(self, errors):
        """Установить список ошибок
        
        Args:
            errors: список кортежей (line, start_col, end_col, message)
        """
        self.clear_errors()
        for error in errors:
            self.add_error(*error)

    def get_error_at_position(self, line, col):
        """Получить ошибку в указанной позиции"""
        for err_line, start_col, end_col, message in self.errors:
            if err_line == line and start_col <= col < end_col:
                return message
        return None

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
        
        # Отправляем сигнал об изменении содержимого
        self.contentChanged.emit()

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.ctrlPressed = False
        super().keyReleaseEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        pos = event.pos()
        position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
        line, col = self.lineIndexFromPosition(position)

        error_msg = None
        if line >= 0 and col >= 0:
            error_msg = self.get_error_at_position(line, col)

        # Если сообщение изменилось – обновляем подсказку
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
        return text.startswith('#include')

    def extractIncludePath(self, line):
        text = self.text(line).strip()
        if not text.startswith('#include'):
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
            'ret', 'assert', 'lambda', 'do',
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

        # Преобразуем в байты UTF-8
        text_bytes = text.encode('utf-8')
        total_bytes = len(text_bytes)

        start = max(0, start)
        end = min(total_bytes, end)

        if start >= end:
            return

        self.startStyling(start)

        pos = start
        # Флаг, указывающий, что следующий идентификатор будет именем namespace (после keyword namespace или struct)
        expecting_namespace = False

        def _get_char_at(byte_pos):
            """Возвращает кортеж (символ, длина_в_байтах) для позиции byte_pos."""
            if byte_pos >= total_bytes:
                return (None, 0)
            b = text_bytes[byte_pos]
            # Определяем длину UTF-8 символа по первому байту
            if b & 0x80 == 0:
                length = 1
            elif b & 0xE0 == 0xC0:
                length = 2
            elif b & 0xF0 == 0xE0:
                length = 3
            elif b & 0xF8 == 0xF0:
                length = 4
            else:
                length = 1  # на всякий случай

            if byte_pos + length > total_bytes:
                length = 1

            try:
                ch = text_bytes[byte_pos:byte_pos+length].decode('utf-8')
            except UnicodeDecodeError:
                ch = '\ufffd'  # символ замены

            return (ch, length)

        while pos < end:
            ch, ch_len = _get_char_at(pos)
            if ch_len == 0:
                break

            # --- Комментарий //
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

            # --- Строковые литералы
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

            # --- Числа
            if ch.isdigit() or (ch == '.' and pos + ch_len < end and _get_char_at(pos + ch_len)[0].isdigit()):
                expecting_namespace = False
                if ch.isdigit():
                    j = pos
                    while j < end:
                        c, l = _get_char_at(j)
                        if not (c.isdigit() or c == '.'):
                            break
                        # Не допускаем две точки
                        if c == '.':
                            # проверим, не было ли уже точки
                            if any(cc == '.' for cc in text_bytes[pos:j].decode('utf-8')):
                                break
                        j += l
                    self.setStyling(j - pos, self.STYLE_NUMBER)
                    pos = j
                    continue
                else:  # случай ".цифра"
                    j = pos + ch_len
                    while j < end:
                        c, l = _get_char_at(j)
                        if not c.isdigit():
                            break
                        j += l
                    self.setStyling(j - pos, self.STYLE_NUMBER)
                    pos = j
                    continue

            # --- Идентификаторы (буквы, _, #)
            if ch.isalpha() or ch == '_' or ch == '#':
                j = pos
                while j < end:
                    c, l = _get_char_at(j)
                    if not (c.isalnum() or c == '_' or c == '@'):
                        break
                    j += l

                word = text_bytes[pos:j].decode('utf-8')
                style = self.STYLE_DEFAULT

                # Определяем стиль по слову
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
                    # Если после слова идут два двоеточия, это имя пространства имён
                    elif j + 2 <= end and text_bytes[j:j+2] == b'::':
                        style = self.STYLE_NAMESPACE_ID
                    # Если следующий символ — открывающая скобка, это функция
                    elif j < end and _get_char_at(j)[0] == '(':
                        style = self.STYLE_FUNCTION
                    # Если следующий символ — точка, это имя объекта
                    elif j < end and _get_char_at(j)[0] == '.':
                        style = self.STYLE_OBJECT
                    else:
                        style = self.STYLE_DEFAULT

                self.setStyling(j - pos, style)
                pos = j
                continue

            # --- Операторы и знаки пунктуации
            if ch in '+-*/%=&|^!<>~?.:;(){}[]':
                # Многосимвольные операторы: ::, ->, <=, >=, ==, !=, <<, >>
                j = pos + ch_len
                if j < end:
                    next_ch, next_len = _get_char_at(j)
                    # Проверяем двухсимвольные операторы
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

            # --- Всё остальное
            self.setStyling(ch_len, self.STYLE_DEFAULT)
            pos += ch_len

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

        # === Таймер для автосохранения ===
        self.autosave_timer = QTimer(self)
        self.autosave_timer.timeout.connect(self.autosave_all_files)
        self.autosave_interval = 500 
        self.autosave_timer.start(self.autosave_interval)

        # === Таймер для проверки ошибок ===
        self.error_check_timer = QTimer(self)
        self.error_check_timer.timeout.connect(self.check_for_errors)
        self.error_check_timer.start(500)  # Проверка каждые 2 секунды

        self.create_menu()
        self.setup_shortcuts()
        self.apply_stylesheet()

        # Включаем приём файлов через Drag & Drop
        self.setAcceptDrops(True)

        # Создаём пустую вкладку при старте
        self.new_file()

        # Добавляем индикатор автосохранения в статус-бар
        self.autosave_label = QLabel("⚡ Auto-save: ON")
        self.autosave_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.status_bar.addPermanentWidget(self.autosave_label)
        
        # Добавляем индикатор ошибок в статус-бар
        self.error_label = QLabel("✓ No errors")
        self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
        self.status_bar.addPermanentWidget(self.error_label)
        
        # Счётчик автосохранений
        self.autosave_count = 0

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

        # Меню автосохранения
        autosave_menu = file_menu.addMenu("Auto-save")
        
        self.autosave_action = QAction("Enable Auto-save", self)
        self.autosave_action.setCheckable(True)
        self.autosave_action.setChecked(True)
        self.autosave_action.triggered.connect(self.toggle_autosave)
        autosave_menu.addAction(self.autosave_action)
        
        autosave_menu.addSeparator()
        
        # Подменю для выбора интервала
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
        
        run_menu.addSeparator()
        
        clear_errors_action = QAction("Clear Errors", self)
        clear_errors_action.setShortcut(QKeySequence("Ctrl+E"))
        clear_errors_action.triggered.connect(self.clear_all_errors)
        run_menu.addAction(clear_errors_action)

    def setup_shortcuts(self):
        # Добавляем горячие клавиши для навигации по ошибкам
        next_error_shortcut = QShortcut(QKeySequence("F8"), self)
        next_error_shortcut.activated.connect(self.goto_next_error)
        
        prev_error_shortcut = QShortcut(QKeySequence("Shift+F8"), self)
        prev_error_shortcut.activated.connect(self.goto_prev_error)

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
        # Подключаем сигнал изменения содержимого
        editor.contentChanged.connect(lambda: self.on_editor_content_changed(editor))
        self.setup_editor_widget(editor)

    def on_editor_content_changed(self, editor):
        """Обработчик изменения содержимого редактора"""
        # При изменении содержимого сразу проверяем ошибки для этого файла
        if editor.filename:
            self.check_file_errors(editor)

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
            # Проверяем ошибки для открытого файла
            self.check_file_errors(editor)
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
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not save file: {e}")

    # --- Методы для автосохранения ---
    def toggle_autosave(self, checked):
        """Включение/выключение автосохранения"""
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
        """Установка интервала автосохранения"""
        self.autosave_interval = ms
        if self.autosave_action.isChecked():
            self.autosave_timer.start(ms)
        
        # Обновляем текст в статус-баре
        status = "ON" if self.autosave_action.isChecked() else "OFF"
        self.autosave_label.setText(f"⚡ Auto-save: {status} ({ms//1000}s)")
        
        # Обновляем меню (снимаем галочки с других пунктов)
        interval_menu = self.sender().parent()
        for action in interval_menu.actions():
            if action.data() == ms:
                action.setChecked(True)
            elif action.data() is not None:
                action.setChecked(False)
        
        self.status_bar.showMessage(f"Auto-save interval set to {ms//1000} seconds", 2000)

    def autosave_all_files(self, manual=False):
        """Автосохранение всех открытых и изменённых файлов"""
        if not self.autosave_action.isChecked() and not manual:
            return
            
        saved_count = 0
        skipped_count = 0
        
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
                elif filename and not editor.isModified():
                    skipped_count += 1
                elif not filename:
                    # Для безымянных файлов не делаем автосохранение
                    skipped_count += 1
        
        if saved_count > 0:
            self.autosave_count += 1
            if manual:
                self.status_bar.showMessage(f"Manual save: {saved_count} file(s) saved", 2000)
            else:
                # Кратковременное сообщение в статус-баре
                self.status_bar.showMessage(f"Auto-saved {saved_count} file(s) [{self.autosave_count}]", 1500)
                
                # Меняем цвет индикатора на секунду
                self.autosave_label.setStyleSheet("color: #f9e2af; padding: 2px 5px;")
                QTimer.singleShot(1000, lambda: self.autosave_label.setStyleSheet(
                    "color: #a6e3a1; padding: 2px 5px;" if self.autosave_action.isChecked() 
                    else "color: #f38ba8; padding: 2px 5px;"
                ))
        elif manual:
            self.status_bar.showMessage("No files to save", 2000)

    # --- Методы для работы с ошибками ---
    def parse_error_file(self, filename):
        errors_by_file = {}
        if not os.path.exists("err.dbg"):
            return errors_by_file
        try:
            with open("err.dbg", 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    # Формат: pif: {file_name}:{line}:{pos}:{lenght} message: {message}
                    pattern = r'pif:\s*([^:]+):(\d+):(\d+):(\d+):(\d+)\s+message:\s*(.+)'
                    match = re.match(pattern, line)
                    if match:
                        file_name = match.group(1).strip()[1:-1]  # убираем кавычки вокруг имени
                        line_num = int(match.group(2))   # 1-based строка
                        pos = int(match.group(3))       # 1-based позиция
                        length = int(match.group(4))
                        t = int(match.group(5))
                        message = match.group(6).strip()
                        
                        # Преобразуем в 0-based для редактора
                        line_0 = line_num - 1
                        start_col_0 = pos - 1
                        end_col_0 = start_col_0 + length
                        
                        if file_name not in errors_by_file:
                            errors_by_file[file_name] = []
                        errors_by_file[file_name].append((line_0, start_col_0, end_col_0, message, t))
        except Exception as e:
            print(f"Error parsing err.dbg: {e}")
        return errors_by_file

    def check_for_errors(self):
        """Периодическая проверка файла err.dbg для всех открытых редакторов"""
        errors_by_file = self.parse_error_file("err.dbg")
        
        
        total_errors = 0
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and editor.filename:
                base_name = os.path.basename(editor.filename)
                if base_name in errors_by_file:
                    editor.set_errors(errors_by_file[base_name])
                    total_errors += len(errors_by_file[base_name])
                else:
                    # Если для файла нет ошибок, очищаем
                    editor.clear_errors()
        
        # Обновляем индикатор в статус-баре
        if total_errors > 0:
            self.error_label.setText(f"✗ {total_errors} error(s)")
            self.error_label.setStyleSheet("color: #f38ba8; padding: 2px 5px;")
        else:
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")

    def check_file_errors(self, editor):
        """Проверка ошибок для конкретного файла"""
        if not editor.filename:
            return
            
        errors_by_file = self.parse_error_file("err.dbg")
        base_name = os.path.basename(editor.filename)
        
        if base_name in errors_by_file:
            editor.set_errors(errors_by_file[base_name])
            total = len(errors_by_file[base_name])
            self.error_label.setText(f"✗ {total} error(s)")
            self.error_label.setStyleSheet("color: #f38ba8; padding: 2px 5px;")
        else:
            editor.clear_errors()
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")

    def clear_all_errors(self):
        """Очистить все ошибки в текущем редакторе"""
        editor = self.current_editor()
        if editor:
            editor.clear_errors()
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet("color: #a6e3a1; padding: 2px 5px;")
            self.status_bar.showMessage("Errors cleared", 2000)

    def goto_next_error(self):
        """Перейти к следующей ошибке"""
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.status_bar.showMessage("No errors", 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        
        # Ищем следующую ошибку
        next_error = None
        for error in editor.errors:
            if error[0] > current_line or (error[0] == current_line and error[1] > current_col):
                next_error = error
                break
                
        if next_error:
            line, col, _, _ = next_error
            editor.setCursorPosition(line, col)
            editor.ensureLineVisible(line)
            self.status_bar.showMessage(f"Error at line {line + 1}", 2000)
        else:
            self.status_bar.showMessage("No more errors", 2000)

    def goto_prev_error(self):
        """Перейти к предыдущей ошибке"""
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.status_bar.showMessage("No errors", 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        
        # Ищем предыдущую ошибку
        prev_error = None
        for error in reversed(editor.errors):
            if error[0] < current_line or (error[0] == current_line and error[1] < current_col):
                prev_error = error
                break
                
        if prev_error:
            line, col, _, _ = prev_error
            editor.setCursorPosition(line, col)
            editor.ensureLineVisible(line)
            self.status_bar.showMessage(f"Error at line {line + 1}", 2000)
        else:
            self.status_bar.showMessage("No previous errors", 2000)

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

        try:
            if platform.system() == "Windows":
                cmd = f'start cmd /k bin\\twistc --file "{filename}"'
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
    os.system('start bin\\twist-ls --file main.twist -d')
    main()