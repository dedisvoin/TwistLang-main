"""
Lumen IDE - Integrated Development Environment for TwistLang
"""

import sys
import os
import subprocess
import platform
import re
from datetime import datetime
from typing import Optional, Dict, List, Tuple, Set
from dataclasses import dataclass
from enum import Enum

from PyQt6.QtGui import QFileSystemModel, QFontMetrics, QPainterPath, QRegion
from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QFileIconProvider, QMainWindow, QMenu, QScrollArea, QSizePolicy, QStatusBar,
    QLabel, QTabWidget, QTabBar, QToolButton, QTreeView, QSplitter,
    QFileDialog, QMessageBox, QToolTip, QWidget, QHBoxLayout,
    QVBoxLayout, QFrame, QStackedWidget
)
from PyQt6.QtCore import QEvent, QPointF, QPropertyAnimation, QEasingCurve, QVariantAnimation, Qt, QDir, QModelIndex, pyqtProperty
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QPainter, QPen, QIcon, QPixmap, QMouseEvent,
    QLinearGradient
)
from PyQt6.Qsci import QsciLexerPython, QsciLexerCPP
from PyQt6.QtCore import QByteArray, QRect, QRectF, pyqtSignal, QTimer, QSize, QPoint
from PyQt6.QtSvg import QSvgRenderer

from themes import *


# =============================================================================
# CONSTANTS & CONFIGURATION
# =============================================================================

MAX_FONT_SIZE = 30
MIN_FONT_SIZE = 8

APP_ICON_PATH = r"data\app_icon.svg"

DEFAULT_THEME = "Rosé Pine"
WINDOW_MIN_WIDTH = 400
WINDOW_MIN_HEIGHT = 300
TITLE_BAR_HEIGHT = 32
RESIZE_MARGIN = 8
DEFAULT_FONT_SIZE = 12
MINUTE = 60_000
AUTOSAVE_INTERVALS = [
    ("0.3 seconds", 300),
    ("1 seconds", 1000),
    ("1 minute", MINUTE),
    ("2 minutes", 2 * MINUTE),
    ("5 minutes", 5 * MINUTE)
]

VERSION = "1.07b"


class Language(Enum):
    """Supported languages"""
    ENGLISH = 0
    RUSSIAN = 1


class ErrorType(Enum):
    """Error severity levels"""
    ERROR = 0
    WARNING = 1
    ECHO = 2


class ResizeDirection(Enum):
    """Window resize directions"""
    TOP = "top"
    BOTTOM = "bottom"
    LEFT = "left"
    RIGHT = "right"
    TOP_LEFT = "topleft"
    TOP_RIGHT = "topright"
    BOTTOM_LEFT = "bottomleft"
    BOTTOM_RIGHT = "bottomright"


# =============================================================================
# LOCALIZATION STRINGS
# =============================================================================

class Strings:
    """Localization strings container"""
    
    # English strings
    EN = {
        # Window title
        "window_title": "Lumen IDE",
        
        # File menu
        "file_menu": "File",
        "new": "New",
        "open": "Open...",
        "open_folder": "Open Folder...",
        "save": "Save",
        "save_as": "Save As...",
        "auto_save": "Auto save",
        "enable_auto_save": "Enable Auto-save",
        "interval": "Interval",
        "save_now": "Save Now",
        "close": "Close",
        "close_all": "Close All",
        "exit": "Exit",
        
        # Edit menu
        "edit_menu": "Edit",
        "undo": "Undo",
        "redo": "Redo",
        "cut": "Cut",
        "copy": "Copy",
        "paste": "Paste",
        
        # Settings menu
        "settings_menu": "Settings",
        "zoom_in": "Zoom In",
        "zoom_out": "Zoom Out",
        "theme": "Theme",
        "language": "Language",
        "english": "English",
        "russian": "Russian",
        "about": "About Lumen IDE",
        
        # Run menu
        "run_menu": "Run",
        "run_code": "Run Code",
        "go_to_include": "Go to Include",
        "restart_ls": "Restart Language Server",
        "clear_errors": "Clear Errors",
        "check_errors": "Check Errors",
        
        # Context menu
        "context_cut": "Cut",
        "context_copy": "Copy",
        "context_paste": "Paste",
        "context_select_all": "Select All",
        "context_go_to_include": "Go to Include",
        "context_clear_errors": "Clear Errors",
        
        # Theme preview
        "theme_preview": "Theme Preview",
        
        # Status bar
        "auto_save_on": "Auto-save: ON ({}s)",
        "auto_save_off": "Auto-save: OFF",
        "no_errors": "✓ No errors",
        "errors_count": "✗ {} error(s)",
        "ls_active": "LS: {} active",
        
        # Messages
        "opened_file": "Opened: {}",
        "saved_file": "Saved: {}",
        "theme_switched": "Theme switched to {}",
        "auto_save_enabled": "Auto-save enabled",
        "auto_save_disabled": "Auto-save disabled",
        "auto_save_interval_set": "Auto-save interval set to {} seconds",
        "manual_save": "Manual save: {} file(s) saved",
        "auto_saved": "Auto-saved {} file(s) [{}]",
        "no_files_to_save": "No files to save",
        "font_size": "Font size: {}pt",
        "code_folding_on": "Code folding: ON",
        "code_folding_off": "Code folding: OFF",
        "no_file_open_ls": "No file open to restart language server",
        "ls_restarted": "Language server restarted for {}",
        "no_errors_status": "No errors",
        "error_at_line": "Error at line {}",
        "no_more_errors": "No more errors",
        "no_previous_errors": "No previous errors",
        "errors_cleared": "Errors cleared",
        "not_include_line": "Not an include line",
        "no_valid_include_path": "No valid include path found",
        "save_before_run": "File is not saved. Save before running?",
        "running_file": "Running {}...",
        "not_supported": "Not Supported",
        "run_manually": "Please run manually:\ntwistc --file {}",
        "failed_to_run": "Failed to run: {}",
        "file_not_found": "File not found",
        "cannot_find_include": "Cannot find include file: {}",
        "could_not_open": "Could not open file: {}",
        "could_not_save": "Could not save file: {}",
        "save_changes": "Save Changes",
        "save_changes_question": "Save changes to {}?",
        "error_check_completed": "Error check completed",
        
        # File explorer
        "select_folder": "Select Folder",
        "refresh": "Refresh",
        "collapse_all": "Collapse All",
        "no_folder_open": "No folder open",
        "open_file": "Open File",
        "open_non_lumen": "This is not a Lumen file.\nOpen it anyway?",
        "reveal_in_explorer": "Reveal in Explorer",
        "copy_path": "Copy Path",
        "delete": "Delete",
        "rename": "Rename",
        "confirm_delete": "Confirm Delete",
        "confirm_delete_message": "Delete '{}'?",
        "file_deleted": "Deleted: {}",
        "delete_error": "Failed to delete: {}",
        "path_copied": "Path copied to clipboard!",
        "toggle_explorer": "Toggle File Explorer",
        "folder_opened": "Opened folder: {}",
        "load_project_files": "Load Project Files",
        "load_project_files_question": "Load all .lumen files from this folder?",
        "loaded_files": "Loaded {} file(s)",
        
        # Welcome screen
        "welcome_title": "Lumen IDE",
        "welcome_subtitle": "Integrated development environment for Lumen",
        "author_text": "By Pavlov Ivan Alexeevich",
        "version": f"Version {VERSION}",
        
        # Tooltips
        "toggle_folding": "Toggle code folding (show/hide fold margins)\n[This function in beta test]",
        "code_snap": "Code Snap",
        "error_text": "Inline Messages",
    }
    
    # Russian strings
    RU = {
        # Window title
        "window_title": "Lumen IDE",
        
        # File menu
        "file_menu": "Файл",
        "new": "Новый",
        "open": "Открыть...",
        "open_folder": "Открыть папку...",
        "save": "Сохранить",
        "save_as": "Сохранить как...",
        "auto_save": "Автосохранение",
        "enable_auto_save": "Включить автосохранение",
        "interval": "Интервал",
        "save_now": "Сохранить сейчас",
        "close": "Закрыть",
        "close_all": "Закрыть все",
        "exit": "Выход",
        
        # Edit menu
        "edit_menu": "Правка",
        "undo": "Отменить",
        "redo": "Повторить",
        "cut": "Вырезать",
        "copy": "Копировать",
        "paste": "Вставить",
        
        # Settings menu
        "settings_menu": "Настройки",
        "zoom_in": "Увеличить",
        "zoom_out": "Уменьшить",
        "theme": "Тема",
        "language": "Язык",
        "english": "Английский",
        "russian": "Русский",
        "about": "О Lumen IDE",
        
        # Run menu
        "run_menu": "Запуск",
        "run_code": "Запустить код",
        "go_to_include": "Перейти к include",
        "restart_ls": "Перезапустить языковой сервер",
        "clear_errors": "Очистить ошибки",
        "check_errors": "Проверить ошибки",
        
        # Context menu
        "context_cut": "Вырезать",
        "context_copy": "Копировать",
        "context_paste": "Вставить",
        "context_select_all": "Выделить всё",
        "context_go_to_include": "Перейти к include",
        "context_clear_errors": "Очистить ошибки",
        
        # Theme preview
        "theme_preview": "Превью темы",
        
        # Status bar
        "auto_save_on": "Автосохранение: ВКЛ ({}с)",
        "auto_save_off": "Автосохранение: ВЫКЛ",
        "no_errors": "✓ Нет ошибок",
        "errors_count": "✗ {} ошибка(и)",
        "ls_active": "LS: {} активно",
        
        # Messages
        "opened_file": "Открыт: {}",
        "saved_file": "Сохранен: {}",
        "theme_switched": "Тема изменена на {}",
        "auto_save_enabled": "Автосохранение включено",
        "auto_save_disabled": "Автосохранение выключено",
        "auto_save_interval_set": "Интервал автосохранения установлен на {} секунд",
        "manual_save": "Сохранено вручную: {} файл(ов)",
        "auto_saved": "Автосохранено {} файл(ов) [{}]",
        "no_files_to_save": "Нет файлов для сохранения",
        "font_size": "Размер шрифта: {}pt",
        "code_folding_on": "Сворачивание кода: ВКЛ",
        "code_folding_off": "Сворачивание кода: ВЫКЛ",
        "no_file_open_ls": "Нет открытого файла для перезапуска языкового сервера",
        "ls_restarted": "Языковой сервер перезапущен для {}",
        "no_errors_status": "Нет ошибок",
        "error_at_line": "Ошибка в строке {}",
        "no_more_errors": "Больше нет ошибок",
        "no_previous_errors": "Предыдущих ошибок нет",
        "errors_cleared": "Ошибки очищены",
        "not_include_line": "Это не строка include",
        "no_valid_include_path": "Не найден корректный путь include",
        "save_before_run": "Файл не сохранен. Сохранить перед запуском?",
        "running_file": "Запуск {}...",
        "not_supported": "Не поддерживается",
        "run_manually": "Пожалуйста, запустите вручную:\ntwistc --file {}",
        "failed_to_run": "Не удалось запустить: {}",
        "file_not_found": "Файл не найден",
        "cannot_find_include": "Не удается найти файл include: {}",
        "could_not_open": "Не удалось открыть файл: {}",
        "could_not_save": "Не удалось сохранить файл: {}",
        "save_changes": "Сохранить изменения",
        "save_changes_question": "Сохранить изменения в {}?",
        "error_check_completed": "Проверка ошибок завершена",
        
        # File explorer
        "select_folder": "Выбрать папку",
        "refresh": "Обновить",
        "collapse_all": "Свернуть все",
        "no_folder_open": "Папка не открыта",
        "open_file": "Открыть файл",
        "open_non_lumen": "Это не файл Lumen.\nВсе равно открыть?",
        "reveal_in_explorer": "Показать в проводнике",
        "copy_path": "Копировать путь",
        "delete": "Удалить",
        "rename": "Переименовать",
        "confirm_delete": "Подтверждение удаления",
        "confirm_delete_message": "Удалить '{}'?",
        "file_deleted": "Удален: {}",
        "delete_error": "Не удалось удалить: {}",
        "path_copied": "Путь скопирован!",
        "toggle_explorer": "Показать/скрыть файлы",
        "folder_opened": "Открыта папка: {}",
        "load_project_files": "Загрузить файлы проекта",
        "load_project_files_question": "Загрузить все .lumen файлы из этой папки?",
        "loaded_files": "Загружено {} файл(ов)",
        
        # Welcome screen
        "welcome_title": "Lumen IDE",
        "welcome_subtitle": "Интегрированная среда разработки для Lumen",
        "author_text": "Автор: Павлов Иван Алексеевич",
        "version": f"Версия {VERSION}",
        
        # Tooltips
        "toggle_folding": "Переключить сворачивание кода (показать/скрыть поля сворачивания)\n[Функция в бета-тестировании]",
        "code_snap": "Снимок кода",
        "error_text": "Строковые сообщения",
    }
    
    current_language = Language.ENGLISH
    
    @classmethod
    def set_language(cls, lang: Language):
        """Set current language"""
        cls.current_language = lang
    
    @classmethod
    def get(cls, key: str) -> str:
        """Get localized string"""
        if cls.current_language == Language.ENGLISH:
            return cls.EN.get(key, key)
        else:
            return cls.RU.get(key, key)


# =============================================================================
# UTILITY FUNCTIONS
# =============================================================================

def unescape_message(s: str) -> str:
    """Convert escaped sequences like \\n, \\t back to actual characters."""
    replacements = {
        '\\\\': '\\',
        '\\n': '\n',
        '\\r': '\r',
        '\\t': '\t',
    }
    for escaped, real in replacements.items():
        s = s.replace(escaped, real)
    return s

def create_svg_icon(svg_path: str, color: QColor, size = 32) -> Optional[QIcon]:
    """
    Create an icon from SVG file with specified color.
    
    Args:
        svg_path: Path to SVG file
        color: Target color for the icon
    
    Returns:
        QIcon object or None if creation failed
    """
    try:
        if not os.path.exists(svg_path):
            print(f"SVG file not found: {svg_path}")
            return None
        
        with open(svg_path, 'r', encoding='utf-8') as f:
            svg_content = f.read()
        
        color_hex = color.name()
        svg_content = svg_content.replace('currentColor', color_hex)
        svg_content = svg_content.replace('fill="black"', f'fill="{color_hex}"')
        svg_content = svg_content.replace('stroke="black"', f'stroke="{color_hex}"')
        
        renderer = QSvgRenderer(QByteArray(svg_content.encode()))
        if not renderer.isValid():
            return None
        
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        renderer.render(painter)
        painter.end()
        
        return QIcon(pixmap) if not pixmap.isNull() else None
        
    except Exception as e:
        print(f"Error creating SVG icon: {e}")
        return None


def get_safe_monospace_font(preferred_font: str, size: int) -> QFont:
    """
    Get a monospace font with fallback options.
    
    Args:
        preferred_font: Preferred font name
        size: Font size in points
    
    Returns:
        QFont object that exists on the system
    """
    font = QFont(preferred_font, size)
    if font.exactMatch():
        return font
    
    fallbacks = ["Consolas"]
    for font_name in fallbacks:
        font = QFont(font_name, size)
        if font.exactMatch():
            return font
    
    return QFont("Consolas", size)


# =============================================================================
# THEME PREVIEW WIDGET
# =============================================================================

class ThemePreviewWidget(QWidget):
    """Widget for displaying theme preview with animated theme transitions"""

    def __init__(self, parent=None):
        super().__init__(None, Qt.WindowType.ToolTip | Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setAttribute(Qt.WidgetAttribute.WA_ShowWithoutActivating)
        self.setFixedSize(460, 700)

        self.theme_name = ""
        self._current_colors = None
        self._start_colors = None
        self._end_colors = None
        self._display_colors = None
        self._setup_ui()

        # Пример кода для отображения
        self.code_lines = """#define PI = 3.141592
#macro max(a, b) = if (a > b) {a} else {b}

final static let a: String = "This is text \\n";

namespace std {
    func main(static a: *auto) -> auto { 
        ret null; 
    }

    for (let i = 0; i < 10; i = i + 1;) {
        out i, " ";
    }
}
// this is demo
// is not valid code!!!
outln std::main(3.14);
""".split('\n')
        self.tokenized_lines = []

        # Анимация перехода между темами
        self.transition_animation = QVariantAnimation(self)
        self.transition_animation.setDuration(300)
        self.transition_animation.setEasingCurve(QEasingCurve.Type.InOutCubic)
        self.transition_animation.valueChanged.connect(self._on_animation_value_changed)
        self.transition_animation.finished.connect(self._on_animation_finished)

    def _setup_ui(self):
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(10, 10, 10, 10)
        self.main_layout.setSpacing(6)

        # Заголовок
        self.title_label = QLabel(Strings.get("theme_preview"))
        self.title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title_font = QFont("Consolas", 10, QFont.Weight.Bold)
        self.title_label.setFont(title_font)
        self.main_layout.addWidget(self.title_label)

        # Разделитель
        self.separator = QFrame()
        self.separator.setFixedHeight(1)
        self.main_layout.addWidget(self.separator)

        # Название темы
        self.name_label = QLabel()
        self.name_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        name_font = QFont("Consolas", 9)
        self.name_label.setFont(name_font)
        self.main_layout.addWidget(self.name_label)

        # Небольшой отступ
        spacer = QWidget()
        spacer.setFixedHeight(1)
        self.main_layout.addWidget(spacer)

        # Контейнер для цветовых образцов
        self.swatches_widget = QWidget()
        swatches_layout = QVBoxLayout(self.swatches_widget)
        swatches_layout.setContentsMargins(0, 0, 0, 0)
        swatches_layout.setSpacing(3)

        self.swatch_labels = []

        swatch_items = [
            ("bg", "Background"),
            ("fg", "Foreground"),
            ("keyword", "Keyword"),
            ("type", "Type"),
            ("string", "String"),
            ("comment", "Comment"),
            ("number", "Number"),
            ("operator", "Operator"),
            ("function", "Function"),
            ("modifier", "Modifier"),
            ("directive", "Directive"),
            ("literal", "Literal"),
            ("namespace", "Namespace"),
            ("special", "Special"),
            ("object", "Object"),
            ("error", "Error"),
            ("warning", "Warning"),
            ("echo", "Echo"),
        ]

        for color_key, label_text in swatch_items:
            row = QWidget()
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(0, 0, 0, 0)
            row_layout.setSpacing(10)

            color_box = QLabel()
            color_box.setFixedSize(16, 10)
            color_box.setStyleSheet("border: 1px solid #888; border-radius: 2px;")

            text_label = QLabel(label_text)
            text_label.setFont(QFont("Consolas", 10))

            row_layout.addWidget(color_box)
            row_layout.addWidget(text_label)
            row_layout.addStretch()

            swatches_layout.addWidget(row)
            self.swatch_labels.append((color_key, color_box, text_label))

        self.main_layout.addWidget(self.swatches_widget)

        # Второй разделитель
        self.sep2 = QFrame()
        self.sep2.setFixedHeight(1)
        self.main_layout.addWidget(self.sep2)

        # Область для кода
        self.code_area = QWidget()
        self.code_area.setMinimumHeight(290)
        self.code_area.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)
        self.main_layout.addWidget(self.code_area)

    def update_language(self):
        """Update header text when language changes"""
        self.title_label.setText(Strings.get("theme_preview"))

    def set_theme(self, theme_name: str):
        """Set theme to preview with smooth transition"""
        if theme_name not in THEMES:
            return

        target_colors = THEMES[theme_name]["colors"].copy()

        if self.transition_animation.state() == QVariantAnimation.State.Running:
            self.transition_animation.stop()
            if self._display_colors is not None:
                self._current_colors = self._display_colors.copy()
        elif self._current_colors is None:
            self._current_colors = target_colors.copy()
            self._display_colors = target_colors.copy()
            self.theme_name = theme_name
            self.name_label.setText(theme_name)
            self._apply_colors(target_colors)
            self._tokenize_and_update(target_colors)
            return

        self.theme_name = theme_name
        self.name_label.setText(theme_name)

        self._start_colors = self._current_colors.copy()
        self._end_colors = target_colors

        self.transition_animation.setStartValue(0.0)
        self.transition_animation.setEndValue(1.0)
        self.transition_animation.start()

    def _on_animation_value_changed(self, value):
        progress = value
        start = self._start_colors
        end = self._end_colors

        interp_colors = {}
        for key in start.keys():
            if key in end:
                s_color = start[key]
                e_color = end[key]
                r = int(s_color.red() + (e_color.red() - s_color.red()) * progress)
                g = int(s_color.green() + (e_color.green() - s_color.green()) * progress)
                b = int(s_color.blue() + (e_color.blue() - s_color.blue()) * progress)
                a = int(s_color.alpha() + (e_color.alpha() - s_color.alpha()) * progress)
                interp_colors[key] = QColor(r, g, b, a)
            else:
                interp_colors[key] = start[key]

        self._display_colors = interp_colors
        self._apply_colors(interp_colors)
        self._tokenize_and_update(interp_colors)

    def _on_animation_finished(self):
        if self._end_colors is not None:
            self._current_colors = self._end_colors.copy()
            self._display_colors = self._current_colors.copy()
            self._apply_colors(self._current_colors)
            self._tokenize_and_update(self._current_colors)

    def _apply_colors(self, colors: dict):
        for color_key, color_box, text_label in self.swatch_labels:
            if color_key in colors:
                color_box.setStyleSheet(f"""
                    background-color: {colors[color_key].name()};
                    border: 1px solid {colors.get('status_border', '#888').name()};
                    border-radius: 2px;
                """)
                text_label.setStyleSheet(f"color: {colors.get('fg', '#cdd6f4').name()}; background: transparent;")

        bg_color = colors.get('bg', QColor('#1e1e2e'))
        fg_color = colors.get('fg', QColor('#cdd6f4'))
        border_color = colors.get('status_border', QColor('#313244'))

        self.separator.setStyleSheet(f"""
            QFrame {{
                background-color: {border_color.name()};
                border: none;
            }}
        """)
        self.sep2.setStyleSheet(f"""
            QFrame {{
                background-color: {border_color.name()};
                border: none;
            }}
        """)

        self.setStyleSheet(f"""
            ThemePreviewWidget {{
                background-color: {bg_color.name()};
                border: 1px solid {border_color.name()};
                border-radius: 10px;
            }}
        """)

        self.title_label.setStyleSheet(f"color: {fg_color.name()}; background: transparent;")
        self.name_label.setStyleSheet(f"color: {fg_color.name()}; background: transparent;")

    def _tokenize_and_update(self, colors: dict):
        lexer = TwistLangLexer(theme_name=self.theme_name)
        self.tokenized_lines = []
        for line in self.code_lines:
            tokens = self._highlight_line(line, lexer, colors)
            self.tokenized_lines.append(tokens)
        self.code_area.update()

    def _highlight_line(self, line: str, lexer, colors: dict) -> List[Tuple[str, QColor]]:
        tokens = []
        i = 0
        n = len(line)
        expecting_namespace = False

        while i < n:
            if line.startswith('//', i):
                tokens.append((line[i:], colors.get("comment", colors["fg"])))
                break

            if line[i] in ('"', "'"):
                start = i
                quote = line[i]
                i += 1
                escaped = False
                while i < n:
                    ch = line[i]
                    if ch == '\\':
                        escaped = not escaped
                    elif ch == quote and not escaped:
                        i += 1
                        break
                    else:
                        escaped = False
                    i += 1
                token_text = line[start:i]
                tokens.append((token_text, colors.get("string", colors["fg"])))
                continue

            if line[i].isdigit() or (line[i] == '.' and i + 1 < n and line[i + 1].isdigit()):
                j = i
                has_dot = False
                while j < n and (line[j].isdigit() or (line[j] == '.' and not has_dot)):
                    if line[j] == '.':
                        has_dot = True
                    j += 1
                tokens.append((line[i:j], colors.get("number", colors["fg"])))
                i = j
                continue

            if line[i].isalpha() or line[i] == '_' or line[i] == '#':
                j = i
                while j < n and (line[j].isalnum() or line[j] == '_' or line[j] == '#'):
                    j += 1
                word = line[i:j]
                color = colors["fg"]

                if word == "namespace" or word == "struct":
                    expecting_namespace = True
                    color = colors.get("keyword", colors["fg"])
                elif expecting_namespace:
                    color = colors.get("namespace", colors["fg"])
                    expecting_namespace = False
                else:
                    if word in lexer.keywords:
                        color = colors.get("keyword", colors["fg"])
                    elif word in lexer.special_keywords:
                        color = colors.get("special", colors["fg"])
                    elif word in lexer.modifiers:
                        color = colors.get("modifier", colors["fg"])
                    elif word in lexer.types:
                        color = colors.get("type", colors["fg"])
                    elif word in lexer.literals:
                        color = colors.get("literal", colors["fg"])
                    elif word.startswith('#'):
                        color = colors.get("directive", colors["fg"])
                    elif j < n and line[j:j+2] == '::':
                        color = colors.get("namespace", colors["fg"])
                    elif j < n and line[j] == '(':
                        color = colors.get("function", colors["fg"])
                    elif j < n and line[j] == '.':
                        color = colors.get("object", colors["fg"])

                tokens.append((word, color))
                i = j
                continue

            if line[i] in '+-*/%=&|^!<>~,?.:;(){}[]':
                two_char = line[i:i+2]
                if two_char in {'::', '->', '<=', '>=', '==', '!=', '<<', '>>'}:
                    tokens.append((two_char, colors.get("operator", colors["fg"])))
                    i += 2
                    continue
                tokens.append((line[i], colors.get("operator", colors["fg"])))
                i += 1
                continue

            if line[i].isspace():
                j = i
                while j < n and line[j].isspace():
                    j += 1
                tokens.append((line[i:j], colors["fg"]))
                i = j
                continue

            tokens.append((line[i], colors["fg"]))
            i += 1

        return tokens

    def paintEvent(self, event):
        super().paintEvent(event)
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        if self._display_colors:
            colors = self._display_colors
            bg_color = colors.get('bg', QColor('#1e1e2e'))
            border_color = colors.get('status_border', QColor('#313244'))
        else:
            bg_color = QColor('#1e1e2e')
            border_color = QColor('#313244')

        painter.setBrush(bg_color)
        painter.setPen(QPen(border_color, 1))
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 10, 10)

        if self.tokenized_lines:
            code_rect = self.code_area.geometry()
            x = code_rect.x() + 5
            y = code_rect.y() + 2

            font = QFont("Consolas", 10)
            painter.setFont(font)
            metrics = painter.fontMetrics()
            line_height = metrics.height() + 2

            for line_tokens in self.tokenized_lines:
                current_x = x
                for text, color in line_tokens:
                    painter.setPen(color)
                    painter.drawText(current_x, y + metrics.ascent(), text)
                    current_x += metrics.horizontalAdvance(text)
                y += line_height


# =============================================================================
# FILE EXPLORER WIDGET (с иконками для файлов)
# =============================================================================

class FileExplorer(QWidget):
    """File Explorer widget for project navigation with file icons"""
    
    file_opened = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()
        self.current_root = None
        self.color = QColor("white")
        
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # Tree view only - no header
        self.tree_view = QTreeView()
        self.tree_view.setHeaderHidden(True)
        self.tree_view.setIndentation(15)
        self.tree_view.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tree_view.customContextMenuRequested.connect(self.show_context_menu)
        self.tree_view.doubleClicked.connect(self.on_item_double_clicked)
        
        # File system model with custom icon provider
        self.model = QFileSystemModel()
        self.model.setFilter(QDir.Filter.AllDirs | QDir.Filter.NoDotAndDotDot | QDir.Filter.Files | QDir.Filter.AllEntries)
        self.model.setNameFilterDisables(False)
        
        # ВАЖНО: Разрешить редактирование
        self.model.setReadOnly(False)
        
        # Устанавливаем кастомные иконки
        self.model.setIconProvider(self._create_icon_provider(THEMES[self.window().current_theme]["colors"]['fg']))
        
        self.tree_view.setModel(self.model)
        
        # ВАЖНО: Включить редактирование в tree view
        self.tree_view.setEditTriggers(
            QTreeView.EditTrigger.EditKeyPressed | 
            QTreeView.EditTrigger.SelectedClicked
        )
        
        # Подключаем сигналы модели
        self.model.dataChanged.connect(self._on_data_changed)
        self.model.fileRenamed.connect(self._on_file_renamed)
        
        # Hide columns we don't need
        self.tree_view.setColumnHidden(1, True)  # Size
        self.tree_view.setColumnHidden(2, True)  # Type
        self.tree_view.setColumnHidden(3, True)  # Date Modified
        
        layout.addWidget(self.tree_view)
        
        # Apply theme
        self.update_theme()
    
    def _on_data_changed(self, topLeft, bottomRight, roles):
        """Called when data in the model changes"""
        # Можно добавить дополнительную логику при необходимости
        pass

    def _on_file_renamed(self, path, oldName, newName):
        """Called when a file is renamed"""
        old_path = os.path.join(path, oldName)
        new_path = os.path.join(path, newName)
        
        # If it's a file that's currently open, update the tab
        main_window = self.window()
        print("Start rename")
        if hasattr(main_window, '_find_editor_by_filename'):
            editor = main_window._find_editor_by_filename(old_path)
            if editor:
                editor.filename = new_path
                # Update tab title
                for i in range(main_window.tab_widget.count()):
                    if main_window.tab_widget.widget(i) == editor:
                        main_window.tab_widget.setTabText(i, os.path.basename(new_path))
                        break
            
            # Также обновляем LS процесс если это .lumen файл
            if old_path.endswith('.lumen') and hasattr(main_window, 'ls_processes'):
                if old_path in main_window.ls_processes:
                    proc = main_window.ls_processes.pop(old_path)
                    main_window.ls_processes[new_path] = proc
    
    def _create_icon_provider(self, color):
        """Create custom icon provider for .lumen files"""
        
        class LumenIconProvider(QFileIconProvider):
            def __init__(self, explorer):
                super().__init__()  # QFileIconProvider не принимает аргументы
                self.explorer = explorer
                
            def icon(self, file_info):
                # Получаем путь к файлу
                file_path = file_info.absoluteFilePath()
                
                # Для .lumen файлов используем кастомную иконку
                if file_info.isFile() and file_path.endswith('.lumen'):
                    return self._get_lumen_icon(color)
                elif file_info.isFile() and file_path.endswith('.cpp'):
                    return create_svg_icon(r'data\app\cpp_file_icon.svg', color)
                elif file_info.isFile() and file_path.endswith('.py'):
                    return create_svg_icon(r'data\app\python_file_icon.svg', color)
                elif file_info.isFile() and file_path.endswith('.h'):
                    return create_svg_icon(r'data\app\cpp_file_icon.svg', color)
                elif file_info.isFile() and file_path.endswith('.hpp'):
                    return create_svg_icon(r'data\app\cpp_file_icon.svg', color)
                # Для папок используем стандартную иконку папки
                elif file_info.isDir():
                    return create_svg_icon(r'data\app\folder_icon.svg', color)
                # Для остальных файлов - стандартная иконка файла
                else:
                    return create_svg_icon(r'data\app\file_icon.svg', color)
            
            def _get_lumen_icon(self, color):
                self.lumen_icon = create_svg_icon(APP_ICON_PATH, color)
                return self.lumen_icon
        
        provider = LumenIconProvider(self)
        return provider
    
    def set_root_path(self, path: str):
        """Set the root directory for file explorer"""
        self.current_root = path
        self.model.setRootPath(path)
        self.tree_view.setRootIndex(self.model.index(path))
        
        # Expand the root item
        self.tree_view.expand(self.model.index(path))
        
        # Emit signal that folder is opened
        if hasattr(self.parent(), 'on_folder_opened'):
            self.parent().on_folder_opened(path)
            
    def refresh_tree(self):
        """Refresh the file tree"""
        if self.current_root:
            self.model.setRootPath(self.current_root)
            self.tree_view.setRootIndex(self.model.index(self.current_root))
            
    def collapse_all(self):
        """Collapse all items in the tree"""
        self.tree_view.collapseAll()
        if self.current_root:
            self.tree_view.expand(self.model.index(self.current_root))
            
    def on_item_double_clicked(self, index: QModelIndex):
        """Handle double click on tree item"""
        file_path = self.model.filePath(index)
        if os.path.isfile(file_path):
            # Проверяем расширение файла
            ext = os.path.splitext(file_path)[1].lower()
            if ext in ['.lumen', '.py', '.cpp', '.h', '.hpp', '.c', '.txt']:
                self.file_opened.emit(file_path)
            else:
                reply = QMessageBox.question(
                    self,
                    Strings.get("open_file"),
                    Strings.get("open_non_lumen").format(os.path.basename(file_path)),
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
                )
                if reply == QMessageBox.StandardButton.Yes:
                    self.file_opened.emit(file_path)
                    
    def show_context_menu(self, position):
        """Show context menu for tree items"""
        index = self.tree_view.indexAt(position)
        if not index.isValid():
            return
            
        file_path = self.model.filePath(index)
        menu = RoundedMenu(self)
        
        # Open action
        open_action = menu.addAction(Strings.get("open"))
        open_action.triggered.connect(lambda: self.file_opened.emit(file_path))
        
        menu.addSeparator()
        
        # Reveal in explorer action
        reveal_action = menu.addAction(Strings.get("reveal_in_explorer"))
        reveal_action.triggered.connect(lambda: self.reveal_in_explorer(file_path))
        
        # Copy path action
        copy_path_action = menu.addAction(Strings.get("copy_path"))
        copy_path_action.triggered.connect(lambda: self.copy_to_clipboard(file_path))
        
        if os.path.isfile(file_path):
            menu.addSeparator()
            
            # Rename file action - проверяем права на запись
            rename_action = menu.addAction(Strings.get("rename"))
            rename_action.setEnabled(os.access(os.path.dirname(file_path), os.W_OK))
            rename_action.triggered.connect(lambda: self.rename_file(index))
            
            # Delete file action - проверяем права на запись
            delete_action = menu.addAction(Strings.get("delete"))
            delete_action.setEnabled(os.access(os.path.dirname(file_path), os.W_OK))
            delete_action.triggered.connect(lambda: self.delete_file(file_path))
            
        menu.exec(self.tree_view.viewport().mapToGlobal(position))
        
    def reveal_in_explorer(self, path: str):
        """Reveal file/folder in system explorer"""
        if platform.system() == "Windows":
            os.startfile(os.path.dirname(path) if os.path.isfile(path) else path)
        elif platform.system() == "Darwin":
            subprocess.run(["open", os.path.dirname(path) if os.path.isfile(path) else path])
        else:
            subprocess.run(["xdg-open", os.path.dirname(path) if os.path.isfile(path) else path])
            
    def copy_to_clipboard(self, path: str):
        """Copy path to clipboard"""
        QApplication.clipboard().setText(path)
        QToolTip.showText(
            self.mapToGlobal(self.rect().center()),
            Strings.get("path_copied"),
            self, QRect(), 1000
        )
        
    def delete_file(self, path: str):
        """Delete file with confirmation"""
        reply = QMessageBox.question(
            self,
            Strings.get("confirm_delete"),
            Strings.get("confirm_delete_message").format(os.path.basename(path)),
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        if reply == QMessageBox.StandardButton.Yes:
            try:
                # Check if file is open in editor
                main_window = self.window()
                if hasattr(main_window, '_find_editor_by_filename'):
                    editor = main_window._find_editor_by_filename(path)
                    if editor:
                        # Close the tab first
                        for i in range(main_window.tab_widget.count()):
                            if main_window.tab_widget.widget(i) == editor:
                                main_window.tab_widget.close_tab(i)
                                break
                
                os.remove(path)
                self.refresh_tree()
                self.show_status_message(Strings.get("file_deleted").format(os.path.basename(path)), 2000)
            except Exception as e:
                QMessageBox.critical(self, Strings.get("file_not_found"), Strings.get("delete_error").format(e))
                
    def rename_file(self, index: QModelIndex):
        """Start inline rename of file"""
        # Check if file is writable
        file_path = self.model.filePath(index)
        if os.path.exists(file_path):
            # Проверяем права на запись
            if os.access(os.path.dirname(file_path), os.W_OK):
                self.tree_view.edit(index)
            else:
                QMessageBox.warning(
                    self,
                    "Permission Error",
                    "Cannot rename: No write permission"
                )
        
    def show_status_message(self, message: str, timeout: int):
        """Show status message through parent window"""
        if self.parent() and hasattr(self.parent(), 'show_status_message'):
            self.parent().show_status_message(message, timeout)
            
    def update_theme(self):
        """Update theme colors"""
        colors = THEMES[self.window().current_theme]["colors"]
        
        self.setStyleSheet(f"""
            FileExplorer {{
                background-color: {colors['margin_bg'].name()};
                border-right: none;
            }}
            QTreeView {{
                background-color: {colors['margin_bg'].name()};
                color: {colors['fg'].name()};
                border: none;
                outline: none;
                font-family: Consolas;
                font-size: 10pt;
            }}
            QTreeView::item {{
                padding: 4px 2px;
            }}
            QTreeView::item:hover {{
                background-color: {colors['title_bg'].name()};
            }}
            QTreeView::item:selected {{
                background-color: {colors['selection_bg'].name()};
                color: {colors['selection_fg'].name()};
            }}
            QTreeView::item:selected:!active {{
                background-color: {colors['selection_bg'].name()};
                color: {colors['selection_fg'].name()};
            }}
            QTreeView QLineEdit {{
                background-color: {colors['selection_bg'].name()};
                color: {colors['selection_fg'].name()};
                border: none;
                outline: none;
                padding: 2px;
                margin-top: -2px;
                margin-left: -1px;
                font-family: Consolas;
                font-size: 10pt;
            }}
            QHeaderView::section {{
                background-color: {colors['title_bg'].name()};
                color: {colors['title_fg'].name()};
                padding: 4px;
                border: none;
            }}
        """)
        
        # Обновляем иконки при смене темы
        # Пересоздаем провайдер иконок чтобы обновить цвета
        self.model.setIconProvider(self._create_icon_provider(colors['fg']))

# =============================================================================
# CUSTOM WIDGETS
# =============================================================================

class AnimatedToggle(QToolButton):
    """Custom toggle switch with animated handle"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setCheckable(True)
        self.setFixedSize(32, 14)
        
        self._handle_position = 0.0 if not self.isChecked() else 1.0
        
        self.animation = QPropertyAnimation(self, b"handle_position")
        self.animation.setDuration(200)
        self.animation.setEasingCurve(QEasingCurve.Type.InOutCubic)
        
        self.bg_color = QColor("#3e4a5a")
        self.active_color = QColor("#82aaff")
        self.handle_color = QColor("#ffffff")

    @pyqtProperty(float)
    def handle_position(self):
        return self._handle_position

    @handle_position.setter
    def handle_position(self, pos):
        self._handle_position = pos
        self.update()

    def nextCheckState(self):
        super().nextCheckState()
        self.animation.stop()
        self.animation.setStartValue(self._handle_position)
        self.animation.setEndValue(1.0 if self.isChecked() else 0.0)
        self.animation.start()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        track_rect = QRectF(0, 0, self.width(), self.height())
        r = track_rect.height() / 2
        
        bg_color = self.bg_color
        active_color = self.active_color
        
        r_bg = bg_color.red() + (active_color.red() - bg_color.red()) * self._handle_position
        g_bg = bg_color.green() + (active_color.green() - bg_color.green()) * self._handle_position
        b_bg = bg_color.blue() + (active_color.blue() - bg_color.blue()) * self._handle_position
        
        current_color = QColor(int(r_bg), int(g_bg), int(b_bg))
        
        painter.setBrush(current_color)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(track_rect, r, r)
        
        margin = 3
        handle_size = self.height() - (margin * 2)
        available_width = self.width() - handle_size - (margin * 2)
        x_pos = margin + (available_width * self._handle_position)
        
        painter.setBrush(self.handle_color)
        painter.drawEllipse(QRectF(x_pos, margin, handle_size, handle_size))


# =============================================================================
# WELCOME WIDGET
# =============================================================================

class WelcomeWidget(QWidget):
    """Welcome screen shown when no files are open"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        
        colors = self._get_theme_colors()
        
        gradient = QLinearGradient(0, 0, self.width(), self.height())
        gradient.setColorAt(0, colors['bg'])
        gradient.setColorAt(1, colors['title_bg'])
        painter.fillRect(self.rect(), gradient)
        
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['fg'], 1))
        
        icon = create_svg_icon(APP_ICON_PATH, colors['fg'], 300)
        
        icon_size = 140
        icon_x = (self.width() - 500) // 2 - 150
        icon_y = (self.height() - 100) // 2 - 30
        
        painter.setOpacity(1.0)
        icon_rect = QRect(icon_x, icon_y, icon_size, icon_size)
        painter.drawPixmap(icon_rect, icon.pixmap(icon_size, icon_size))
        
        font = QFont("Segoe UI", 100, QFont.Weight.Light)
        font.setLetterSpacing(QFont.SpacingType.PercentageSpacing, 110)
        painter.setFont(font)
        
        text = Strings.get("welcome_title")
        metrics = painter.fontMetrics()
        text_width = metrics.horizontalAdvance(text)
        text_height = metrics.height()
        
        x = (self.width() - text_width) // 2 + 80
        y = (self.height() - text_height) // 2 + metrics.ascent()
        
        for i in range(5):
            offset = i * 2
            painter.setOpacity(0.1 * (5 - i))
            painter.setPen(QPen(colors['title_fg'], 1))
            painter.drawText(x + offset, y + offset, text)
        
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['title_fg'], 1))
        painter.drawText(x, y, text)
        
        painter.setPen(QPen(colors['title_fg'], 1))
        subtitle_font = QFont("Consolas", 14)
        painter.setFont(subtitle_font)
        
        subtitle = Strings.get("welcome_subtitle")
        metrics = painter.fontMetrics()
        subtitle_width = metrics.horizontalAdvance(subtitle)
        
        subtitle_x = (self.width() - subtitle_width) // 2
        subtitle_y = y + text_height // 2 - 50
        
        painter.drawText(subtitle_x, subtitle_y, subtitle)
        
        painter.end()
            
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


# =============================================================================
# CUSTOM WIDGETS (continued)
# =============================================================================

class AnimatedTitleButton(QToolButton):
    """Base class for title bar buttons with hover animation"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(46, TITLE_BAR_HEIGHT)
        self.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        
        self._opacity = 0.0
        self._target_opacity = 0.0
        self._is_hovered = False
        
        self._animation_timer = QTimer(self)
        self._animation_timer.timeout.connect(self._update_animation)
        self._animation_timer.setInterval(16)
        
    def enterEvent(self, event):
        self._is_hovered = True
        self._target_opacity = 1.0
        self._start_animation()
        super().enterEvent(event)
        
    def leaveEvent(self, event):
        self._is_hovered = False
        self._target_opacity = 0.0
        self._start_animation()
        super().leaveEvent(event)
        
    def _start_animation(self):
        if not self._animation_timer.isActive():
            self._animation_timer.start()
            
    def _update_animation(self):
        step = 0.05
        if abs(self._opacity - self._target_opacity) < step:
            self._opacity = self._target_opacity
            self._animation_timer.stop()
        else:
            self._opacity += step if self._opacity < self._target_opacity else -step
            
        self.update()


class RunButton(AnimatedTitleButton):
    """Run button in title bar with play icon"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        if self._opacity > 0:
            green_color = QColor(colors['autosave_on'])
            green_color.setAlphaF(self._opacity)
            painter.fillRect(self.rect(), green_color)
        
        icon_color = QColor(colors['title_fg']) if not self._is_hovered else QColor(colors['bg'])
        
        center_x = self.width() // 2
        center_y = self.height() // 2
        size = 7
        
        points = [
            QPoint(center_x - size//2, center_y - size + 1),
            QPoint(center_x - size//2, center_y + size - 1),
            QPoint(center_x + size, center_y)
        ]
        
        if self._is_hovered and self._opacity > 0.5:
            painter.setBrush(icon_color)
            painter.setPen(Qt.PenStyle.NoPen)
            painter.drawPolygon(points)
        else:
            painter.setBrush(Qt.BrushStyle.NoBrush)
            painter.setPen(QPen(icon_color, 1))
            painter.drawPolygon(points)
        
        painter.end()
        
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


class CustomTitleBar(QFrame):
    """
    Custom window title bar with integrated menu and window controls.
    """
    
    window_minimized = pyqtSignal()
    window_maximized = pyqtSignal()
    window_closed = pyqtSignal()
    run_clicked = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.parent_window = parent
        self.is_maximized = False
        self.dragging = False
        self.drag_position = QPoint()
        
        self.setFixedHeight(TITLE_BAR_HEIGHT)
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        
        self._setup_ui()
        self.update_style()
        
    def _setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        title_widget = self._create_title_section()
        layout.addWidget(title_widget)
        
        self.menu_container = QWidget()
        self.menu_layout = QHBoxLayout(self.menu_container)
        self.menu_layout.setContentsMargins(5, 0, 0, 0)
        self.menu_layout.setSpacing(0)
        layout.addWidget(self.menu_container)
        
        layout.addStretch()
        
        self.run_button = RunButton(self)
        self.run_button.clicked.connect(self.run_clicked.emit)
        layout.addWidget(self.run_button)
        
        self.minimize_btn = self._create_minimize_button()
        self.maximize_btn = self._create_maximize_button()
        self.close_btn = self._create_close_button()
        
        layout.addWidget(self.minimize_btn)
        layout.addWidget(self.maximize_btn)
        layout.addWidget(self.close_btn)
        
    def _create_title_section(self) -> QWidget:
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(10, 0, 0, 0)
        layout.setSpacing(8)
        
        self.icon_label = QLabel()
        layout.addWidget(self.icon_label)
        
        self.title_label = QLabel(Strings.get("window_title"))
        self.title_label.setStyleSheet("font-size: 14px; font-family: Consolas;")
        layout.addWidget(self.title_label)
        
        return widget
        
    def _create_minimize_button(self) -> QToolButton:
        btn = QToolButton()
        btn.setFixedSize(46, TITLE_BAR_HEIGHT)
        btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        btn.clicked.connect(self.window_minimized.emit)
        btn.paintEvent = lambda e: self._paint_minimize_icon(e, btn)
        return btn
        
    def _create_maximize_button(self) -> QToolButton:
        btn = QToolButton()
        btn.setFixedSize(46, TITLE_BAR_HEIGHT)
        btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        btn.clicked.connect(self.window_maximized.emit)
        btn.paintEvent = lambda e: self._paint_maximize_icon(e, btn)
        return btn
        
    def _create_close_button(self) -> QToolButton:
        btn = QToolButton()
        btn.setFixedSize(46, TITLE_BAR_HEIGHT)
        btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        btn.clicked.connect(self.window_closed.emit)
        btn.paintEvent = lambda e: self._paint_close_icon(e, btn)
        return btn
        
    def _paint_minimize_icon(self, event, btn):
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        if btn.underMouse():
            painter.fillRect(btn.rect(), colors['title_bg_darker'])
        
        painter.setPen(QPen(colors['title_fg'], 1))
        x = btn.width() // 2 - 7
        y = btn.height() // 2
        painter.drawLine(x, y, x + 12, y)
        painter.end()
        
    def _paint_maximize_icon(self, event, btn):
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        if btn.underMouse():
            painter.fillRect(btn.rect(), colors['title_bg_darker'])
        
        painter.setPen(QPen(colors['title_fg'], 1))
        
        if self.is_maximized:
            x1 = btn.width() // 2 - 8 + 2
            y1 = btn.height() // 2 - 2
            painter.drawRect(x1, y1, 8, 8)
            
            x1 = btn.width() // 2 - 5 + 2
            y1 = btn.height() // 2 - 5
            x2 = btn.width() // 2 + 2 + 2
            y2 = btn.height() // 2 - 5
            painter.drawLine(x1, y1, x2, y2)
            
            x1 = btn.width() // 2 + 3 + 2
            y1 = btn.height() // 2 - 5
            x2 = btn.width() // 2 + 3 + 2
            y2 = btn.height() // 2 + 2
            painter.drawLine(x1, y1, x2, y2)
        else:
            x = btn.width() // 2 - 4
            y = btn.height() // 2 - 4
            painter.drawRect(x, y, 8, 8)
            
        painter.end()
        
    def _paint_close_icon(self, event, btn):
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        if btn.underMouse():
            self._draw_close_hover_background(painter, btn)
            painter.setPen(QPen(Qt.GlobalColor.white, 2))
        else:
            painter.setPen(QPen(colors['title_fg'], 1.5))
        
        x1 = btn.width() // 2 - 5
        y1 = btn.height() // 2 - 5
        x2 = btn.width() // 2 + 5
        y2 = btn.height() // 2 + 5
        
        painter.drawLine(x1, y1, x2, y2)
        painter.drawLine(x2, y1, x1, y2)
        painter.end()
        
    def _draw_close_hover_background(self, painter, btn):
        rect = btn.rect()
        painter.setBrush(QColor(196, 43, 28))
        painter.setPen(Qt.PenStyle.NoPen)
        
        radius = 7
    
        if not self.is_maximized:
            path = QPainterPath()
            path.moveTo(rect.left(), rect.top())
            path.lineTo(rect.right() - radius, rect.top())
            path.arcTo(rect.right() - radius * 2 + 1, rect.top(), radius * 2, radius * 2, 90, -90)
            path.lineTo(rect.right() + 1, rect.bottom())
            path.lineTo(rect.left(), rect.bottom())
            path.closeSubpath()
        else:
            path = QPainterPath()
            path.moveTo(rect.left(), rect.top())
            path.lineTo(rect.right() + 1, rect.top())
            path.lineTo(rect.right() + 1, rect.bottom())
            path.lineTo(rect.left(), rect.bottom())
        
        painter.drawPath(path)
        
    def _get_theme_colors(self):
        if hasattr(self.parent_window, 'current_theme'):
            return THEMES[self.parent_window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]
        
    def update_icon(self, icon):
        if icon and not icon.isNull():
            self.icon_label.setPixmap(icon.pixmap(16, 16))
            
    def set_maximized_state(self, maximized: bool):
        self.is_maximized = maximized
        self.maximize_btn.update()
        self.update_style()
        
    def update_style(self):
        colors = self._get_theme_colors()
        is_maximized = self.parent_window.isMaximized() if self.parent_window else False
        border_radius = "7px" if not is_maximized else "0px"
        
        self.setStyleSheet(f"""
            CustomTitleBar {{
                background-color: {colors['title_bg'].name()};
                border-top-left-radius: {border_radius};
                border-top-right-radius: {border_radius};
            }}
            QLabel {{
                color: {colors['title_fg'].name()};
                background-color: transparent;
                font-family: Consolas;
            }}
        """)
        
        for i in range(self.menu_layout.count()):
            btn = self.menu_layout.itemAt(i).widget()
            if isinstance(btn, QToolButton):
                btn.setStyleSheet(f"""
                    QToolButton {{
                        background-color: transparent;
                        color: {colors['title_fg'].name()};
                        border: none;
                        padding: 8px 12px;
                        font-size: 10pt;
                        font-family: Consolas;
                    }}
                    QToolButton:hover {{
                        background-color: {colors['title_bg_darker'].name()};
                        color: {colors['selection_fg'].name()};
                        font-family: Consolas;
                    }}
                    QToolButton::menu-indicator {{
                        image: none;
                    }}
                """)
        
        self.minimize_btn.update()
        self.maximize_btn.update()
        self.close_btn.update()
        self.run_button.update()
        
    def add_menu(self, menu_bar):
        for i in reversed(range(self.menu_layout.count())):
            widget = self.menu_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
        
        for action in menu_bar.actions():
            if action.menu():
                btn = QToolButton()
                btn.setText(action.text())
                btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
                btn.setMenu(action.menu())
                btn.setStyleSheet("""
                    QToolButton::menu-indicator {
                        image: none;
                    }
                    QToolButton {
                        font-family: Consolas;
                    }
                """)
                self.menu_layout.addWidget(btn)
        
        self.update_style()
        
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            if self.parent_window and self.is_maximized:
                super().mousePressEvent(event)
                return
                
            pos = event.pos()
            if not any(btn.geometry().contains(pos) for btn in [
                self.minimize_btn, self.maximize_btn, self.close_btn, self.run_button
            ]):
                self.dragging = True
                self.drag_position = event.globalPosition().toPoint() - self.parent_window.frameGeometry().topLeft()
                event.accept()
                return
        super().mousePressEvent(event)
        
    def mouseMoveEvent(self, event):
        if self.parent_window and self.is_maximized:
            super().mouseMoveEvent(event)
            return
            
        if self.dragging and event.buttons() & Qt.MouseButton.LeftButton:
            self.parent_window.move(event.globalPosition().toPoint() - self.drag_position)
            event.accept()
        else:
            super().mouseMoveEvent(event)
            
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.dragging = False
            event.accept()
            
    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            pos = event.pos()
            if not any(btn.geometry().contains(pos) for btn in [
                self.minimize_btn, self.maximize_btn, self.close_btn, self.run_button
            ]):
                self.window_maximized.emit()
                event.accept()
                self.update_style()


class AboutDialog(QWidget):
    """Custom About dialog window"""
    
    def __init__(self, parent=None):
        super().__init__(None, Qt.WindowType.Window | Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        self.setWindowFlags(self.windowFlags() | Qt.WindowType.NoDropShadowWindowHint)
        self.parent_window = parent
        self.setFixedSize(600, 400)
        self._apply_rounded_mask()
        
    def _apply_rounded_mask(self):
        radius = 10
        path = QPainterPath()
        rect = self.rect()
        path.addRoundedRect(float(rect.x()), float(rect.y()), float(rect.width()), float(rect.height()), radius, radius)
        region = QRegion(path.toFillPolygon().toPolygon())
        self.setMask(region)
        
    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._apply_rounded_mask()
        
    def mousePressEvent(self, event):
        self.close()
        super().mousePressEvent(event)
        
    def focusOutEvent(self, event):
        QTimer.singleShot(100, self.close)
        super().focusOutEvent(event)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        gradient = QLinearGradient(0, 0, self.width(), self.height())
        gradient.setColorAt(0, colors['bg'])
        gradient.setColorAt(1, colors['title_bg'])
        painter.fillRect(self.rect(), colors['bg'])
        
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.setPen(QPen(colors['status_border'], 1))
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 10, 10)
        
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['fg'], 1))
        
        font = QFont("Segoe UI", 60, QFont.Weight.Light)
        font.setLetterSpacing(QFont.SpacingType.PercentageSpacing, 110)
        painter.setFont(font)
        
        text = Strings.get("welcome_title")
        metrics = painter.fontMetrics()
        text_width = metrics.horizontalAdvance(text)
        text_height = metrics.height()
        
        x = (self.width() - text_width) // 2
        y = (self.height() - text_height) // 2 + metrics.ascent() - 70
        
        for i in range(5):
            offset = i * 2
            painter.setOpacity(0.1 * (5 - i))
            painter.setPen(QPen(colors['title_fg'], 1))
            painter.drawText(x + offset, y + offset, text)
        
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['title_fg'], 1))
        painter.drawText(x, y, text)
        
        painter.setPen(QPen(colors['title_fg'], 1))
        subtitle_font = QFont("Consolas", 11)
        painter.setFont(subtitle_font)
        x = (self.width() - text_width) // 2
        y = (self.height() - text_height) // 2 + metrics.ascent() - 50
        subtitle = Strings.get("welcome_subtitle")
        metrics = painter.fontMetrics()
        subtitle_width = metrics.horizontalAdvance(subtitle)
        
        subtitle_x = (self.width() - subtitle_width) // 2
        subtitle_y = y + text_height // 2 - 40
        
        painter.drawText(subtitle_x, subtitle_y, subtitle)
        
        author_font = QFont("Consolas", 10)
        painter.setFont(author_font)
        painter.setPen(QPen(colors['title_fg'], 1))
        painter.setOpacity(0.8)
        
        author_text = Strings.get("author_text")
        metrics = painter.fontMetrics()
        author_x = 10
        author_y = self.height() - 10

        painter.drawText(author_x, author_y, author_text)
        
        version_text = Strings.get("version")
        version_width = metrics.horizontalAdvance(version_text)
        painter.drawText(self.width() - version_width - 10, author_y, version_text)
        
        painter.end()
        
    def _get_theme_colors(self):
        if self.parent_window and hasattr(self.parent_window, 'current_theme'):
            return THEMES[self.parent_window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


class ErrorTooltip(QWidget):
    """Custom tooltip for error messages with rounded corners and theme support"""
    
    def __init__(self, parent=None):
        super().__init__(None, Qt.WindowType.ToolTip | Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setAttribute(Qt.WidgetAttribute.WA_ShowWithoutActivating)
        self.parent_window = parent
        
        self._setup_ui()
        self._current_colors = None
        self._last_error_type = ErrorType.ERROR
        
    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 6, 10, 6)
        layout.setSpacing(0)
        
        self.label = QLabel()
        self.label.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        self.label.setWordWrap(False)
        self.label.setFont(QFont("Consolas", 10))
        layout.addWidget(self.label)
        
    def set_text(self, text: str, colors: dict, error_type: ErrorType = ErrorType.ERROR):
        self._current_colors = colors
        self._last_error_type = error_type
        self.label.setText(text)
        self.label.setStyleSheet(f"color: {colors['fg'].name()}; background: transparent;")
        
        if error_type == ErrorType.ERROR:
            accent = colors.get('error', QColor(255, 80, 80))
        elif error_type == ErrorType.WARNING:
            accent = colors.get('warning', QColor(255, 200, 80))
        else:
            accent = colors.get('echo', QColor(80, 200, 255))
        self.label.setStyleSheet(f"color: {accent.name()}; background: transparent;")
        self._accent_color = accent
        
        self.label.adjustSize()
        self.adjustSize()
        self.update()
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        if self._current_colors is not None:
            bg_color = QColor(self._current_colors['bg'])
            bg_color.setAlpha(200)
            
            painter.setBrush(bg_color)
            painter.setPen(QPen(self._accent_color, 1))
            painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 6, 6)
        else:
            painter.setBrush(QColor('#1e1e2e'))
            painter.setPen(QPen(QColor('#313244'), 1))
            painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 6, 6)
        
        super().paintEvent(event)


class RoundedMenu(QMenu):
    """Custom menu with rounded corners and hover tracking"""
    
    hovered_action = pyqtSignal(QAction)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.setWindowFlags(
            Qt.WindowType.Popup | 
            Qt.WindowType.FramelessWindowHint | 
            Qt.WindowType.NoDropShadowWindowHint
        )
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        
        consolas_font = QFont("Consolas", 11)
        self.setFont(consolas_font)
        
        self._current_hovered_action = None
        self._hover_timer = QTimer(self)
        self._hover_timer.setInterval(50)
        self._hover_timer.timeout.connect(self._check_hover)
        self._hover_timer.start()
        
    def _check_hover(self):
        if not self.isVisible():
            return
            
        from PyQt6.QtGui import QCursor
        pos = self.mapFromGlobal(QCursor.pos())
        action = self.actionAt(pos)
        
        if action != self._current_hovered_action:
            self._current_hovered_action = action
            self.hovered_action.emit(action)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        rect = self.rect()
        
        painter.setBrush(QColor(colors['bg']))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 8, 8)
        
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, False)
        painter.setPen(QPen(QColor(colors['status_border']), 1, 
                            Qt.PenStyle.SolidLine, 
                            Qt.PenCapStyle.RoundCap, 
                            Qt.PenJoinStyle.RoundJoin))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 7, 7)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        
        super().paintEvent(event)
        
    def hideEvent(self, event):
        self._current_hovered_action = None
        self.hovered_action.emit(None)
        super().hideEvent(event)
        
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


class EditorTabWidget(QTabWidget):
    """Custom tab widget with close buttons on each tab"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTabsClosable(False)
        self.setMovable(True)
        self.tabBar().tabMoved.connect(self._on_tab_moved)
        
    def addTab(self, widget, title):
        index = super().addTab(widget, title)
        self._add_close_button(index)
        self._update_movable_state()
        return index
        
    def insertTab(self, index, widget, title):
        index = super().insertTab(index, widget, title)
        self._add_close_button(index)
        self._update_movable_state()
        return index
        
    def removeTab(self, index):
        super().removeTab(index)
        self._update_movable_state()
        
    def _update_movable_state(self):
        self.setMovable(self.count() > 1)
        
    def _on_tab_moved(self, from_index, to_index):
        for i in range(self.count()):
            self._update_close_button(i)
            
    def _add_close_button(self, index):
        self._update_close_button(index)
        
    def _update_close_button(self, index):
        tab_bar = self.tabBar()
        old_btn = tab_bar.tabButton(index, QTabBar.ButtonPosition.RightSide)
        if old_btn:
            old_btn.deleteLater()
        
        btn = QToolButton(tab_bar)
        btn.setIcon(self._create_close_icon())
        btn.setIconSize(QSize(14, 14))
        
        colors = self._get_theme_colors()
        
        btn.setStyleSheet(f"""
            QToolButton {{
                background-color: transparent;
                border: none;
                border-radius: 4px;
                padding: 0px;
                margin-right: 4px;
                font-family: Consolas;
            }}
            QToolButton:hover {{
                background-color: {colors['selection_bg'].name()};
                border-radius: 4px;
            }}
            QToolButton:pressed {{
                background-color: {colors['title_border'].name()};
            }}
        """)
        
        btn.clicked.connect(lambda checked, idx=index: self._on_close_clicked(idx))
        tab_bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, btn)
        
    def _create_close_icon(self) -> QIcon:
        pixmap = QPixmap(14, 14)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        
        colors = self._get_theme_colors()
        painter.setPen(QPen(colors["title_fg"], 1.5))
        
        painter.drawLine(4, 4, 10, 10)
        painter.drawLine(10, 4, 4, 10)
        painter.end()
        
        return QIcon(pixmap)
        
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]
        
    def update_all_close_buttons(self):
        for i in range(self.count()):
            self._update_close_button(i)
        
    def _on_close_clicked(self, index):
        if index < self.count():
            self.close_tab(index)
        else:
            sender = self.sender()
            if sender:
                tab_bar = self.tabBar()
                for i in range(self.count()):
                    if tab_bar.tabButton(i, QTabBar.ButtonPosition.RightSide) == sender:
                        self.close_tab(i)
                        break
                        
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
                    Strings.get("save_changes"),
                    Strings.get("save_changes_question").format(name),
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No | QMessageBox.StandardButton.Cancel
                )
                if reply == QMessageBox.StandardButton.Cancel:
                    return
                elif reply == QMessageBox.StandardButton.Yes:
                    main_window = self.window()
                    if hasattr(main_window, 'save_editor'):
                        if filename:
                            main_window.save_editor(editor, filename)
                        else:
                            main_window.save_editor_as(editor)
        
        self.removeTab(index)
        widget.deleteLater()
        
        if self.count() == 0:
            main_window = self.window()
            if hasattr(main_window, '_update_content_display'):
                main_window._update_content_display()


# =============================================================================
# LEXER
# =============================================================================

class TwistLangLexer(QsciLexerCustom):
    """
    Custom lexer for TwistLang syntax highlighting.
    """
    
    STYLE_DEFAULT = 0
    STYLE_KEYWORD = 1
    STYLE_TYPE = 2
    STYLE_COMMENT = 3
    STYLE_STRING = 4
    STYLE_NUMBER = 5
    STYLE_OPERATOR = 6
    STYLE_FUNCTION = 7
    STYLE_MODIFIER = 8
    STYLE_DIRECTIVE = 9
    STYLE_LITERAL = 10
    STYLE_NAMESPACE_ID = 11
    STYLE_SPECIAL = 12
    STYLE_OBJECT = 13
    STYLE_ESCAPE = 14
    
    def __init__(self, parent=None, theme_name: str = DEFAULT_THEME, font_size: int = DEFAULT_FONT_SIZE):
        super().__init__(parent)
        self.theme_name = theme_name
        self.font_size = font_size
        
        self.keywords = {
            'if', 'else', 'for', 'while', 'let', 'in', 'and', 'or', 'echo',
            'ret', 'assert', 'lambda', 'do',
            'struct', 'namespace', 'func', 'continue;', 'break;'
        }
        self.modifiers = {'const', 'static', 'global', 'final', 'private'}
        self.types = {'Int', 'Bool', 'String', 'Char', 'Null', 'Double',
                      'Namespace', 'Func', 'Lambda', 'auto', "Type"}
        self.literals = {'true', 'false', 'null', 'self'}
        self.directives = {'#define', '#macro', '#include'}
        self.special_keywords = {'new', 'del', 'typeof', 'sizeof', 'out', 'outln', 'input', 'exit'}
        
        self.setup_styles()
        
    def wordCharacters(self) -> str:
        return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"
        
    def language(self) -> str:
        return "TwistLang"
        
    def description(self, style: int) -> str:
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
            self.STYLE_OBJECT: "Object",
            self.STYLE_ESCAPE: "Escape sequence"
        }
        return descriptions.get(style, "")
        
    def set_theme(self, theme_name: str):
        self.theme_name = theme_name
        self.setup_styles()
        
    def set_font_size(self, size: int):
        self.font_size = size
        self.setup_styles()
        
    def setup_styles(self):
        theme = THEMES[self.theme_name]
        colors = theme["colors"]
        
        self.setDefaultPaper(colors["bg"])
        self.setDefaultColor(colors["fg"])
        
        safe_font = get_safe_monospace_font("Consolas", self.font_size)
        self.setFont(safe_font)
        
        style_colors = {
            self.STYLE_KEYWORD: colors["keyword"],
            self.STYLE_TYPE: colors["type"],
            self.STYLE_COMMENT: colors["comment"],
            self.STYLE_STRING: colors["string"],
            self.STYLE_NUMBER: colors["number"],
            self.STYLE_OPERATOR: colors["operator"],
            self.STYLE_FUNCTION: colors["function"],
            self.STYLE_MODIFIER: colors["modifier"],
            self.STYLE_DIRECTIVE: colors["directive"],
            self.STYLE_LITERAL: colors["literal"],
            self.STYLE_NAMESPACE_ID: colors["namespace"],
            self.STYLE_SPECIAL: colors["special"],
            self.STYLE_OBJECT: colors["object"],
            self.STYLE_ESCAPE: colors["function"],
        }
        
        for style, color in style_colors.items():
            self.setColor(color, style)
            
        italic_font = get_safe_monospace_font("Consolas", self.font_size)
        italic_font.setItalic(True)
        self.setFont(italic_font, self.STYLE_COMMENT)
        
        bold_font = get_safe_monospace_font("Consolas", self.font_size)
        bold_font.setBold(True)
        self.setFont(bold_font, self.STYLE_KEYWORD)

        special_font = get_safe_monospace_font("Consolas", self.font_size)
        special_font.setItalic(True)
        special_font.setUnderline(True)
        self.setFont(special_font, self.STYLE_MODIFIER)

    def styleText(self, start: int, end: int):
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
        
        while pos < end:
            ch, ch_len = self._get_char_at(text_bytes, pos, total_bytes)
            if ch_len == 0:
                break
                
            if ch == '/' and pos + ch_len < end:
                next_ch, next_len = self._get_char_at(text_bytes, pos + ch_len, total_bytes)
                if next_ch == '/':
                    expecting_namespace = False
                    j = pos
                    while j < end:
                        c, l = self._get_char_at(text_bytes, j, total_bytes)
                        if c == '\n':
                            j += l
                            break
                        j += l
                    self.setStyling(j - pos, self.STYLE_COMMENT)
                    pos = j
                    continue
                    
            if ch in ('"', "'"):
                start_pos = pos
                quote_char = ch
                j = pos + ch_len
                escaped = False
                while j < end:
                    c, l = self._get_char_at(text_bytes, j, total_bytes)
                    if c == '\\':
                        escaped = not escaped
                    elif c == quote_char and not escaped:
                        j += l
                        break
                    else:
                        escaped = False
                    j += l
                self.startStyling(start_pos)
                self.setStyling(ch_len, self.STYLE_STRING)
                interior_start = start_pos + ch_len
                interior_end = j - l
                i = interior_start
                while i < interior_end:
                    c, lc = self._get_char_at(text_bytes, i, total_bytes)
                    if c == '\\' and i + lc < interior_end:
                        next_i = i + lc
                        if next_i < interior_end:
                            self.startStyling(i)
                            self.setStyling(lc + (self._get_char_at(text_bytes, next_i, total_bytes)[1]), self.STYLE_ESCAPE)
                            i = next_i + (self._get_char_at(text_bytes, next_i, total_bytes)[1])
                        else:
                            self.startStyling(i)
                            self.setStyling(lc, self.STYLE_STRING)
                            i += lc
                    else:
                        self.startStyling(i)
                        self.setStyling(lc, self.STYLE_STRING)
                        i += lc
                self.startStyling(j - l)
                self.setStyling(l, self.STYLE_STRING)
                pos = j
                continue
                
            if ch.isdigit() or (ch == '.' and pos + ch_len < end and 
                                self._get_char_at(text_bytes, pos + ch_len, total_bytes)[0].isdigit()):
                expecting_namespace = False
                j = pos
                while j < end:
                    c, l = self._get_char_at(text_bytes, j, total_bytes)
                    if not (c.isdigit() or c == '.'):
                        break
                    if c == '.':
                        if any(cc == '.' for cc in text_bytes[pos:j].decode('utf-8')):
                            break
                    j += l
                self.setStyling(j - pos, self.STYLE_NUMBER)
                pos = j
                continue
                
            if ch.isalpha() or ch == '_' or ch == '#':
                j = pos
                while j < end:
                    c, l = self._get_char_at(text_bytes, j, total_bytes)
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
                    elif j < end and self._get_char_at(text_bytes, j, total_bytes)[0] == '(':
                        style = self.STYLE_FUNCTION
                    elif j < end and self._get_char_at(text_bytes, j, total_bytes)[0] == '.':
                        style = self.STYLE_OBJECT
                        
                self.setStyling(j - pos, style)
                pos = j
                continue
                
            if ch in '+-*/%=&|^!<>~,?.:;(){}[]':
                expecting_namespace = False
                j = pos + ch_len
                
                if ch == '*' and j <= end:
                    temp_j = pos
                    while temp_j < end:
                        c, l = self._get_char_at(text_bytes, temp_j, total_bytes)
                        if c == '*' or c.isspace():
                            temp_j += l
                        else:
                            break
                    
                    if text_bytes[temp_j:temp_j+4].decode('utf-8', errors='ignore') == "auto":
                        next_c, next_l = self._get_char_at(text_bytes, temp_j+4, total_bytes)
                        try:
                            if not next_c.isalnum() and next_c != '_':
                                self.setStyling((temp_j + 4) - pos, self.STYLE_TYPE)
                                pos = temp_j + 4
                                continue
                        except: pass

                if j < end:
                    next_ch, next_len = self._get_char_at(text_bytes, j, total_bytes)
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
        self.update_folding_levels(start, end)
        
    def update_folding_levels(self, start, end):
        editor = self.editor()
        if not editor:
            return

        start_line = editor.SendScintilla(editor.SCI_LINEFROMPOSITION, start)
        end_line = editor.SendScintilla(editor.SCI_LINEFROMPOSITION, end)
        total_lines = editor.lines()

        current_level = 1024
        if start_line > 0:
            prev_level = editor.SendScintilla(editor.SCI_GETFOLDLEVEL, start_line - 1)
            current_level = prev_level & 0x0FFF
            if prev_level & 0x2000:
                current_level += 1

        for line in range(start_line, total_lines):
            text = editor.text(line)
            open_count = text.count('{')
            close_count = text.count('}')

            is_header = 0
            if open_count > 0 and (open_count > close_count or text.find('{') < text.find('}') or close_count == 0):
                is_header = 0x2000

            new_fold_level = current_level | is_header
            old_fold_level = editor.SendScintilla(editor.SCI_GETFOLDLEVEL, line)

            if old_fold_level == new_fold_level and line > end_line:
                break

            editor.SendScintilla(editor.SCI_SETFOLDLEVEL, line, new_fold_level)

            current_level += open_count
            current_level -= close_count
            if current_level < 1024:
                current_level = 1024
        
            
    def _get_char_at(self, text_bytes: bytes, pos: int, total_bytes: int) -> Tuple[Optional[str], int]:
        if pos >= total_bytes:
            return (None, 0)
            
        b = text_bytes[pos]
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
            
        if pos + length > total_bytes:
            length = 1
            
        try:
            ch = text_bytes[pos:pos+length].decode('utf-8')
        except UnicodeDecodeError:
            ch = '\ufffd'
            
        return (ch, length)


class WindowBorderOverlay(QWidget):
    """Прозрачный оверлей для рисования обводки окна"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WidgetAttribute.WA_TransparentForMouseEvents)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        
        main_window = self.parent()
        if hasattr(main_window, 'current_theme'):
            colors = THEMES[main_window.current_theme]["colors"]
            border_color = colors.get('status_border', QColor('#FF0000')).lighter(170)
        else:
            border_color = QColor('#FF0000')
        
        is_maximized = main_window.isMaximized()
        radius = 8 if not is_maximized else 0
        
        if not is_maximized:
            pen = QPen(border_color, 1)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            
            rect = self.rect().adjusted(0, 0, 0, 0)
            painter.drawRoundedRect(rect, radius, radius)


# =============================================================================
# EDITOR
# =============================================================================

@dataclass
class ErrorInfo:
    line: int
    start_col: int
    end_col: int
    message: str
    type: ErrorType


class CustomScintilla(QsciScintilla):
    """
    Enhanced Scintilla editor.
    """
    
    goto_definition_requested = pyqtSignal(str, int)
    
    INDICATOR_ERROR = 8
    WARNING_ERROR = 9

    ERROR_LINE_MARKER = 10
    WARNING_LINE_MARKER = 11
    ECHO_LINE_MARKER = 12
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.main_window = None
        self.filename = None
        self.last_save_time = datetime.now()
        
        self.ctrl_pressed = False
        
        self.errors: List[ErrorInfo] = []
        self.error_lines: Set[int] = set()
        self.error_messages: Dict[int, str] = {}
        
        self.error_text_visible = True
        
        self.setMouseTracking(True)
        self._setup_editor()
        self._setup_error_highlighting()
        self.cursorPositionChanged.connect(self.highlightMatchingBrace)
        self.textChanged.connect(self._on_text_changed)

        self.scroll_animation = QPropertyAnimation(self.verticalScrollBar(), b"value")
        self.scroll_animation.setDuration(250)
        self.scroll_animation.setEasingCurve(QEasingCurve.Type.OutCubic)
        
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        
        self.error_tooltip = ErrorTooltip(self)
        self.error_tooltip.hide()

   
    
    # ... существующий код до метода setup_lexer_for_file ...
    
    def setup_lexer_for_file(self, filename: str = None):
        """Setup appropriate lexer based on file extension"""
        if filename:
            ext = os.path.splitext(filename)[1].lower()
            
            if ext == '.py':
                lexer = QsciLexerPython(self)
                self._setup_python_lexer(lexer)
                return lexer
            elif ext in ['.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.hxx']:
                lexer = QsciLexerCPP(self)
                self._setup_cpp_lexer(lexer)
                return lexer
        
        # Default to TwistLang lexer
        lexer = TwistLangLexer(self, self.main_window.current_theme if self.main_window else DEFAULT_THEME, 
                              self.main_window.global_font_size if self.main_window else DEFAULT_FONT_SIZE)
        return lexer
    
    def _setup_python_lexer(self, lexer: QsciLexerPython):
        """Configure Python lexer with current theme colors"""
        if not self.main_window:
            return
        
        colors = THEMES[self.main_window.current_theme]["colors"]
        font_size = self.main_window.global_font_size
        font = get_safe_monospace_font("Consolas", font_size)
        
        lexer.setDefaultPaper(colors["bg"])
        lexer.setDefaultColor(colors["fg"])
        lexer.setFont(font)
        
        # Python-specific styles
        lexer.setColor(colors["comment"], QsciLexerPython.Comment)
        lexer.setColor(colors["number"], QsciLexerPython.Number)
        lexer.setColor(colors["keyword"], QsciLexerPython.Keyword)
        
        # Strings
        lexer.setColor(colors["string"], QsciLexerPython.DoubleQuotedString)
        lexer.setColor(colors["string"], QsciLexerPython.SingleQuotedString)
        lexer.setColor(colors["string"], QsciLexerPython.TripleDoubleQuotedString)
        lexer.setColor(colors["string"], QsciLexerPython.TripleSingleQuotedString)
        lexer.setColor(colors["string"], QsciLexerPython.UnclosedString)
        
        # Classes and functions
        lexer.setColor(colors["type"], QsciLexerPython.ClassName)
        lexer.setColor(colors["function"], QsciLexerPython.FunctionMethodName)
        
        # Operators
        lexer.setColor(colors["operator"], QsciLexerPython.Operator)
        
        # Identifiers
        lexer.setColor(colors["fg"], QsciLexerPython.Identifier)
        
        # Decorators
        lexer.setColor(colors["directive"], QsciLexerPython.Decorator)
        
        # Highlighted identifiers (like self)
        lexer.setColor(colors["special"], QsciLexerPython.HighlightedIdentifier)
          
        
        # Set comment font to italic
        italic_font = QFont(font)
        italic_font.setItalic(True)
        lexer.setFont(italic_font, QsciLexerPython.Comment)
        
        # Set keyword font to bold
        bold_font = QFont(font)
        bold_font.setBold(True)
        lexer.setFont(bold_font, QsciLexerPython.Keyword)
        lexer.setFont(bold_font, QsciLexerPython.ClassName)
        lexer.setFont(bold_font, QsciLexerPython.FunctionMethodName)
        
        # Set decorator font
        decorator_font = QFont(font)
        decorator_font.setItalic(True)
        lexer.setFont(decorator_font, QsciLexerPython.Decorator)
        
    def _setup_cpp_lexer(self, lexer: QsciLexerCPP):
        """Configure C++ lexer with current theme colors"""
        if not self.main_window:
            return
        
        colors = THEMES[self.main_window.current_theme]["colors"]
        font_size = self.main_window.global_font_size
        font = get_safe_monospace_font("Consolas", font_size)
        
        lexer.setDefaultPaper(colors["bg"])
        lexer.setDefaultColor(colors["fg"])
        lexer.setFont(font)
        
        # C++-specific styles with theme colors
        # Comments
        lexer.setColor(colors["comment"], QsciLexerCPP.Comment)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentLine)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentDoc)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentDocKeyword)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentDocKeywordError)
        
        # Numbers
        lexer.setColor(colors["number"], QsciLexerCPP.Number)
        
        # Keywords
        lexer.setColor(colors["keyword"], QsciLexerCPP.Keyword)
        lexer.setColor(colors["keyword"], QsciLexerCPP.KeywordSet2)  # Secondary keywords
        
        # Strings
        lexer.setColor(colors["string"], QsciLexerCPP.DoubleQuotedString)
        lexer.setColor(colors["string"], QsciLexerCPP.SingleQuotedString)
        lexer.setColor(colors["string"], QsciLexerCPP.RawString)
        
        # Preprocessor directives
        lexer.setColor(colors["directive"], QsciLexerCPP.PreProcessor)
        lexer.setColor(colors["comment"], QsciLexerCPP.PreProcessorCommentLineDoc)
        
        # Operators
        lexer.setColor(colors["operator"], QsciLexerCPP.Operator)
        
        # Types and classes
        lexer.setColor(colors["type"], QsciLexerCPP.KeywordSet2)
        lexer.setColor(colors["type"], QsciLexerCPP.GlobalClass)
        
        # Functions
        lexer.setColor(colors["function"], QsciLexerCPP.GlobalClass)
        
        # Identifiers
        lexer.setColor(colors["fg"], QsciLexerCPP.Identifier)
        
        # Unclosed string
        lexer.setColor(colors["error"], QsciLexerCPP.UnclosedString)
        
        # Verbatim string
        lexer.setColor(colors["string"], QsciLexerCPP.VerbatimString)
        
        # Regex
        lexer.setColor(colors["string"], QsciLexerCPP.Regex)
        
        # UUID
        lexer.setColor(colors["number"], QsciLexerCPP.UUID)
        
        # Triple quoted strings
        lexer.setColor(colors["string"], QsciLexerCPP.TripleQuotedVerbatimString)

        
        # Escape sequences in strings
        lexer.setColor(colors["special"], QsciLexerCPP.EscapeSequence)
        

        
        # Documentation comments
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentLineDoc)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentDocKeyword)
        lexer.setColor(colors["comment"], QsciLexerCPP.CommentDocKeywordError)
        
        # Inactive code (inside #if 0)
        inactive_color = QColor(colors["comment"])
        inactive_color.setAlpha(150)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveComment)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveCommentLine)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveCommentDoc)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveNumber)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveKeyword)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveKeywordSet2)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveDoubleQuotedString)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveSingleQuotedString)
        lexer.setColor(inactive_color, QsciLexerCPP.InactivePreProcessor)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveOperator)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveIdentifier)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveGlobalClass)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveUnclosedString)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveVerbatimString)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveRegex)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveUUID)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveEscapeSequence)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveRawString)
        lexer.setColor(inactive_color, QsciLexerCPP.InactiveTripleQuotedVerbatimString)

        
        # Task markers (TODO, FIXME, etc.)
        lexer.setColor(colors["warning"], QsciLexerCPP.TaskMarker)
        
        # User defined literals
        lexer.setColor(colors["literal"], QsciLexerCPP.UserLiteral)
        
        # Raw string literals
        lexer.setColor(colors["string"], QsciLexerCPP.RawString)
        
        # Set fonts
        italic_font = QFont(font)
        italic_font.setItalic(True)
        lexer.setFont(italic_font, QsciLexerCPP.Comment)
        lexer.setFont(italic_font, QsciLexerCPP.CommentLine)
        lexer.setFont(italic_font, QsciLexerCPP.CommentDoc)
        lexer.setFont(italic_font, QsciLexerCPP.CommentLineDoc)
        lexer.setFont(italic_font, QsciLexerCPP.CommentDocKeyword)
        lexer.setFont(italic_font, QsciLexerCPP.CommentDocKeywordError)
        
        bold_font = QFont(font)
        bold_font.setBold(True)
        lexer.setFont(bold_font, QsciLexerCPP.Keyword)
        lexer.setFont(bold_font, QsciLexerCPP.KeywordSet2)
        lexer.setFont(bold_font, QsciLexerCPP.GlobalClass)
        
        # Preprocessor font
        preproc_font = QFont(font)
        preproc_font.setBold(True)
        lexer.setFont(preproc_font, QsciLexerCPP.PreProcessor)

    def _show_context_menu(self, pos: QPoint):
        menu = RoundedMenu(self)
        
        cut_action = menu.addAction(Strings.get("context_cut"))
        cut_action.triggered.connect(self.cut)
        cut_action.setEnabled(self.hasSelectedText())
        
        copy_action = menu.addAction(Strings.get("context_copy"))
        copy_action.triggered.connect(self.copy)
        copy_action.setEnabled(self.hasSelectedText())
        
        paste_action = menu.addAction(Strings.get("context_paste"))
        paste_action.triggered.connect(self.paste)
        
        menu.addSeparator()
        
        select_all_action = menu.addAction(Strings.get("context_select_all"))
        select_all_action.triggered.connect(lambda: self.selectAll())
        
        menu.addSeparator()
        
        line, _ = self.lineIndexFromPosition(self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y()))
        if line >= 0 and self.is_include_line(line):
            go_to_action = menu.addAction(Strings.get("context_go_to_include"))
            go_to_action.triggered.connect(lambda: self._goto_include_at_line(line))
        
        clear_errors_action = menu.addAction(Strings.get("context_clear_errors"))
        clear_errors_action.triggered.connect(self.clear_errors)
        
        menu.exec(self.mapToGlobal(pos))
        
    def _goto_include_at_line(self, line: int):
        path = self.extract_include_path(line)
        if path:
            self.goto_definition_requested.emit(path, line)
        
    def set_folding_visible(self, visible: bool):
        if visible:
            self.setFolding(QsciScintilla.FoldStyle.CircledTreeFoldStyle)
            self.setMarginWidth(2, 14)
        else:
            self.setFolding(QsciScintilla.FoldStyle.NoFoldStyle)
            self.setMarginWidth(2, 0)
    
    def _on_text_changed(self):
        lexer = self.lexer()
        if lexer and hasattr(lexer, 'update_folding_levels'):
            lexer.update_folding_levels(0, self.length())

    def highlightMatchingBrace(self):
        pos = self.SendScintilla(self.SCI_GETCURRENTPOS)
        
        self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
        self.SendScintilla(self.SCI_INDICATORCLEARRANGE, 0, self.length())
        
        self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
        self.SendScintilla(self.SCI_INDICATORCLEARRANGE, 0, self.length())
        
        try:
            byte_val = self.SendScintilla(self.SCI_GETCHARAT, pos)
            char = chr(byte_val) if 0 <= byte_val < 128 else ''
        except (ValueError, OverflowError):
            char = ''
        braces = {'(': ')', ')': '(', '[': ']', ']': '[', '{': '}', '}': '{', '<': '>', '>': '<'}
        
        if char in braces:
            brace_pos = self.SendScintilla(self.SCI_BRACEMATCH, pos, 0)
            
            if brace_pos != -1:
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, pos, 1)
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, pos, 1)
                
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, brace_pos, 1)
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, brace_pos, 1)
        
    def _setup_editor(self):
        self.setUtf8(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setScrollWidthTracking(True)
        self.setScrollWidth(1)
        self.setCaretLineVisible(True)
        
        self.setMarginType(0, QsciScintilla.MarginType.NumberMargin)
        self.setBraceMatching(QsciScintilla.BraceMatch.SloppyBraceMatch)
        self.setIndentationsUseTabs(False)
        self.setTabWidth(4)
        self.setIndentationGuides(True)
        
        self.setAutoCompletionSource(QsciScintilla.AutoCompletionSource.AcsAPIs)
        self.setAutoCompletionThreshold(1)
        self.setAutoCompletionCaseSensitivity(False)
        self.setAutoCompletionReplaceWord(False)

        self.setFolding(QsciScintilla.FoldStyle.CircledTreeFoldStyle)
        self.SendScintilla(self.SCI_SETAUTOMATICFOLD, 0x0001 | 0x0002 | 0x0004)
        
    def _setup_error_highlighting(self):
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.INDICATOR_ERROR
        )
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.WARNING_ERROR
        )
        
        self.markerDefine(QsciScintilla.MarkerSymbol.Background, self.ERROR_LINE_MARKER)
        self.markerDefine(QsciScintilla.MarkerSymbol.Background, self.WARNING_LINE_MARKER)
        self.markerDefine(QsciScintilla.MarkerSymbol.Background, self.ECHO_LINE_MARKER)
        
    def set_main_window(self, window):
        self.main_window = window
        
    def set_error_text_visible(self, visible: bool):
        self.error_text_visible = visible
        self.viewport().update()
        
    def set_errors(self, errors: List[tuple]):
        self.clear_errors()
        for error in errors:
            self.add_error(*error)
        self.viewport().update()
        
    def clear_errors(self):
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.INDICATOR_ERROR)
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.WARNING_ERROR)

        self.markerDeleteAll(self.ERROR_LINE_MARKER)
        self.markerDeleteAll(self.WARNING_LINE_MARKER)
        self.markerDeleteAll(self.ECHO_LINE_MARKER)
        
        self.errors.clear()
        self.error_lines.clear()
        self.error_messages.clear()
        self.viewport().update()
        
    def add_error(self, line: int, start_col: int, end_col: int, message: str, error_type: int):
        start_col += 1
        end_col += 1
        
        if not isinstance(message, str):
            message = str(message)
        
        if (error_type in [0, 1]):
            self.fillIndicatorRange(line, start_col, line, end_col,
                                self.INDICATOR_ERROR if error_type == 0 else self.WARNING_ERROR)
            
        if error_type == 0:
            if (self.error_text_visible): self.markerAdd(line, self.ERROR_LINE_MARKER)
            self.error_messages[line] = f"Error: {message}"
        elif error_type == 1:
            if (self.error_text_visible): self.markerAdd(line, self.WARNING_LINE_MARKER)
            self.error_messages[line] = f"Warning: {message}"
        elif error_type == 2:
            if (self.error_text_visible): self.markerAdd(line, self.ECHO_LINE_MARKER)
            self.error_messages[line] = message
            
        self.error_lines.add(line)
        self.errors.append(ErrorInfo(line, start_col, end_col, message, ErrorType(error_type)))
        
    def get_line_text(self, line: int) -> str:
        try:
            start_pos = self.SendScintilla(self.SCI_POSITIONFROMLINE, line)
            if start_pos < 0:
                return ""
                
            line_length = self.SendScintilla(self.SCI_LINELENGTH, line)
            if line_length <= 0:
                return ""
                
            text_bytes = self.SendScintilla(self.SCI_GETTEXTRANGE, start_pos, start_pos + line_length)
            if isinstance(text_bytes, bytes):
                return text_bytes.decode('utf-8', errors='replace')
            return str(text_bytes) if text_bytes else ""
        except Exception:
            return ""
            
    def is_include_line(self, line: int) -> bool:
        text = self.text(line).strip()
        return text.startswith('#include')
        
    def extract_include_path(self, line: int) -> Optional[str]:
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
    
    def get_error_at_position(self, line: int, col: int) -> Optional[Tuple[str, ErrorType]]:
        for error in self.errors:
            if error.line == line and error.start_col <= col < error.end_col:
                return (error.message, error.type)
        return None
        
    def keyPressEvent(self, event):
        pairs = {
            Qt.Key.Key_ParenLeft: ('(', ')'),
            Qt.Key.Key_BracketLeft: ('[', ']'),
            Qt.Key.Key_BraceLeft: ('{', '}'),
            Qt.Key.Key_QuoteDbl: ('"', '"'),
            Qt.Key.Key_Apostrophe: ("'", "'")
        }
        
        if event.key() in pairs:
            open_char, close_char = pairs[event.key()]
            self._insert_pair(open_char, close_char)
            event.accept()
            return
        elif event.key() == Qt.Key.Key_Control:
            self.ctrl_pressed = True
            
        super().keyPressEvent(event)
        
    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.ctrl_pressed = False
        super().keyReleaseEvent(event)
        
    def _insert_pair(self, open_char: str, close_char: str):
        line, col = self.getCursorPosition()
        self.insertAt(open_char + close_char, line, col)
        self.setCursorPosition(line, col + 1)
        
    def mouseMoveEvent(self, event: QMouseEvent):
        try:
            pos = event.pos()
            position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
            line, col = self.lineIndexFromPosition(position)
            
            error_info = self.get_error_at_position(line, col) if line >= 0 and col >= 0 else None
            
            if error_info:
                msg, err_type = error_info
                colors = self._get_theme_colors()
                self.error_tooltip.set_text(msg, colors, err_type)
                global_pos = event.globalPosition().toPoint()
                self.error_tooltip.move(global_pos.x() + 20, global_pos.y() + 10)
                if not self.error_tooltip.isVisible():
                    self.error_tooltip.show()
            else:
                self.error_tooltip.hide()
                
            super().mouseMoveEvent(event)
        except Exception as e:
            print(f"Error in mouseMoveEvent: {e}")
            super().mouseMoveEvent(event)
            
    def leaveEvent(self, event):
        self.error_tooltip.hide()
        super().leaveEvent(event)
        
    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.ctrl_pressed:
            pos = event.pos()
            position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
            line, col = self.lineIndexFromPosition(position)
            
            if line >= 0 and col >= 0 and self.is_include_line(line):
                path = self.extract_include_path(line)
                if path:
                    self.goto_definition_requested.emit(path, line)
                    event.accept()
                    return
                    
        super().mousePressEvent(event)
        
    def wheelEvent(self, event):
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0 and self.main_window:
                self.main_window.zoom_in()
            elif delta < 0 and self.main_window:
                self.main_window.zoom_out()
            event.accept()
        else:
            delta = event.angleDelta().y()
    
            current_value = self.verticalScrollBar().value()
            
            scroll_step = 10
            
            if delta > 0:
                target_value = current_value - scroll_step
            else:
                target_value = current_value + scroll_step

            target_value = max(0, min(target_value, self.verticalScrollBar().maximum()))

            self.scroll_animation.stop()
            self.scroll_animation.setStartValue(current_value)
            self.scroll_animation.setEndValue(target_value)
            self.scroll_animation.start()
            
    def paintEvent(self, event):
        super().paintEvent(event)
        
        if not self.error_text_visible or not self.error_messages:
            return
            
        try:
            painter = QPainter(self.viewport())
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            
            colors = self._get_theme_colors()
            
            first_line = self.SendScintilla(self.SCI_GETFIRSTVISIBLELINE)
            lines_visible = self.SendScintilla(self.SCI_LINESONSCREEN)
            margin_width = self.marginWidth(0)
            
            for line in range(first_line, first_line + lines_visible + 1):
                if line in self.error_messages:
                    self._paint_error_for_line(painter, line, colors, margin_width)
                    
            painter.end()
        except Exception as e:
            print(f"Error in paintEvent: {e}")
            
    def _paint_error_for_line(self, painter: QPainter, line: int, colors: dict, margin_width: int):
        try:
            pos = self.SendScintilla(self.SCI_POSITIONFROMLINE, line)
            if pos < 0:
                return
                
            y = self.SendScintilla(self.SCI_POINTYFROMPOSITION, 0, pos)
            line_end_pos = self.SendScintilla(self.SCI_GETLINEENDPOSITION, line)
            
            if line_end_pos > pos:
                end_x = self.SendScintilla(self.SCI_POINTXFROMPOSITION, 0, line_end_pos)
            else:
                end_x = margin_width
                
            x = end_x + 35
            
            error_type = 0
            for err in self.errors:
                if err.line == line:
                    error_type = err.type.value
                    break
                    
            text_color = colors['error']
            
            if (error_type == 1):
                text_color = colors['warning']
            elif (error_type == 2):
                text_color = colors['echo']

                
            error_text = self.error_messages[line]
            
            if (len(error_text.split('\n')) > 1):
                error_text = error_text.split('\n')[0] + "..."
            
            font = QFont("Consolas", 9)
            if self.main_window:
                font.setPointSize(self.main_window.global_font_size)
            painter.setFont(font)
            
            metrics = painter.fontMetrics()
            text_width = metrics.horizontalAdvance(error_text)
            max_width = self.width() - x - 20
            
            if text_width > max_width and max_width > 50:
                available_width = max_width - 30
                if available_width > 20:
                    avg_char_width = metrics.averageCharWidth()
                    if avg_char_width > 0:
                        chars_to_keep = int(available_width / avg_char_width)
                        if chars_to_keep > 5:
                            error_text = error_text[:chars_to_keep] + "..."
                            
            painter.setPen(QPen(text_color))
            painter.drawText(x, y + metrics.ascent(), error_text)
            
        except Exception as e:
            print(f"Error painting error for line {line}: {e}")
            
    def _get_theme_colors(self):
        if self.main_window:
            return THEMES[self.main_window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


class CodeSnapDialog(QWidget):
    """Dialog for creating beautiful code screenshots"""
    
    def __init__(self, editor: CustomScintilla, parent=None):
        super().__init__(None, Qt.WindowType.Window | Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)

        self.parent_window = parent
        self.editor = editor
        self.setWindowTitle("CodeSnap")
        
        self.padding = 30
        self.corner_radius = 12
        self.show_line_numbers = True
        self.show_window_controls = True
        self.auto_scale = True
        self.min_scale = 0.5
        self.max_scale = 1.0
        self.scale_factor = 1.0
        
        self.selected_text = self._get_selected_or_all_text()
        self.lines = self.selected_text.split('\n')
        
        if len(self.lines) > 500:
            self.lines = self.lines[:500]
            self.lines.append("// ... (truncated, showing first 500 lines)")
        
        self._calculate_size()
        self._adjust_to_screen()
        
        self.setFixedSize(self.image_width, self.image_height)
        self._center_on_screen()
        
        self.setWindowOpacity(0.0)
        self.fade_animation = QPropertyAnimation(self, b"windowOpacity")
        self.fade_animation.setDuration(300)
        self.fade_animation.setStartValue(0.0)
        self.fade_animation.setEndValue(1.0)
        self.fade_animation.start()
        
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        
        self.focus_check_timer = QTimer(self)
        self.focus_check_timer.setInterval(100)
        self.focus_check_timer.timeout.connect(self._check_focus)
        self.focus_check_timer.start()
        
        self.installEventFilter(self)
        
    def _get_selected_or_all_text(self) -> str:
        if self.editor.hasSelectedText():
            return self.editor.selectedText()
        return self.editor.text()
    
    def _calculate_size(self):
        font = QFont("Consolas", int(12 * self.scale_factor))
        metrics = QFontMetrics(font)
        
        line_height = metrics.height() + 4
        max_line_width = 0
        
        for line in self.lines:
            display_line = line.replace('\t', '    ')
            line_width = metrics.horizontalAdvance(display_line)
            max_line_width = max(max_line_width, line_width)
        
        line_number_width = 0
        if self.show_line_numbers:
            line_number_width = metrics.horizontalAdvance(str(len(self.lines))) + 30
        
        content_width = line_number_width + max_line_width
        self.image_width = content_width + self.padding * 2
        self.image_height = line_height * len(self.lines) + self.padding * 2 + 40
        
        self.image_width = int(self.image_width)
        self.image_height = int(self.image_height)
        
        self.image_width = max(self.image_width, 400)
        self.image_height = max(self.image_height, 200)
    
    def _adjust_to_screen(self):
        screen = QApplication.primaryScreen().availableGeometry()
        
        max_width = screen.width() - 100
        max_height = screen.height() - 100
        
        if self.auto_scale and (self.image_width > max_width or self.image_height > max_height):
            scale_x = max_width / self.image_width
            scale_y = max_height / self.image_height
            scale = min(scale_x, scale_y)
            
            scale = max(scale, self.min_scale)
            scale = min(scale, self.max_scale)
            
            if scale < 1.0:
                self.scale_factor = scale
                self._calculate_size()
                
                if self.image_width > max_width:
                    self.image_width = int(max_width)
                if self.image_height > max_height:
                    self.image_height = int(max_height)
    
    def _center_on_screen(self):
        screen = QApplication.primaryScreen().geometry()
        x = (screen.width() - self.width()) // 2
        y = (screen.height() - self.height()) // 2
        
        x = max(10, min(x, screen.width() - self.width() - 10))
        y = max(10, min(y, screen.height() - self.height() - 10))
        
        self.move(x, y)
    
    def _check_focus(self):
        if not self.isActiveWindow() and not self.underMouse():
            for widget in QApplication.topLevelWidgets():
                if isinstance(widget, QFileDialog) and widget.isVisible():
                    return
            self.close()
    
    def eventFilter(self, obj, event):
        if event.type() == QEvent.Type.WindowDeactivate:
            QTimer.singleShot(100, self._check_and_close)
        elif event.type() == QEvent.Type.MouseButtonPress:
            if not self.rect().contains(event.pos()):
                self.close()
        return super().eventFilter(obj, event)
    
    def _check_and_close(self):
        for widget in QApplication.topLevelWidgets():
            if isinstance(widget, QFileDialog) and widget.isVisible():
                return
            if isinstance(widget, QMessageBox) and widget.isVisible():
                return
        
        if not self.isActiveWindow() and not self.underMouse():
            self.close()
    
    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self.close()
        elif event.key() == Qt.Key.Key_S and event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            self.save_screenshot()
        elif event.key() == Qt.Key.Key_C and event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            self.copy_to_clipboard()
        elif event.key() == Qt.Key.Key_Plus and event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            self._zoom_in()
        elif event.key() == Qt.Key.Key_Minus and event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            self._zoom_out()
        else:
            super().keyPressEvent(event)
    
    def wheelEvent(self, event):
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0:
                self._zoom_in()
            else:
                self._zoom_out()
            event.accept()
        else:
            super().wheelEvent(event)
    
    def _zoom_in(self):
        if self.scale_factor < self.max_scale:
            self.scale_factor = min(self.scale_factor + 0.1, self.max_scale)
            self._recalculate_and_update()
    
    def _zoom_out(self):
        if self.scale_factor > self.min_scale:
            self.scale_factor = max(self.scale_factor - 0.1, self.min_scale)
            self._recalculate_and_update()
    
    def _recalculate_and_update(self):
        self._calculate_size()
        self.setFixedSize(self.image_width, self.image_height)
        self._center_on_screen()
        self.update()
    
    def focusOutEvent(self, event):
        QTimer.singleShot(100, self._check_and_close)
        super().focusOutEvent(event)
    
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        
        bg_color = colors['bg']
        painter.setBrush(bg_color)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(self.rect(), self.corner_radius, self.corner_radius)
        
        painter.setBrush(Qt.BrushStyle.NoBrush)
        pen = QPen(colors['status_border'], 1)
        painter.setPen(pen)
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), self.corner_radius, self.corner_radius)
        
        if self.show_window_controls:
            self._draw_window_header(painter, colors)
        
        self._draw_code(painter, colors)
    
    def _draw_window_header(self, painter: QPainter, colors: dict):
        header_height = 30
        
        btn_size = 12
        btn_spacing = 8
        btn_y = self.padding + (header_height - btn_size) // 2
        
        painter.setBrush(QColor(255, 95, 86))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawEllipse(self.padding + 10, btn_y, btn_size, btn_size)
        
        painter.setBrush(QColor(255, 189, 46))
        painter.drawEllipse(self.padding + 10 + btn_size + btn_spacing, btn_y, btn_size, btn_size)
        
        painter.setBrush(QColor(39, 201, 63))
        painter.drawEllipse(self.padding + 10 + (btn_size + btn_spacing) * 2, btn_y, btn_size, btn_size)
        
        if self.editor.filename:
            filename = os.path.basename(self.editor.filename)
            painter.setPen(QPen(colors['comment'], 1))
            font = QFont("Consolas", int(10))
            painter.setFont(font)
            text_rect = QRect(
                self.padding + 10 + (btn_size + btn_spacing) * 3 + 20,
                self.padding,
                self.width() - self.padding * 2 - 100,
                header_height
            )
            
            metrics = QFontMetrics(font)
            elided_text = metrics.elidedText(filename, Qt.TextElideMode.ElideMiddle, text_rect.width())
            painter.drawText(text_rect, Qt.AlignmentFlag.AlignVCenter, elided_text)
    
    def _draw_code(self, painter: QPainter, colors: dict):
        lexer = TwistLangLexer(theme_name=self.parent_window.current_theme)
        
        font = QFont("Consolas", int(12 * self.scale_factor))
        painter.setFont(font)
        metrics = QFontMetrics(font)
        
        line_height = metrics.height() + 4
        start_y = self.padding + (40 if self.show_window_controls else 10)
        
        max_y = self.height() - self.padding - 25
        visible_lines = len(self.lines)
        
        max_x = self.width() - self.padding - 10
        
        for i in range(visible_lines):
            line = self.lines[i]
            y = start_y + i * line_height
            
            if self.show_line_numbers:
                line_num = str(i + 1)
                num_width = metrics.horizontalAdvance(str(len(self.lines))) + 20
                painter.setPen(QPen(colors['comment'], 1))
                painter.drawText(
                    int(self.padding + 5), y + metrics.ascent(),
                    line_num.rjust(len(str(len(self.lines))))
                )
                x_offset = int(self.padding + num_width)
            else:
                x_offset = self.padding + 10
            
            tokens = self._highlight_line(line, lexer, colors)
            current_x = x_offset
            
            for text, color in tokens:
                char_width = metrics.horizontalAdvance(text)
                
                if current_x + char_width > max_x:
                    available_width = max_x - current_x
                    if available_width > metrics.averageCharWidth():
                        chars_to_fit = 0
                        width_so_far = 0
                        for char in text:
                            char_w = metrics.horizontalAdvance(char)
                            if width_so_far + char_w > available_width:
                                break
                            width_so_far += char_w
                            chars_to_fit += 1
                        
                        if chars_to_fit > 0:
                            truncated = text[:chars_to_fit]
                            painter.setPen(QPen(color, 1))
                            painter.drawText(current_x, y + metrics.ascent(), truncated)
                    break
                
                painter.setPen(QPen(color, 1))
                painter.drawText(current_x, y + metrics.ascent(), text)
                current_x += char_width
        
        if visible_lines < len(self.lines):
            remaining = len(self.lines) - visible_lines
            indicator = f"... {remaining} more line(s) ..."
            
            font = QFont("Consolas", int(10 * self.scale_factor))
            font.setItalic(True)
            painter.setFont(font)
            metrics = QFontMetrics(font)
            
            text_width = metrics.horizontalAdvance(indicator)
            x = (self.width() - text_width) // 2
            y = start_y + visible_lines * line_height + metrics.ascent() + 5
            
            indicator_color = QColor(colors['comment'])
            indicator_color.setAlpha(150)
            painter.setPen(QPen(indicator_color, 1))
            painter.drawText(x, y, indicator)
    
    def _highlight_line(self, line: str, lexer, colors: dict) -> List[Tuple[str, QColor]]:
        tokens = []
        i = 0
        n = len(line)
        expecting_namespace = False
        
        while i < n:
            if line.startswith('//', i):
                tokens.append((line[i:], colors.get("comment", colors["fg"])))
                break
            
            if line[i] in ('"', "'"):
                start = i
                quote = line[i]
                i += 1
                escaped = False
                while i < n:
                    ch = line[i]
                    if ch == '\\':
                        escaped = not escaped
                    elif ch == quote and not escaped:
                        i += 1
                        break
                    else:
                        escaped = False
                    i += 1
                token_text = line[start:i]
                tokens.append((token_text, colors.get("string", colors["fg"])))
                continue
            
            if line[i].isdigit() or (line[i] == '.' and i + 1 < n and line[i + 1].isdigit()):
                j = i
                has_dot = False
                while j < n and (line[j].isdigit() or (line[j] == '.' and not has_dot)):
                    if line[j] == '.':
                        has_dot = True
                    j += 1
                tokens.append((line[i:j], colors.get("number", colors["fg"])))
                i = j
                continue
            
            if line[i].isalpha() or line[i] == '_' or line[i] == '#':
                j = i
                while j < n and (line[j].isalnum() or line[j] == '_' or line[j] == '#'):
                    j += 1
                word = line[i:j]
                color = colors["fg"]
                
                if word == "namespace" or word == "struct":
                    expecting_namespace = True
                    color = colors.get("keyword", colors["fg"])
                elif expecting_namespace:
                    color = colors.get("namespace", colors["fg"])
                    expecting_namespace = False
                else:
                    if word in lexer.keywords:
                        color = colors.get("keyword", colors["fg"])
                    elif word in lexer.special_keywords:
                        color = colors.get("special", colors["fg"])
                    elif word in lexer.modifiers:
                        color = colors.get("modifier", colors["fg"])
                    elif word in lexer.types:
                        color = colors.get("type", colors["fg"])
                    elif word in lexer.literals:
                        color = colors.get("literal", colors["fg"])
                    elif word.startswith('#'):
                        color = colors.get("directive", colors["fg"])
                    elif j < n and line[j:j+2] == '::':
                        color = colors.get("namespace", colors["fg"])
                    elif j < n and line[j] == '(':
                        color = colors.get("function", colors["fg"])
                    elif j < n and line[j] == '.':
                        color = colors.get("object", colors["fg"])
                
                tokens.append((word, color))
                i = j
                continue
            
            if line[i] in '+-*/%=&|^!<>~,?.:;(){}[]':
                two_char = line[i:i+2]
                if two_char in {'::', '->', '<=', '>=', '==', '!=', '<<', '>>'}:
                    tokens.append((two_char, colors.get("operator", colors["fg"])))
                    i += 2
                    continue
                tokens.append((line[i], colors.get("operator", colors["fg"])))
                i += 1
                continue
            
            if line[i].isspace():
                j = i
                while j < n and line[j].isspace():
                    j += 1
                tokens.append((line[i:j], colors["fg"]))
                i = j
                continue
            
            tokens.append((line[i], colors["fg"]))
            i += 1
        
        return tokens
    
    def _get_theme_colors(self):
        if self.parent_window and hasattr(self.parent_window, 'current_theme'):
            return THEMES[self.parent_window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]
    
    def save_screenshot(self):
        filename, _ = QFileDialog.getSaveFileName(
            self, "Save Screenshot", "",
            "PNG Image (*.png);;All Files (*.*)"
        )
        if filename:
            if not filename.endswith('.png'):
                filename += '.png'
            
            pixmap = self.grab()
            pixmap.save(filename, 'PNG')
    
    def copy_to_clipboard(self):
        pixmap = self.grab()
        QApplication.clipboard().setPixmap(pixmap)
        
        QToolTip.showText(
            self.mapToGlobal(QPoint(self.width() // 2, self.height() // 2)),
            "Copied to clipboard!",
            self, QRect(), 1000
        )
    
    def closeEvent(self, event):
        self.focus_check_timer.stop()
        super().closeEvent(event)


# =============================================================================
# MAIN WINDOW
# =============================================================================

class TwistLangEditor(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        
        self.current_theme = DEFAULT_THEME
        self.global_font_size = DEFAULT_FONT_SIZE
        self.autosave_interval = 300
        self.autosave_count = 0
        self.current_file = None
        self.current_project_folder = None
        self.explorer_width = 250
        self.explorer_visible = False  # Изначально скрыто
        self.first_file_opened = False  # Флаг для отслеживания первого открытого файла
        
        self.ls_processes: Dict[str, subprocess.Popen] = {}
        self.theme_actions: List[QAction] = []
        self.interval_actions: List[QAction] = []
        self.language_actions: List[QAction] = []
        
        self.resizing = False
        self.resize_direction = None
        self.resize_start_pos = None
        self.resize_start_geometry = None

        self.error_text_visible = True
        
        self.theme_preview = None
        self.preview_hide_timer = QTimer(self)
        self.preview_hide_timer.setInterval(50)
        self.preview_hide_timer.timeout.connect(self._check_preview_hide)
        
        self._setup_window()
        self._setup_ui()
        self._setup_shortcuts()
        self._setup_timers()
        
        self.apply_theme(self.current_theme)
        
        QTimer.singleShot(100, self._initialize_theme_checkmark)
        QTimer.singleShot(200, self._initialize_interval_icons)

        self.error_timer = QTimer()
        self.error_timer.timeout.connect(self.check_current_file_errors)
        self.error_timer.start(100)
    
        
    def _setup_window(self):
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setGeometry(100, 100, 1200, 800)
        self.setAcceptDrops(True)

    def _rebuild_menus(self):
        menubar = self.menuBar()
        menubar.clear()
        menubar.setVisible(False)
        
        consolas_font = QFont("Consolas", 11)
        menubar.setFont(consolas_font)

        self.theme_actions.clear()
        self.interval_actions.clear()
        self.language_actions.clear()
    
        
        menubar = self.menuBar()
        menubar.setVisible(False)
        
        consolas_font = QFont("Consolas", 11)
        
        # File menu
        file_menu = RoundedMenu(self)
        file_menu.setTitle(Strings.get("file_menu"))
        file_menu.setFont(consolas_font)
        menubar.addMenu(file_menu)
        
        new_action = self._create_action(Strings.get("new"), "Ctrl+N", self.new_file)
        file_menu.addAction(new_action)
        
        open_action = self._create_action(Strings.get("open"), "Ctrl+O", self.open_file_dialog)
        file_menu.addAction(open_action)
        
        open_folder_action = self._create_action(Strings.get("open_folder"), "Ctrl+Shift+O", self.open_folder_dialog)
        file_menu.addAction(open_folder_action)
        
        save_action = self._create_action(Strings.get("save"), "Ctrl+S", self.save_current_file)
        file_menu.addAction(save_action)
        
        save_as_action = self._create_action(Strings.get("save_as"), "Ctrl+Shift+S", self.save_current_file_as)
        file_menu.addAction(save_as_action)
        
        autosave_menu = RoundedMenu(self)
        autosave_menu.setTitle(Strings.get("auto_save"))
        autosave_menu.setFont(consolas_font)
        file_menu.addMenu(autosave_menu)
        
        self.autosave_action = QAction(Strings.get("enable_auto_save"), self)
        self.autosave_action.setCheckable(True)
        self.autosave_action.setChecked(True)
        self.autosave_action.setFont(consolas_font)
        self.autosave_action.triggered.connect(self._toggle_autosave)
        autosave_menu.addAction(self.autosave_action)
        
        colors = THEMES[self.current_theme]["colors"]
        self.autosave_action.setIcon(self._create_check_icon(colors["fg"]))
        
        autosave_menu.addSeparator()
        
        interval_menu = RoundedMenu(self)
        interval_menu.setTitle(Strings.get("interval"))
        interval_menu.setFont(consolas_font)
        autosave_menu.addMenu(interval_menu)
        
        for name, ms in AUTOSAVE_INTERVALS:
            action = QAction(name, self)
            action.setCheckable(True)
            action.setData(ms)
            action.setFont(consolas_font)
            if ms == self.autosave_interval:
                action.setChecked(True)
                colors = THEMES[self.current_theme]["colors"]
                action.setIcon(self._create_check_icon(colors["fg"]))
            action.triggered.connect(lambda checked, ms=ms: self._set_autosave_interval(ms))
            interval_menu.addAction(action)
            self.interval_actions.append(action)
            
        autosave_menu.addSeparator()
        
        save_now_action = self._create_action(Strings.get("save_now"), "Ctrl+Shift+S", 
                                              lambda: self.autosave_all_files(manual=True))
        autosave_menu.addAction(save_now_action)
    
        file_menu.addSeparator()
        
        close_action = self._create_action(Strings.get("close"), "Ctrl+W", self.close_current_tab)
        file_menu.addAction(close_action)
        
        close_all_action = self._create_action(Strings.get("close_all"), None, self.close_all_tabs)
        file_menu.addAction(close_all_action)
        
        file_menu.addSeparator()
        
        exit_action = self._create_action(Strings.get("exit"), None, self.close)
        file_menu.addAction(exit_action)
        
        # Edit menu
        edit_menu = RoundedMenu(self)
        edit_menu.setTitle(Strings.get("edit_menu"))
        edit_menu.setFont(consolas_font)
        menubar.addMenu(edit_menu)
        
        edit_menu.addAction(self._create_action(Strings.get("undo"), "Ctrl+Z", lambda: self.current_editor().undo() if self.current_editor() else None))
        edit_menu.addAction(self._create_action(Strings.get("redo"), "Ctrl+Y", lambda: self.current_editor().redo() if self.current_editor() else None))
        edit_menu.addSeparator()
        edit_menu.addAction(self._create_action(Strings.get("cut"), "Ctrl+X", lambda: self.current_editor().cut() if self.current_editor() else None))
        edit_menu.addAction(self._create_action(Strings.get("copy"), "Ctrl+C", lambda: self.current_editor().copy() if self.current_editor() else None))
        edit_menu.addAction(self._create_action(Strings.get("paste"), "Ctrl+V", lambda: self.current_editor().paste() if self.current_editor() else None))
        
        # Settings menu
        settings_menu = RoundedMenu(self)
        settings_menu.setTitle(Strings.get("settings_menu"))
        settings_menu.setFont(consolas_font)
        menubar.addMenu(settings_menu)
        
        settings_menu.addAction(self._create_action(Strings.get("zoom_in"), "Ctrl+=", self.zoom_in))
        settings_menu.addAction(self._create_action(Strings.get("zoom_out"), "Ctrl+-", self.zoom_out))
        settings_menu.addSeparator()
        
        # Theme submenu
        theme_menu = RoundedMenu(self)
        theme_menu.setTitle(Strings.get("theme"))
        theme_menu.setFont(consolas_font)
        settings_menu.addMenu(theme_menu)
        
        theme_menu.hovered_action.connect(self._on_theme_menu_hover)
        
        prev_type = None
        for theme_name in THEMES.keys():
            theme_data = THEMES[theme_name]
            current_type = theme_data.get("type", "dark")
            
            if prev_type is not None and prev_type != current_type:
                theme_menu.addSeparator()
            prev_type = current_type

            theme_action = QAction(theme_name, self)
            theme_action.setCheckable(True)
            theme_action.setFont(consolas_font)
            theme_action.setData(theme_name)
            if theme_name == self.current_theme:
                theme_action.setChecked(True)
            theme_action.triggered.connect(lambda checked, tn=theme_name: self.select_theme(tn))
            theme_menu.addAction(theme_action)
            self.theme_actions.append(theme_action)
        
        settings_menu.addSeparator()
        
        # Language submenu
        language_menu = RoundedMenu(self)
        language_menu.setTitle(Strings.get("language"))
        language_menu.setFont(consolas_font)
        settings_menu.addMenu(language_menu)
        
        eng_action = QAction(Strings.get("english"), self)
        eng_action.setCheckable(True)
        eng_action.setFont(consolas_font)
        if Strings.current_language == Language.ENGLISH:
            eng_action.setChecked(True)
        eng_action.triggered.connect(lambda: self._switch_language(Language.ENGLISH))
        language_menu.addAction(eng_action)
        self.language_actions.append(eng_action)
        
        ru_action = QAction(Strings.get("russian"), self)
        ru_action.setCheckable(True)
        ru_action.setFont(consolas_font)
        if Strings.current_language == Language.RUSSIAN:
            ru_action.setChecked(True)
        ru_action.triggered.connect(lambda: self._switch_language(Language.RUSSIAN))
        language_menu.addAction(ru_action)
        self.language_actions.append(ru_action)
        
        settings_menu.addSeparator()
        
        about_action = self._create_action(Strings.get("about"), None, self.show_about_dialog)
        settings_menu.addAction(about_action)

        settings_menu.addSeparator()

        self.error_text_action = QAction(Strings.get("error_text"), self)
        self.error_text_action.setCheckable(True)
        self.error_text_action.setChecked(self.error_text_visible)
        self.error_text_action.setFont(consolas_font)
        self.error_text_action.triggered.connect(self._toggle_error_text)
        settings_menu.addAction(self.error_text_action)

            
        # Run menu
        run_menu = RoundedMenu(self)
        run_menu.setTitle(Strings.get("run_menu"))
        run_menu.setFont(consolas_font)
        menubar.addMenu(run_menu)
        
        run_action = self._create_action(Strings.get("run_code"), "F5", self.run_current_file)
        run_menu.addAction(run_action)
        
        go_to_action = self._create_action(Strings.get("go_to_include"), "F12", self.goto_include_under_cursor)
        run_menu.addAction(go_to_action)
        
        run_menu.addSeparator()

        restart_ls_action = self._create_action(Strings.get("restart_ls"), "Ctrl+Shift+R", self.restart_language_server)
        run_menu.addAction(restart_ls_action)

        run_menu.addSeparator()
        
        clear_errors_action = self._create_action(Strings.get("clear_errors"), "Ctrl+E", self.clear_all_errors)
        run_menu.addAction(clear_errors_action)
        
        check_errors_action = self._create_action(Strings.get("check_errors"), "F4", self.check_current_file_errors)
        run_menu.addAction(check_errors_action)

        run_menu.addSeparator()

        codesnap_action = self._create_action(Strings.get("code_snap"), "Ctrl+Shift+P", self.show_codesnap)
        run_menu.addAction(codesnap_action)

        
        self.title_bar.add_menu(menubar)
        self._update_checkmarks_after_rebuild()

    def _toggle_error_text(self, checked: bool):
        self.error_text_visible = checked
        
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                editor.set_error_text_visible(checked)
        
        if checked:
            colors = THEMES[self.current_theme]["colors"]
            self.error_text_action.setIcon(self._create_check_icon(colors["fg"]))
        else:
            self.error_text_action.setIcon(QIcon())
        
        status_text = "Inline errors ON" if checked else "Inline errors OFF"
        self.show_status_message(status_text, 2000)

    def show_codesnap(self):
        editor = self.current_editor()
        if not editor:
            self.show_status_message("No file open to screenshot", 2000)
            return
        
        dialog = CodeSnapDialog(editor, self)
        dialog.show()
        
        self._codesnap_dialog = dialog

    def open_folder_dialog(self):
        folder = QFileDialog.getExistingDirectory(
            self,
            Strings.get("select_folder"),
            "",
            QFileDialog.Option.ShowDirsOnly
        )
        if folder:
            self.file_explorer.set_root_path(folder)

    def on_folder_opened(self, path: str):
        self.current_project_folder = path
        self.show_status_message(Strings.get("folder_opened").format(path), 2000)
        
        reply = QMessageBox.question(
            self,
            Strings.get("load_project_files"),
            Strings.get("load_project_files_question"),
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        if reply == QMessageBox.StandardButton.Yes:
            self.load_files_from_folder(path)

    def load_files_from_folder(self, path: str):
        loaded = 0
        for root, dirs, files in os.walk(path):
            for file in files:
                if file.endswith('.lumen'):
                    full_path = os.path.join(root, file)
                    already_open = False
                    for i in range(self.tab_widget.count()):
                        editor = self.tab_widget.widget(i)
                        if isinstance(editor, CustomScintilla) and getattr(editor, 'filename', None) == full_path:
                            already_open = True
                            break
                    if not already_open:
                        self.open_file(full_path)
                        loaded += 1
                    
        if loaded > 0:
            self.show_status_message(Strings.get("loaded_files").format(loaded), 3000)

    def _on_theme_menu_hover(self, action: QAction):
        if action and action.data():
            theme_name = action.data()
            if theme_name in THEMES:
                self._show_theme_preview(theme_name, action)
            else:
                self._hide_theme_preview()
        else:
            self.preview_hide_timer.start()
    
    def _check_preview_hide(self):
        if self.theme_preview:
            from PyQt6.QtGui import QCursor
            cursor_pos = QCursor.pos()
            preview_geo = self.theme_preview.geometry()
            
            if not preview_geo.contains(cursor_pos):
                for widget in QApplication.instance().topLevelWidgets():
                    if isinstance(widget, RoundedMenu) and widget.isVisible():
                        if widget.geometry().contains(cursor_pos):
                            return
                self._hide_theme_preview()
                self.preview_hide_timer.stop()
    
    def _show_theme_preview(self, theme_name: str, action: QAction):
        self.preview_hide_timer.stop()
        
        if not self.theme_preview:
            self.theme_preview = ThemePreviewWidget()
        
        self.theme_preview.set_theme(theme_name)
        
        menu = None
        for widget in QApplication.instance().topLevelWidgets():
            if isinstance(widget, RoundedMenu) and widget.isVisible():
                for menu_action in widget.actions():
                    if menu_action == action:
                        menu = widget
                        break
                if menu:
                    break
        
        if menu:
            menu_geo = menu.geometry()
            preview_x = menu_geo.right() + 5
            preview_y = menu_geo.top()
            self.theme_preview.move(preview_x, preview_y)
        else:
            from PyQt6.QtGui import QCursor
            cursor_pos = QCursor.pos()
            self.theme_preview.move(cursor_pos.x() + 20, cursor_pos.y())
        
        if not self.theme_preview.isVisible():
            self.theme_preview.show()
    
    def _hide_theme_preview(self):
        if self.theme_preview:
            self.theme_preview.hide()
        self.preview_hide_timer.stop()

    def _update_checkmarks_after_rebuild(self):
        colors = THEMES[self.current_theme]["colors"]
        self.check_icon = self._create_check_icon(colors["fg"])
        
        for action in self.theme_actions:
            if action.text() == self.current_theme:
                action.setChecked(True)
                action.setIcon(self.check_icon)
            else:
                action.setChecked(False)
                action.setIcon(QIcon())
        
        if hasattr(self, 'autosave_action'):
            if self.autosave_action.isChecked():
                self.autosave_action.setIcon(self.check_icon)
            else:
                self.autosave_action.setIcon(QIcon())

        if hasattr(self, 'error_text_action'):
            if self.error_text_visible:
                self.error_text_action.setChecked(True)
                self.error_text_action.setIcon(self.check_icon)
            else:
                self.error_text_action.setChecked(False)
                self.error_text_action.setIcon(QIcon())
        
        for action in self.interval_actions:
            if action.data() == self.autosave_interval:
                action.setChecked(True)
                action.setIcon(self.check_icon)
            else:
                action.setChecked(False)
                action.setIcon(QIcon())
        
        for action in self.language_actions:
            if (Strings.current_language == Language.ENGLISH and action.text() == Strings.get("english")) or \
            (Strings.current_language == Language.RUSSIAN and action.text() == Strings.get("russian")):
                action.setChecked(True)
                action.setIcon(self.check_icon)
            else:
                action.setChecked(False)
                action.setIcon(QIcon())
        
    def _setup_ui(self):

        
        central_widget = QWidget()
        central_widget.setObjectName("centralWidget")
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        
        self.title_bar = CustomTitleBar(self)
        self.title_bar.setObjectName("titleBar")
        main_layout.addWidget(self.title_bar)
        
        # Create splitter for file explorer and content
        self.main_splitter = QSplitter(Qt.Orientation.Horizontal)
        self.main_splitter.setHandleWidth(0)
        self.main_splitter.setChildrenCollapsible(False)
        main_layout.addWidget(self.main_splitter, 1)
        
        # File explorer - изначально скрыт
        self.file_explorer = FileExplorer(self)
        self.file_explorer.file_opened.connect(self.open_file)
        self.main_splitter.addWidget(self.file_explorer)
        
        # Content stack (welcome + tabs)
        self.content_stack = QStackedWidget()
        self.content_stack.setObjectName("contentStack")
        self.main_splitter.addWidget(self.content_stack)
        
        # Set initial splitter sizes (explorer width = 0)
        self.main_splitter.setSizes([0, self.width()])

        self.main_splitter.setStyleSheet("""
            QSplitter::handle {
                background-color: transparent;
                width: 0px;
            }
            QSplitter {
                margin: 0px;
                padding: 0px;
            }
        """)
            
        # Store splitter visibility state
        self.explorer_visible = False
        self.explorer_width = 250
        
        # Add toggle button to title bar
        self._add_explorer_toggle()
        self.explorer_toggle.setChecked(False)  # Изначально не отмечен
        
        self.welcome_widget = WelcomeWidget()
        self.content_stack.addWidget(self.welcome_widget)
        
        self.tab_container = QWidget()
        self.tab_container.setObjectName("tabContainer")
        
        self.tab_container.setStyleSheet(f"""
            QWidget#tabContainer {{
                background-color: {THEMES[DEFAULT_THEME]['colors']['title_bg'].name()};
            }}
        """)
        
        tab_layout = QVBoxLayout(self.tab_container)
        tab_layout.setContentsMargins(0, 0, 0, 0)
        tab_layout.setSpacing(0)
        
        self.tab_widget = EditorTabWidget(self)
        tab_layout.addWidget(self.tab_widget)
        self.content_stack.addWidget(self.tab_container)
        
        self.tab_widget.currentChanged.connect(self._on_tab_changed)
        
        self.status_bar = QStatusBar()
        self.status_bar.setObjectName("statusBar")
        self.status_bar.setSizeGripEnabled(False)
        self.setStatusBar(self.status_bar)
        
        toggle_container = QWidget()
        toggle_layout = QHBoxLayout(toggle_container)
        toggle_layout.setContentsMargins(10, 0, 10, 0)
        toggle_layout.setSpacing(5)
        
        self.folding_toggle = AnimatedToggle()
        self.folding_toggle.setChecked(False)
        self.folding_toggle.toggled.connect(self._toggle_folding_handler)
        self.folding_toggle.setToolTip(Strings.get("toggle_folding"))
        
        toggle_layout.addWidget(self.folding_toggle)
        
        self.separator = QFrame()
        self.separator.setFrameShape(QFrame.Shape.VLine)
        self.separator.setFrameShadow(QFrame.Shadow.Sunken)
        self.separator.setFixedWidth(2)
        self.separator.setStyleSheet(f"""
            QFrame {{
                background-color: {THEMES[DEFAULT_THEME]['colors']['status_border'].name()};
                margin: 2px 0px;
            }}
        """)
        
        self.autosave_label = QLabel()
        self.error_label = QLabel()
        self.ls_status_label = QLabel()
        
        self.status_bar.addPermanentWidget(self.autosave_label)
        self.status_bar.addPermanentWidget(self.error_label)
        self.status_bar.addPermanentWidget(self.ls_status_label)
        self.status_bar.addPermanentWidget(self.separator)
        self.status_bar.addPermanentWidget(toggle_container)
        
        menubar = self.menuBar()
        consolas_font = QFont("Consolas", 11)
        menubar.setFont(consolas_font)
        
        self._rebuild_menus()
        
        self.title_bar.window_minimized.connect(self.showMinimized)
        self.title_bar.window_maximized.connect(self._toggle_maximize)
        self.title_bar.window_closed.connect(self.close)
        self.title_bar.run_clicked.connect(self.run_current_file)
        
        self._update_content_display()

        self.border_overlay = WindowBorderOverlay(self)
        self.border_overlay.setGeometry(self.rect())
        
        QApplication.instance().installEventFilter(self)
        
        self._apply_global_font_to_all_editors()

    def _show_file_explorer_for_path(self, path: str):
        """Показать файловое дерево для указанного пути"""
        if not path:
            return
            
        folder_path = os.path.dirname(path) if os.path.isfile(path) else path
        
        if os.path.exists(folder_path):
            self.file_explorer.set_root_path(folder_path)
            
            if not self.explorer_visible:
                # Показать файловое дерево
                sizes = self.main_splitter.sizes()
                self.main_splitter.setSizes([self.explorer_width, sizes[1] - self.explorer_width])
                self.explorer_visible = True
                self.explorer_toggle.setChecked(True)
            
            self.current_project_folder = folder_path

    def _add_explorer_toggle(self):
        self.explorer_toggle = QToolButton()
        self.explorer_toggle.setText("📂")
        self.explorer_toggle.setToolTip(Strings.get("toggle_explorer"))
        self.explorer_toggle.setFixedSize(30, TITLE_BAR_HEIGHT)
        self.explorer_toggle.setCheckable(True)
        self.explorer_toggle.setChecked(True)
        self.explorer_toggle.clicked.connect(self._toggle_file_explorer)
        
        self.title_bar.menu_layout.insertWidget(0, self.explorer_toggle)
    
    def _toggle_file_explorer(self):
        if self.explorer_visible:
            sizes = self.main_splitter.sizes()
            self.explorer_width = sizes[0]
            self.main_splitter.setSizes([0, sizes[0] + sizes[1]])
            self.explorer_visible = False
        else:
            sizes = self.main_splitter.sizes()
            self.main_splitter.setSizes([self.explorer_width, sizes[1] - self.explorer_width])
            self.explorer_visible = True
            
        self.explorer_toggle.setChecked(self.explorer_visible)

    def _handle_resize_start(self, event):
        """Обработчик начала изменения размера"""
        global_pos = event.globalPosition().toPoint()
        local_pos = self.mapFromGlobal(global_pos)
        direction = self._get_resize_direction(local_pos)
        
        if direction:
            self.resizing = True
            self.resize_direction = direction
            self.resize_start_pos = global_pos
            self.resize_start_geometry = self.geometry()
            event.accept()
            return True
        return False

    def _handle_resize_move(self, event):
        """Обработчик изменения размера"""
        if self.resizing and self.resize_direction:
            current_pos = event.globalPosition().toPoint()
            dx = current_pos.x() - self.resize_start_pos.x()
            dy = current_pos.y() - self.resize_start_pos.y()
            
            new_geo = QRect(self.resize_start_geometry)
            
            if "left" in self.resize_direction:
                new_geo.setLeft(new_geo.left() + dx)
            if "right" in self.resize_direction:
                new_geo.setRight(new_geo.right() + dx)
            if "top" in self.resize_direction:
                new_geo.setTop(new_geo.top() + dy)
            if "bottom" in self.resize_direction:
                new_geo.setBottom(new_geo.bottom() + dy)
                
            if new_geo.width() < WINDOW_MIN_WIDTH:
                if "left" in self.resize_direction:
                    new_geo.setLeft(new_geo.right() - WINDOW_MIN_WIDTH)
                else:
                    new_geo.setRight(new_geo.left() + WINDOW_MIN_WIDTH)
            if new_geo.height() < WINDOW_MIN_HEIGHT:
                if "top" in self.resize_direction:
                    new_geo.setTop(new_geo.bottom() - WINDOW_MIN_HEIGHT)
                else:
                    new_geo.setBottom(new_geo.top() + WINDOW_MIN_HEIGHT)
                    
            self.setGeometry(new_geo)
            event.accept()
            return True
        return False

    def _handle_resize_end(self, event):
        """Обработчик окончания изменения размера"""
        if self.resizing:
            self.resizing = False
            self.resize_direction = None
            self.setCursor(Qt.CursorShape.ArrowCursor)
            event.accept()
            return True
        return False

    def eventFilter(self, obj, event):
        # Перехватываем MouseButtonPress для всех виджетов
        if event.type() == QEvent.Type.MouseButtonPress:
            if self._handle_resize_start(event):
                return True
        
        # Перехватываем MouseMove для всех виджетов
        if event.type() == QEvent.Type.MouseMove:
            # Сначала проверяем, нужно ли обновить курсор
            if not self.resizing:
                global_pos = event.globalPosition().toPoint()
                local_pos = self.mapFromGlobal(global_pos)
                direction = self._get_resize_direction(local_pos)
                cursor_map = {
                    "left": Qt.CursorShape.SizeHorCursor,
                    "right": Qt.CursorShape.SizeHorCursor,
                    "top": Qt.CursorShape.SizeVerCursor,
                    "bottom": Qt.CursorShape.SizeVerCursor,
                    "topleft": Qt.CursorShape.SizeFDiagCursor,
                    "bottomright": Qt.CursorShape.SizeFDiagCursor,
                    "topright": Qt.CursorShape.SizeBDiagCursor,
                    "bottomleft": Qt.CursorShape.SizeBDiagCursor
                }
                self.setCursor(cursor_map.get(direction, Qt.CursorShape.ArrowCursor))
            
            # Затем обрабатываем изменение размера
            if self._handle_resize_move(event):
                return True
        
        # Перехватываем MouseButtonRelease для всех виджетов
        if event.type() == QEvent.Type.MouseButtonRelease:
            if self._handle_resize_end(event):
                return True
        
        # Обработка изменения размера окна
        if obj == self and event.type() == QEvent.Type.Resize:
            self.border_overlay.setGeometry(self.rect())
        
        return super().eventFilter(obj, event)

        
    def _initialize_theme_checkmark(self):
        colors = THEMES[self.current_theme]["colors"]
        self.check_icon = self._create_check_icon(colors["fg"])
        
        for action in self.theme_actions:
            if action.text() == self.current_theme:
                action.setChecked(True)
                action.setIcon(self.check_icon)
                break
        
        if hasattr(self, 'autosave_action'):
            if self.autosave_action.isChecked():
                self.autosave_action.setIcon(self.check_icon)
            else:
                self.autosave_action.setIcon(QIcon())
    
    def _initialize_interval_icons(self):
        colors = THEMES[self.current_theme]["colors"]
        check_icon = self._create_check_icon(colors["fg"])
        
        for action in self.interval_actions:
            if action.data() == self.autosave_interval:
                action.setChecked(True)
                action.setIcon(check_icon)
            else:
                action.setChecked(False)
                action.setIcon(QIcon())
                
    def _toggle_folding_handler(self, enabled: bool):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                editor.set_folding_visible(enabled)
        
        state_text = Strings.get("code_folding_on") if enabled else Strings.get("code_folding_off")
        self.show_status_message(state_text, 2000)
        
    def _switch_language(self, language: Language):
        Strings.set_language(language)
        
        self._rebuild_menus()
        
        self.title_bar.title_label.setText(Strings.get("window_title"))
        
        self.welcome_widget.update()
        
        self._update_status_labels()
        
        self.folding_toggle.setToolTip(Strings.get("toggle_folding"))
        
        lang_name = Strings.get("english") if language == Language.ENGLISH else Strings.get("russian")
        self.show_status_message(f"Language switched to {lang_name}", 2000)

        if self.theme_preview:
            self.theme_preview.update_language()
            
        self.file_explorer.update_theme()

    def _create_action(self, text: str, shortcut: Optional[str], slot) -> QAction:
        action = QAction(text, self)
        if shortcut:
            action.setShortcut(QKeySequence(shortcut))
        action.triggered.connect(slot)
        action.setFont(QFont("Consolas", 10))
        return action
    
    def show_about_dialog(self):
        about_win = AboutDialog(self)
        about_win.setWindowTitle(Strings.get("about"))
        about_win.show()
        self._about_window = about_win
        
    def _setup_shortcuts(self):
        QShortcut(QKeySequence("F8"), self).activated.connect(self.goto_next_error)
        QShortcut(QKeySequence("Shift+F8"), self).activated.connect(self.goto_prev_error)

    def _create_check_icon(self, color: QColor) -> QIcon:
        try:
            svg_path = r"data\check.svg"
            if os.path.exists(svg_path):
                return create_svg_icon(svg_path, color)
        except Exception as e:
            print(f"Error creating check icon: {e}")
        
        pixmap = QPixmap(16, 16)
        pixmap.fill(Qt.GlobalColor.transparent)
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.setPen(QPen(color, 2))
        painter.drawLine(3, 8, 7, 12)
        painter.drawLine(7, 12, 13, 4)
        painter.end()
        return QIcon(pixmap)
        
    def _setup_timers(self):
        self.autosave_timer = QTimer(self)
        self.autosave_timer.timeout.connect(self.autosave_all_files)
        self.autosave_timer.start(self.autosave_interval)
        
    def _update_content_display(self):
        if self.tab_widget.count() == 0:
            self.content_stack.setCurrentWidget(self.welcome_widget)
        else:
            self.content_stack.setCurrentWidget(self.tab_container)
    
    def show_status_message(self, message: str, timeout: int = 2000):
        self.status_bar.showMessage(message, timeout)
    
    def apply_theme(self, theme_name: str):
        self.current_theme = theme_name
        colors = THEMES[theme_name]["colors"]
        
        self.check_icon = self._create_check_icon(colors["fg"])
        
        if hasattr(self, 'autosave_action'):
            if self.autosave_action.isChecked():
                self.autosave_action.setIcon(self.check_icon)
            else:
                self.autosave_action.setIcon(QIcon())

        if hasattr(self, 'error_text_action'):
            if self.error_text_visible:
                self.error_text_action.setIcon(self.check_icon)
            else:
                self.error_text_action.setIcon(QIcon())
        
        for action in self.interval_actions:
            if action.data() == self.autosave_interval:
                action.setIcon(self.check_icon)
            else:
                action.setIcon(QIcon())
                
        for action in self.language_actions:
            if (Strings.current_language == Language.ENGLISH and action.text() == Strings.EN["english"]) or \
               (Strings.current_language == Language.RUSSIAN and action.text() == Strings.RU["russian"]):
                action.setIcon(self.check_icon)
            else:
                action.setIcon(QIcon())

        accent = colors["autosave_on"] 

        self.folding_toggle.bg_color = colors.get("bg", QColor("#333"))
        self.folding_toggle.active_color = accent
        self.folding_toggle.handle_color = colors.get("status_bg")
        
        self.folding_toggle.update()
        
        self.separator.setStyleSheet(f"""
            QFrame {{
                background-color: {colors['status_border'].name()};
                margin: 3px 0px;
            }}
        """)
        
        self.check_icon = self._create_check_icon(colors["fg"])
        
        icon = create_svg_icon(APP_ICON_PATH, colors['title_fg'])
        if icon and not icon.isNull():
            self.setWindowIcon(icon)
            self.title_bar.update_icon(icon)
        
        self.title_bar.update_style()
        
        content_widget = self.findChild(QWidget, "contentWidget")
        if content_widget:
            content_widget.setStyleSheet(f"""
                QWidget#contentWidget {{
                    background-color: {colors['title_bg'].name()};
                }}
            """)
        
        self.tab_container.setStyleSheet(f"""
            QWidget#tabContainer {{
                background-color: {THEMES[self.current_theme]['colors']['title_bg'].name()};
            }}
        """)

        status_text_color = QColor(colors['status_bg']).lighter(300).name()
        
        status_font_family = "Consolas"
        status_font_size = "10pt"

        self.status_bar.setStyleSheet(f"""
            QStatusBar#statusBar {{
                background-color: {colors['status_bg'].name()};
                border-top: 1px solid {colors['status_border'].name()};
                border-bottom-left-radius: 7px;
                border-bottom-right-radius: 7px;
                color: {status_text_color};
                font-family: {status_font_family};
                font-size: {status_font_size};
            }}
            QStatusBar::item {{
                border: none;
            }}
            QStatusBar QLabel {{
                color: {status_text_color};
                font-family: {status_font_family};
                font-size: {status_font_size};
                padding: 0px 10px;
            }}
        """)
        
        if hasattr(self, 'separator'):
            self.separator.setStyleSheet(f"""
                background-color: {colors['status_border'].name()}; 
                margin: 4px 0px;
        """)
        
        scrollbar_style = f"""
            QScrollBar:vertical {{
                background-color: {colors['margin_bg'].name()};
                width: 10px;
            }}
            QScrollBar::handle:vertical {{
                background-color: {colors['title_bg_darker'].name()};
                min-height: 30px;
            }}
            QScrollBar::handle:vertical:hover {{
                background-color: {colors['title_bg_darker'].darker(120).name()};
            }}
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
                border: none;
                background: none;
            }}
            QScrollBar:horizontal {{
                background-color: {colors['margin_bg'].name()};
                height: 10px;
            }}
            QScrollBar::handle:horizontal {{
                background-color: {colors['title_bg_darker'].name()};
                min-width: 30px;
            }}
            QScrollBar::handle:horizontal:hover {{
                background-color: {colors['title_bg_darker'].darker(120).name()};
            }}
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
                border: none;
                background: none;
            }}
        """
        
        menu_style = f"""
            QMenu {{
                background-color: {colors['bg'].name()};
                border: 1px solid {colors['status_border'].name()};
                border-radius: 8px;
                padding: 4px 0px;
                font-family: Consolas;
                font-size: 11pt;
            }}
            QMenu::item {{
                padding: 6px 35px 6px 25px;
                background-color: transparent;
                color: {colors['fg'].name()};
                font-family: Consolas;
            }}
            QMenu::item:selected {{
                background-color: {colors['title_bg'].name()};
                color: {colors['selection_fg'].name()};
                border-radius: 7px;
                margin: 0px 4px;
                font-family: Consolas;
            }}
            QMenu::item:disabled {{
                color: {colors['comment'].name()};
                font-family: Consolas;
            }}
            QMenu::separator {{
                height: 2px;
                background-color: {colors['title_bg'].name()};
                margin: 3px 12px;
            }}
            QMenu::indicator {{
                width: 0px
            }}
            QMenu::indicator:checked {{
                image: url(data/check.svg);
            }}
            QMenu::icon {{
                left: 5px;
                right: auto;
                position: absolute;
                width: 16px;
                height: 16px;
            }}
            QMenuBar {{
                background-color: transparent;
                font-family: Consolas;
                font-size: 11pt;
            }}
            QMenuBar::item {{
                background-color: transparent;
                color: {colors['title_fg'].name()};
                padding: 6px 10px;
                font-family: Consolas;
            }}
            QMenuBar::item:selected {{
                background-color: {colors['title_bg_darker'].name()};
                border-radius: 4px;
                font-family: Consolas;
            }}
            QMenuBar::item:pressed {{
                background-color: {colors['selection_bg'].name()};
                font-family: Consolas;
            }}
        """
        
        QApplication.instance().setStyleSheet(scrollbar_style + menu_style)
        
        for widget in QApplication.instance().allWidgets():
            if isinstance(widget, (QsciScintilla, QTabWidget)):
                widget.style().unpolish(widget)
                widget.style().polish(widget)
        
        self.tab_widget.setStyleSheet(f"""
            QTabWidget::pane {{
                background-color: {colors['bg'].name()};
                border: none;
            }}
            QTabBar::tab {{
                background-color: {colors['margin_bg'].name()};
                color: {colors['fg'].name()};
                border: 1px solid {colors['title_bg_darker'].name()};
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
                min-width: 16ex;
                padding: 5px;
                margin-left: 2px;
                font-family: Consolas;
            }}
            QTabBar::tab:selected {{
                background-color: {colors['bg'].name()};
                border-bottom-color: {colors['bg'].name()};
            }}
            QTabBar::tab:hover:!selected {{
                background-color: {colors['title_border'].name()};
            }}
            QTabBar QToolButton {{
                background-color: {colors['margin_bg'].name()};
                border: 1px solid {colors['status_border'].name()};
                border-radius: 3px;
                font-family: Consolas;
            }}
            QTabBar QToolButton:hover {{
                background-color: {colors['selection_bg'].name()};
            }}
        """)
        
        self._update_all_editor_themes()
        self._update_status_labels()
        self._update_application_palette()
        self.tab_widget.update_all_close_buttons()

        self.content_stack.setStyleSheet(f"""
            QStackedWidget#contentStack {{
                background-color: {colors['bg'].name()};
            }}
        """)
        self.content_stack.update()
        
        self.repaint()
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                editor.repaint()
        
        self.welcome_widget.update()
        self.file_explorer.update_theme()
        
    def select_theme(self, theme_name: str):
        self.apply_theme(theme_name)
        
        colors = THEMES[theme_name]["colors"]
        self.check_icon = self._create_check_icon(colors["fg"])
        
        for action in self.theme_actions:
            if action.text() == theme_name:
                action.setChecked(True)
                action.setIcon(self.check_icon)
            else:
                action.setChecked(False)
                action.setIcon(QIcon())
        
        if hasattr(self, 'autosave_action'):
            if self.autosave_action.isChecked():
                self.autosave_action.setIcon(self.check_icon)
            else:
                self.autosave_action.setIcon(QIcon())
        
        for action in self.interval_actions:
            if action.data() == self.autosave_interval:
                action.setIcon(self.check_icon)
            else:
                action.setIcon(QIcon())
                
        self.show_status_message(Strings.get("theme_switched").format(theme_name), 2000)
        
    def _update_all_editor_themes(self):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                self._update_editor_theme(editor)

    def _update_editor_theme(self, editor: CustomScintilla):
        colors = THEMES[self.current_theme]["colors"]
        
        text = editor.text()
        line, col = editor.getCursorPosition()
        scroll_pos = editor.SendScintilla(editor.SCI_GETFIRSTVISIBLELINE)
        
        # Сохраняем тип текущего лексера
        current_lexer = editor.lexer()
        is_python = isinstance(current_lexer, QsciLexerPython)
        is_cpp = isinstance(current_lexer, QsciLexerCPP)
        
        if is_python:
            new_lexer = QsciLexerPython(editor)
            editor._setup_python_lexer(new_lexer)
        elif is_cpp:
            new_lexer = QsciLexerCPP(editor)
            editor._setup_cpp_lexer(new_lexer)
        else:
            # Для Lumen и других файлов
            new_lexer = TwistLangLexer(editor, self.current_theme, self.global_font_size)
        
        editor.setLexer(new_lexer)
        
        editor.setText(text)
        editor.setCursorPosition(line, col)
        editor.SendScintilla(editor.SCI_SETFIRSTVISIBLELINE, scroll_pos)
        
        # Общие настройки для всех типов редакторов
        editor.setCaretForegroundColor(colors["caret"])
        editor.setCaretLineBackgroundColor(colors["caret_line"])
        editor.setMarginsBackgroundColor(colors["margin_bg"])
        editor.setMarginsForegroundColor(colors["margin_fg"])
        editor.setSelectionBackgroundColor(colors["selection_bg"])
        editor.setSelectionForegroundColor(colors["selection_fg"])
        
        # Настройка индикаторов для подсветки скобок
        editor.setBraceMatching(QsciScintilla.BraceMatch.NoBraceMatch)
        
        editor.SendScintilla(editor.SCI_INDICSETFORE, 1, colors["brace_fg"])
        editor.SendScintilla(editor.SCI_INDICSETSTYLE, 1, QsciScintilla.INDIC_FULLBOX)
        editor.SendScintilla(editor.SCI_INDICSETALPHA, 1, 30)
        
        editor.SendScintilla(editor.SCI_INDICSETFORE, 2, colors["brace_bg"])
        editor.SendScintilla(editor.SCI_INDICSETSTYLE, 2, QsciScintilla.INDIC_FULLBOX)
        editor.SendScintilla(editor.SCI_INDICSETALPHA, 2, 30)
        
        # Настройка маркеров ошибок (только для Lumen файлов)
        if not is_python and not is_cpp:
            editor.setIndicatorForegroundColor(colors["error"], CustomScintilla.INDICATOR_ERROR)
            editor.setIndicatorForegroundColor(colors["warning"], CustomScintilla.WARNING_ERROR)
            
            error_bg = QColor(colors["error"])
            error_bg.setAlpha(40)
            editor.setMarkerBackgroundColor(error_bg, CustomScintilla.ERROR_LINE_MARKER)
            
            warning_bg = QColor(colors["warning"])
            warning_bg.setAlpha(40)
            editor.setMarkerBackgroundColor(warning_bg, CustomScintilla.WARNING_LINE_MARKER)

            echo_bg = QColor(colors["echo"])
            echo_bg.setAlpha(40)
            editor.setMarkerBackgroundColor(echo_bg, CustomScintilla.ECHO_LINE_MARKER)
        
        # Установка шрифта для марджинов
        editor.setMarginsFont(editor.font())
        self._update_margin_width(editor)
        
        # Настройка автодополнения только для Lumen лексера
        if not is_python and not is_cpp:
            self._setup_autocompletion_icons(editor, new_lexer)
        
        # Настройка цветов для сворачивания кода
        editor.setFoldMarginColors(colors["margin_bg"], colors["margin_bg"])
        
        if hasattr(editor, 'error_tooltip') and editor.error_tooltip.isVisible():
            current_text = editor.error_tooltip.label.text()
            editor.error_tooltip.set_text(current_text, colors, editor.error_tooltip._last_error_type)
        self._apply_global_font_to_all_editors()
        editor.repaint()

    def restart_language_server(self):
        editor = self.current_editor()
        if not editor or not editor.filename:
            self.show_status_message(Strings.get("no_file_open_ls"), 2000)
            return
            
        filename = editor.filename
        
        self.stop_language_server(filename)
        
        self.start_language_server(filename)
        
        self.show_status_message(Strings.get("ls_restarted").format(os.path.basename(filename)), 2000)
            
    def _update_application_palette(self):
        app = QApplication.instance()
        colors = THEMES[self.current_theme]["colors"]
        
        palette = app.palette()
        
        palette.setColor(palette.ColorRole.Window, colors["bg"])
        palette.setColor(palette.ColorRole.WindowText, colors["fg"])
        palette.setColor(palette.ColorRole.Base, colors["margin_bg"])
        palette.setColor(palette.ColorRole.AlternateBase, colors["caret_line"])
        palette.setColor(palette.ColorRole.ToolTipBase, colors["selection_bg"])
        palette.setColor(palette.ColorRole.ToolTipText, colors["fg"])
        palette.setColor(palette.ColorRole.Text, colors["fg"])
        palette.setColor(palette.ColorRole.Button, colors["selection_bg"])
        palette.setColor(palette.ColorRole.ButtonText, colors["fg"])
        palette.setColor(palette.ColorRole.BrightText, colors["error"])
        palette.setColor(palette.ColorRole.Link, colors["function"])
        palette.setColor(palette.ColorRole.Highlight, colors["selection_bg"])
        palette.setColor(palette.ColorRole.HighlightedText, colors["selection_fg"])
        
        app.setPalette(palette)
        
    def _get_resize_direction(self, pos: QPoint) -> Optional[str]:
        rect = self.rect()
        margin = RESIZE_MARGIN
        
        if pos.y() <= self.title_bar.height() + margin:
            return None
            
        left = pos.x() <= margin
        right = pos.x() >= rect.width() - margin
        top = pos.y() <= margin
        bottom = pos.y() >= rect.height() - margin

        
        
        if left and top:
            return "topleft"
        elif right and top:
            return "topright"
        elif left and bottom:
            return "bottomleft"
        elif right and bottom:
            return "bottomright"
        elif left:
            return "left"
        elif right:
            return "right"
        elif top:
            return "top"
        elif bottom:
            return "bottom"
            
        return None
        
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            pos = event.pos()
            
            if pos.y() <= self.title_bar.height():
                super().mousePressEvent(event)
                return
                
            direction = self._get_resize_direction(pos)
            if direction:
                self.resizing = True
                self.resize_direction = direction
                self.resize_start_pos = event.globalPosition().toPoint()
                self.resize_start_geometry = self.geometry()
                event.accept()
                
                return
                
        super().mousePressEvent(event)
        
    def mouseMoveEvent(self, event):
        if self.resizing and self.resize_direction:
            current_pos = event.globalPosition().toPoint()
            dx = current_pos.x() - self.resize_start_pos.x()
            dy = current_pos.y() - self.resize_start_pos.y()
            
            new_geo = QRect(self.resize_start_geometry)
            
            if "left" in self.resize_direction:
                new_geo.setLeft(new_geo.left() + dx)
            if "right" in self.resize_direction:
                new_geo.setRight(new_geo.right() + dx)
            if "top" in self.resize_direction:
                new_geo.setTop(new_geo.top() + dy)
            if "bottom" in self.resize_direction:
                new_geo.setBottom(new_geo.bottom() + dy)
                
            if new_geo.width() < WINDOW_MIN_WIDTH:
                if "left" in self.resize_direction:
                    new_geo.setLeft(new_geo.right() - WINDOW_MIN_WIDTH)
                else:
                    new_geo.setRight(new_geo.left() + WINDOW_MIN_WIDTH)
            if new_geo.height() < WINDOW_MIN_HEIGHT:
                if "top" in self.resize_direction:
                    new_geo.setTop(new_geo.bottom() - WINDOW_MIN_HEIGHT)
                else:
                    new_geo.setBottom(new_geo.top() + WINDOW_MIN_HEIGHT)
                    
            self.setGeometry(new_geo)
            event.accept()
        else:
            super().mouseMoveEvent(event)
                
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton and self.resizing:
            self.resizing = False
            self.resize_direction = None
            self.setCursor(Qt.CursorShape.ArrowCursor)
            event.accept()
        else:
            super().mouseReleaseEvent(event)
            
    def _toggle_maximize(self):
        if self.title_bar.is_maximized:
            self.showNormal()
        else:
            self.showMaximized()
        self.title_bar.is_maximized = not self.title_bar.is_maximized
        
    def current_editor(self) -> Optional[CustomScintilla]:
        return self.tab_widget.currentWidget()
        
    def new_file(self):
        self._add_editor_tab(CustomScintilla(), filename=None, title="Untitled")
        self._update_content_display()
        
    def open_file_dialog(self):
        filename, _ = QFileDialog.getOpenFileName(
            self, Strings.get("open"), "", "TwistLang Files (*.lumen);;All Files (*.*)"
        )
        if filename:
            self._show_file_explorer_for_path(filename)
            self.open_file(filename)
            
    # В методе open_file класса TwistLangEditor, после чтения файла:
    def open_file(self, filename: str):
        # Проверяем, открыт ли уже файл
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and getattr(editor, 'filename', None) == filename:
                self.tab_widget.setCurrentIndex(i)
                return
                
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                content = f.read()
                
            editor = CustomScintilla()
            editor.set_main_window(self)  # Важно установить main_window перед setup_lexer
            editor.setText(content)
            editor.setModified(False)
            editor.filename = filename
            
            # Установить правильный лексер в зависимости от расширения
            lexer = editor.setup_lexer_for_file(filename)
            editor.setLexer(lexer)
            
            self._add_editor_tab(editor, filename=filename, title=os.path.basename(filename))
            self.show_status_message(Strings.get("opened_file").format(filename), 2000)
            self._update_status_labels()
            self._apply_global_font_to_all_editors()
            
            # Показываем файловое дерево при первом открытии файла
            if not self.first_file_opened:
                self._show_file_explorer_for_path(filename)
                self.first_file_opened = True
                
            # Запускаем LS только для .lumen файлов
            if filename.endswith('.lumen'):
                self.start_language_server(filename)
            
        except Exception as e:
            QMessageBox.critical(self, Strings.get("file_not_found"), Strings.get("could_not_open").format(e))
            
    def open_folder_dialog(self):
        folder = QFileDialog.getExistingDirectory(
            self,
            Strings.get("select_folder"),
            "",
            QFileDialog.Option.ShowDirsOnly
        )
        if folder:
            self._show_file_explorer_for_path(folder)
            self.first_file_opened = True
            
    def save_current_file(self):
        editor = self.current_editor()
        if not editor:
            return
            
        filename = getattr(editor, 'filename', None)
        if filename:
            self.save_editor(editor, filename)
        else:
            self.save_editor_as(editor)
            
    def save_current_file_as(self):
        self.save_editor_as(self.current_editor())
        
    def save_editor_as(self, editor: CustomScintilla):
        filename, _ = QFileDialog.getSaveFileName(
            self, Strings.get("save_as"), "", "Lumen Files (*.lumen);;All Files (*.*)"
        )
        if filename:
            if not filename.endswith('.lumen'):
                filename += '.lumen'
            self.save_editor(editor, filename)
            editor.filename = filename
            index = self.tab_widget.indexOf(editor)
            if index != -1:
                self.tab_widget.setTabText(index, os.path.basename(filename) + 
                                          ('*' if editor.isModified() else ''))
            
            # При сохранении нового файла показываем файловое дерево в папке файла
            if not self.first_file_opened:
                self._show_file_explorer_for_path(filename)
                self.first_file_opened = True
            elif not self.explorer_visible:
                # Если дерево скрыто, но первый файл уже был открыт, все равно показываем
                self._show_file_explorer_for_path(filename)

    def save_editor(self, editor: CustomScintilla, filename: str):
        try:
            text = editor.text()
            cleaned = text.replace('\x00', '')
            normalized = cleaned.replace('\r\n', '\n').replace('\r', '\n')
            
            with open(filename, 'w', encoding='utf-8', newline='') as f:
                f.write(normalized)
                
            editor.setModified(False)
            editor.last_save_time = datetime.now()
            self.show_status_message(Strings.get("saved_file").format(filename), 2000)
            
        except Exception as e:
            QMessageBox.critical(self, Strings.get("file_not_found"), Strings.get("could_not_save").format(e))
            
    def close_current_tab(self):
        index = self.tab_widget.currentIndex()
        if index >= 0:
            self.tab_widget.close_tab(index)
            
    def close_all_tabs(self):
        while self.tab_widget.count() > 0:
            self.tab_widget.close_tab(0)
        self._update_content_display()
            
    def _add_editor_tab(self, editor: CustomScintilla, filename: Optional[str], title: str):
        editor.set_main_window(self)
        index = self.tab_widget.addTab(editor, title)
        self.tab_widget.setCurrentIndex(index)
        
        editor.modificationChanged.connect(lambda modified: self._update_tab_title(editor, modified))
        editor.goto_definition_requested.connect(self.open_include_file)
        
        self._setup_editor_widget(editor)
        
        # Применяем тему (она уже установит правильный лексер)
        self._update_editor_theme(editor)
        
        editor.set_folding_visible(self.folding_toggle.isChecked())
        
        # Запускаем LS только для .lumen файлов
        if filename and filename.endswith('.lumen'):
            self.start_language_server(filename)
        
        editor.set_error_text_visible(self.error_text_visible)
        
        self._update_content_display()
        
    def _setup_editor_widget(self, editor: CustomScintilla):
        font = get_safe_monospace_font("Consolas", self.global_font_size)
        editor.setFont(font)
        editor.setMarginsFont(font)
        self._update_margin_width(editor)
        
        # Лексер уже установлен в open_file или _update_editor_theme
        lexer = editor.lexer()
        
        # Настройка автодополнения только для Lumen лексера
        if isinstance(lexer, TwistLangLexer):
            self._setup_autocompletion_icons(editor, lexer)
        
    def _setup_autocompletion_icons(self, editor: CustomScintilla, lexer: TwistLangLexer):
        colors = THEMES[self.current_theme]["colors"]
        size = lexer.font_size
        
        icons = {
            1: ("K", colors["keyword"]),
            2: ("M", colors["modifier"]),
            3: ("T", colors["type"]),
            4: ("L", colors["literal"]),
            5: ("D", colors["directive"]),
            6: ("S", colors["special"]),
            7: ("F", colors["function"])
        }
        
        for img_id, (text, color) in icons.items():
            pixmap = self._create_type_pixmap(text, color, size)
            editor.registerImage(img_id, pixmap)
            
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
            if word == '#include':
                api.add(word + " : Include another file: #include \"filename\" ?5")
            elif word == "#define":
                api.add(word + " : Define a macro: #define NAME = value ?5")
            elif word == "#macro":
                api.add(word + " : Define a macro function ?5")
                
        api.prepare()
        lexer.setAPIs(api)
        
    def _create_type_pixmap(self, text: str, color: QColor, size: int) -> QPixmap:
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setPen(QPen(color))
        painter.setFont(QFont("Arial", size, QFont.Weight.Bold))
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, text)
        painter.end()
        
        return pixmap
        
    def _update_margin_width(self, editor: CustomScintilla):
        lines = max(editor.lines(), 1)
        digits = len(str(lines))
        editor.setMarginWidth(0, "9" * (digits + 2))
        
    def _update_tab_title(self, editor: CustomScintilla, modified: bool):
        index = self.tab_widget.indexOf(editor)
        if index == -1:
            return
            
        base = self.tab_widget.tabText(index)
        if base.endswith('*'):
            base = base[:-1]
        self.tab_widget.setTabText(index, base + ('*' if modified else ''))
        
    def start_language_server(self, file_path: str):
        if not file_path:
            return
            
        if file_path in self.ls_processes:
            proc = self.ls_processes[file_path]
            if proc.poll() is None:
                print(f"LS already running for {os.path.basename(file_path)}")
                return
            else:
                del self.ls_processes[file_path]
            
        try:
            ls_path = os.path.join('bin', 'lumen-ls')
            if platform.system() == "Windows" and not os.path.exists(ls_path):
                ls_path += '.exe'
                
            process = subprocess.Popen(
                [ls_path, '--file', file_path, '-d'],
            )
            
            self.ls_processes[file_path] = process
            print(f"Started LS for {os.path.basename(file_path)}")
            self._update_status_labels()
            
            editor = self._find_editor_by_filename(file_path)
            if editor:
                QTimer.singleShot(500, lambda: self._check_file_errors(editor))
                
        except Exception as e:
            print(f"Failed to start LS for {file_path}: {e}")
            
    def stop_language_server(self, file_path: str):
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
                    process.wait()
            print(f"Stopped LS for {os.path.basename(file_path)}")
        except Exception as e:
            print(f"Failed to stop LS for {file_path}: {e}")
        finally:
            if file_path in self.ls_processes:
                del self.ls_processes[file_path]
            self._update_status_labels()

    def closeEvent(self, event):
        for file_path in list(self.ls_processes.keys()):
            self.stop_language_server(file_path)

        QApplication.processEvents()

        super().closeEvent(event)
            
    def stop_all_language_servers(self):
        for file_path in list(self.ls_processes.keys()):
            self.stop_language_server(file_path)
            
    def _find_editor_by_filename(self, filename: str) -> Optional[CustomScintilla]:
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and editor.filename == filename:
                return editor
        return None
        
    def _get_error_filename(self, file_path: str) -> Optional[str]:
        if not file_path:
            return None
        base = os.path.splitext(os.path.basename(file_path))[0]
        return f"dbg/{base}_ls.dbg"
        
    def _parse_error_file(self, error_file_path: str) -> List[tuple]:
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
                        message = unescape_message(match.group(6).strip())
                        
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
        
    def _check_file_errors(self, editor: CustomScintilla):
        if not editor or not editor.filename:
            return
            
        error_filename = self._get_error_filename(editor.filename)
        if error_filename and os.path.exists(error_filename):
            errors = self._parse_error_file(error_filename)
            editor.set_errors(errors)
        else:
            editor.clear_errors()
            
        self._update_status_labels()
        
    def check_current_file_errors(self):
        editor = self.current_editor()
        if editor and editor.filename:
            self._check_file_errors(editor)
            self.show_status_message(Strings.get("error_check_completed"), 2000)
        
    def run_current_file(self):
        editor = self.current_editor()
        if not editor:
            return
            
        filename = getattr(editor, 'filename', None)
        if not filename:
            reply = QMessageBox.question(
                self,
                Strings.get("save_changes"),
                Strings.get("save_before_run"),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            if reply == QMessageBox.StandardButton.Yes:
                self.save_current_file()
                filename = getattr(editor, 'filename', None)
                if not filename:
                    return
            else:
                return
                
        self.save_editor(editor, filename)
        
        try:
            system = platform.system()
            if system == "Windows":
                subprocess.Popen(f'start cmd /k bin\\lumenc --file "{filename}"', shell=True)
                self.status_bar.showMessage(Strings.get("running_file").format(os.path.basename(filename)), 3000)
            else:
                QMessageBox.information(self, Strings.get("not_supported"),
                                       Strings.get("run_manually").format(filename))
        except Exception as e:
            QMessageBox.critical(self, Strings.get("file_not_found"), Strings.get("failed_to_run").format(e))
            
    def open_include_file(self, path: str, line: int):
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
            QMessageBox.warning(self, Strings.get("file_not_found"), Strings.get("cannot_find_include").format(path))
            
    def goto_include_under_cursor(self):
        editor = self.current_editor()
        if not editor:
            return
            
        line, _ = editor.getCursorPosition()
        if editor.is_include_line(line):
            path = editor.extract_include_path(line)
            if path:
                self.open_include_file(path, line)
            else:
                self.show_status_message(Strings.get("no_valid_include_path"), 2000)
        else:
            self.show_status_message(Strings.get("not_include_line"), 2000)
            
    def goto_next_error(self):
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.show_status_message(Strings.get("no_errors_status"), 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        for error in editor.errors:
            if error.line > current_line or (error.line == current_line and error.start_col > current_col):
                editor.setCursorPosition(error.line, error.start_col)
                editor.ensureLineVisible(error.line)
                self.show_status_message(Strings.get("error_at_line").format(error.line + 1), 2000)
                return
                
        self.show_status_message(Strings.get("no_more_errors"), 2000)
        
    def goto_prev_error(self):
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.show_status_message(Strings.get("no_errors_status"), 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        for error in reversed(editor.errors):
            if error.line < current_line or (error.line == current_line and error.start_col < current_col):
                editor.setCursorPosition(error.line, error.start_col)
                editor.ensureLineVisible(error.line)
                self.show_status_message(Strings.get("error_at_line").format(error.line + 1), 2000)
                return
                
        self.show_status_message(Strings.get("no_previous_errors"), 2000)
        
    def clear_all_errors(self):
        editor = self.current_editor()
        if editor:
            editor.clear_errors()
            self._update_status_labels()
            self.show_status_message(Strings.get("errors_cleared"), 2000)
            
    def zoom_in(self):
        if self.global_font_size < MAX_FONT_SIZE:
            self.global_font_size += 1
            self._apply_global_font_to_all_editors()
            self.show_status_message(Strings.get("font_size").format(self.global_font_size), 2000)
            
    def zoom_out(self):
        if self.global_font_size > MIN_FONT_SIZE:
            self.global_font_size -= 1
            self._apply_global_font_to_all_editors()
            self.show_status_message(Strings.get("font_size").format(self.global_font_size), 2000)
            
    def _apply_global_font_to_all_editors(self):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                lexer = editor.lexer()
                
                # Обновляем шрифт в зависимости от типа лексера
                if isinstance(lexer, TwistLangLexer):
                    lexer.set_font_size(self.global_font_size)
                    editor.setFont(lexer.font(lexer.STYLE_DEFAULT))
                    editor.setMarginsFont(lexer.font(lexer.STYLE_DEFAULT))
                    self._setup_autocompletion_icons(editor, lexer)
                elif isinstance(lexer, QsciLexerPython):
                    font = get_safe_monospace_font("Consolas", self.global_font_size)
                    lexer.setFont(font)
                    editor.setFont(font)
                    editor.setMarginsFont(font)
                    # Обновить курсив для комментариев
                    italic_font = QFont(font)
                    italic_font.setItalic(True)
                    lexer.setFont(italic_font, QsciLexerPython.Comment)
                elif isinstance(lexer, QsciLexerCPP):
                    font = get_safe_monospace_font("Consolas", self.global_font_size)
                    lexer.setFont(font)
                    editor.setFont(font)
                    editor.setMarginsFont(font)
                    # Обновить курсив для комментариев
                    italic_font = QFont(font)
                    italic_font.setItalic(True)
                    lexer.setFont(italic_font, QsciLexerCPP.Comment)
                    lexer.setFont(italic_font, QsciLexerCPP.CommentLine)
                    lexer.setFont(italic_font, QsciLexerCPP.CommentDoc)
                
                self._update_margin_width(editor)
                editor.recolor()
                editor.repaint()
                    
    def _toggle_autosave(self, checked: bool):
        if checked:
            self.autosave_timer.start(self.autosave_interval)
            self.show_status_message(Strings.get("auto_save_enabled"), 2000)
            colors = THEMES[self.current_theme]["colors"]
            self.autosave_action.setIcon(self._create_check_icon(colors["fg"]))
        else:
            self.autosave_timer.stop()
            self.show_status_message(Strings.get("auto_save_disabled"), 2000)
            self.autosave_action.setIcon(QIcon())
        
        self._update_status_labels()

    def _set_autosave_interval(self, ms: int):
        self.autosave_interval = ms
        if self.autosave_action.isChecked():
            self.autosave_timer.start(ms)
        
        self._update_status_labels()
        
        interval_menu = self.sender().parent()
        colors = THEMES[self.current_theme]["colors"]
        check_icon = self._create_check_icon(colors["fg"])
        
        for action in interval_menu.actions():
            if action.data() == ms:
                action.setChecked(True)
                action.setIcon(check_icon)
            elif action.data() is not None:
                action.setChecked(False)
                action.setIcon(QIcon())
        
        for action in self.interval_actions:
            if action.data() == ms:
                action.setIcon(check_icon)
            else:
                action.setIcon(QIcon())
        
        self.show_status_message(Strings.get("auto_save_interval_set").format(ms//1000), 2000)
        
    def autosave_all_files(self, manual: bool = False):
        if not self.autosave_action.isChecked() and not manual:
            return
            
        saved_count = 0
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                filename = getattr(editor, 'filename', None)
                if filename and editor.isModified():
                    try:
                        self.save_editor(editor, filename)
                        saved_count += 1
                    except Exception as e:
                        print(f"Auto-save error for {filename}: {e}")
                        
        if saved_count > 0:
            self.autosave_count += 1
            if manual:
                self.show_status_message(Strings.get("manual_save").format(saved_count), 2000)
            else:
                self.show_status_message(Strings.get("auto_saved").format(saved_count, self.autosave_count), 1500)
                colors = THEMES[self.current_theme]["colors"]
                self.autosave_label.setStyleSheet(f"color: {colors['warning'].name()}; padding: 2px 5px;")
                QTimer.singleShot(1000, self._update_status_labels)
        elif manual:
            self.show_status_message(Strings.get("no_files_to_save"), 2000)
            
    def _update_status_labels(self):
        colors = THEMES[self.current_theme]["colors"]
        
        status_bg = colors['status_bg']
        text_color = QColor(status_bg).lighter(300) if status_bg.lightness() < 128 else QColor(status_bg).darker(300)
        
        if self.autosave_action.isChecked():
            self.autosave_label.setText(Strings.get("auto_save_on").format(self.autosave_interval/1000))
        else:
            self.autosave_label.setText(Strings.get("auto_save_off"))
        
        current = self.current_editor()
        if current and len(current.errors) > 0:
            self.error_label.setText(Strings.get("errors_count").format(len(current.errors)))
        else:
            self.error_label.setText(Strings.get("no_errors"))
        
        active_ls = sum(1 for proc in self.ls_processes.values() if proc.poll() is None)
        self.ls_status_label.setText(Strings.get("ls_active").format(active_ls))
        
        font = QFont("Consolas")
        font.setPointSize(9)
        
        self.autosave_label.setFont(font)
        self.error_label.setFont(font)
        self.ls_status_label.setFont(font)
        
        unified_style = f"color: {text_color.name()}; padding: 2px 5px; font-family: Consolas;"
        self.autosave_label.setStyleSheet(unified_style)
        self.error_label.setStyleSheet(unified_style)
        self.ls_status_label.setStyleSheet(unified_style)

    def _on_tab_changed(self, index: int):
        editor = self.tab_widget.widget(index)
        if not isinstance(editor, CustomScintilla):
            return
            
        self.current_file = editor.filename
        self._update_status_labels()
        
        if self.current_file:
            self._check_file_errors(editor)
            
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            
    def dropEvent(self, event):
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if os.path.isfile(file_path):
                self.open_file(file_path)
        event.acceptProposedAction()


# =============================================================================
# MAIN ENTRY POINT
# =============================================================================

def main():
    """Application entry point"""
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    app.setAttribute(Qt.ApplicationAttribute.AA_DontUseNativeMenuBar, True)

    default_font = QFont("Segoe UI", 10)
    if not default_font.exactMatch():
        default_font = QFont("Arial", 10)
    app.setFont(default_font)
    
    colors = THEMES[DEFAULT_THEME]["colors"]
    palette = app.palette()
    palette.setColor(palette.ColorRole.Window, colors["bg"])
    palette.setColor(palette.ColorRole.WindowText, colors["fg"])
    palette.setColor(palette.ColorRole.Base, colors["margin_bg"])
    palette.setColor(palette.ColorRole.AlternateBase, colors["caret_line"])
    palette.setColor(palette.ColorRole.ToolTipBase, colors["selection_bg"])
    palette.setColor(palette.ColorRole.ToolTipText, colors["fg"])
    palette.setColor(palette.ColorRole.Text, colors["fg"])
    palette.setColor(palette.ColorRole.Button, colors["selection_bg"])
    palette.setColor(palette.ColorRole.ButtonText, colors["fg"])
    palette.setColor(palette.ColorRole.BrightText, colors["error"])
    palette.setColor(palette.ColorRole.Link, colors["function"])
    palette.setColor(palette.ColorRole.Highlight, colors["selection_bg"])
    palette.setColor(palette.ColorRole.HighlightedText, colors["selection_fg"])
    app.setPalette(palette)
    
    editor = TwistLangEditor()
    editor.show()
    
    app.aboutToQuit.connect(editor.stop_all_language_servers)
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()