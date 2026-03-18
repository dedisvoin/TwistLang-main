import sys
import os
import subprocess
import platform
import shutil
import re
from datetime import datetime

from PyQt6.QtGui import QPainterPath
from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QMenu, QStatusBar,
    QPushButton, QLabel, QTabWidget, QTabBar, QToolButton,
    QFileDialog, QMessageBox, QToolTip, QWidget, QHBoxLayout,
    QVBoxLayout, QFrame
)
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QPainter, QPen, QIcon, QPixmap, QMouseEvent,
)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer, QSize, QPoint
from themes import *

from PyQt6.QtSvg import QSvgRenderer
from PyQt6.QtGui import QPainter, QImage, QPixmap, QColor
from PyQt6.QtCore import QByteArray, Qt

# Текущая активная тема
CURRENT_THEME = "Lumen Classic"  # светлая по умолчанию




def create_colored_svg_icon(svg_path, color):
    """
    Создает иконку из SVG с заданным цветом
    """
    try:
        # Проверяем существование файла
        if not os.path.exists(svg_path):
            print(f"❌ SVG file not found: {svg_path}")
            return None
        
        # Читаем SVG файл
        with open(svg_path, 'r', encoding='utf-8') as f:
            svg_content = f.read()
        
        print(f"✅ SVG file loaded: {svg_path}")
        print(f"   Original SVG size: {len(svg_content)} bytes")
        
        # Заменяем цвета
        color_hex = color.name()
        print(f"   Target color: {color_hex}")
        
        # Проверяем наличие fill атрибутов
        if 'fill="' in svg_content:
            print(f"   Found fill attributes in SVG")
        else:
            print(f"   ⚠️ No fill attributes found in SVG")
        
        svg_content = svg_content.replace('currentColor', color_hex)
        svg_content = svg_content.replace('fill="black"', f'fill="{color_hex}"')
        svg_content = svg_content.replace('stroke="black"', f'stroke="{color_hex}"')
        
        # Создаем renderer
        renderer = QSvgRenderer(QByteArray(svg_content.encode()))
        
        # Проверяем, что renderer валидный
        if not renderer.isValid():
            print("❌ SVG Renderer is not valid")
            return None
        
        print(f"✅ SVG Renderer created successfully")
        print(f"   Default size: {renderer.defaultSize().width()}x{renderer.defaultSize().height()}")
        
        # Создаем pixmap
        pixmap = QPixmap(32, 32)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        # Рисуем SVG
        painter = QPainter(pixmap)
        renderer.render(painter)
        painter.end()
        
        # Проверяем, что pixmap не пустой
        if pixmap.isNull():
            print("❌ Generated pixmap is null")
            return None
        
        
        print(f"   Pixmap size: {pixmap.width()}x{pixmap.height()}")
        
        
        return QIcon(pixmap)
        
    except Exception as e:
        print(f"❌ Error creating SVG icon: {e}")
        import traceback
        traceback.print_exc()
        return None


# ----------------------------------------------------------------------
# Кастомный заголовок окна с меню в стиле Windows 10
# ----------------------------------------------------------------------
class CustomTitleBar(QFrame):
    """Кастомный заголовок окна с встроенным меню в стиле Windows 10"""
    
    windowMinimized = pyqtSignal()
    windowMaximized = pyqtSignal()
    windowClosed = pyqtSignal()
    windowMoved = pyqtSignal(QPoint)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.is_window_maximized = False
        self.parent = parent
        self.setFixedHeight(32)
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        
        # Флаг для перетаскивания окна
        self.dragging = False
        self.drag_position = QPoint()
        
        # Основной layout
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # Иконка приложения и заголовок
        title_widget = QWidget()
        title_layout = QHBoxLayout(title_widget)
        title_layout.setContentsMargins(10, 0, 0, 0)
        title_layout.setSpacing(8)
        
        self.icon_label = QLabel()
        self.icon_label.setPixmap(create_colored_svg_icon(r"data\app_icon.svg", THEMES[CURRENT_THEME]['colors']['fg']).pixmap(16, 16))
        title_layout.addWidget(self.icon_label)
        
        self.title_label = QLabel("Lumen IDE")
        self.title_label.setStyleSheet("font-size: 12px;")
        title_layout.addWidget(self.title_label)
        
        layout.addWidget(title_widget)
        
        # Здесь будет меню
        self.menu_container = QWidget()
        self.menu_layout = QHBoxLayout(self.menu_container)
        self.menu_layout.setContentsMargins(5, 0, 0, 0)
        self.menu_layout.setSpacing(0)
        layout.addWidget(self.menu_container)
        
        layout.addStretch()
        
        # Кнопки управления окном в стиле Windows 10
        self.minimize_btn = QToolButton()
        self.minimize_btn.setFixedSize(46, 32)
        self.minimize_btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.minimize_btn.clicked.connect(self.windowMinimized.emit)
        # Отрисовываем иконку свернуть
        self.minimize_btn.paintEvent = lambda event: self.paint_minimize_icon(event, self.minimize_btn)
        layout.addWidget(self.minimize_btn)

        self.maximize_btn = QToolButton()
        self.maximize_btn.setFixedSize(46, 32)
        self.maximize_btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        
        # ИСПРАВЛЕНО: Правильная функция для обработки клика
        def on_maximize_clicked():
            # Всегда эмитим сигнал, независимо от текущего состояния
            # Окно само переключится между максимизированным и нормальным состоянием
            self.windowMaximized.emit()
            # Не меняем флаг здесь, так как окно еще не изменило состояние
        
        self.maximize_btn.clicked.connect(on_maximize_clicked)
        # Отрисовываем иконку развернуть/восстановить
        self.maximize_btn.paintEvent = lambda event: self.paint_maximize_icon(event, self.maximize_btn)
        layout.addWidget(self.maximize_btn)

        self.close_btn = QToolButton()
        self.close_btn.setFixedSize(46, 32)
        self.close_btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.close_btn.clicked.connect(self.windowClosed.emit)
        # Отрисовываем иконку закрыть
        self.close_btn.paintEvent = lambda event: self.paint_close_icon(event, self.close_btn)
        layout.addWidget(self.close_btn)
        
        # Устанавливаем стиль
        self.update_style()

    def update_icon(self, icon):
        """Обновить иконку в заголовке"""
        if icon and not icon.isNull():
            self.icon_label.setPixmap(icon.pixmap(16, 16))

    def set_maximized_state(self, maximized):
        """Установить состояние максимизации и обновить иконку"""
        self.is_window_maximized = maximized
        self.maximize_btn.update()  # Обновляем отрисовку кнопки

    def paint_minimize_icon(self, event, btn):
        """Отрисовка иконки свернуть"""
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Получаем цвета из текущей темы
        if hasattr(self.parent, 'current_theme'):
            colors = THEMES[self.parent.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        # Рисуем фон при наведении
        if btn.underMouse():
            painter.fillRect(btn.rect(), QColor(255, 255, 255, 30))
        
        # Рисуем иконку (горизонтальная линия)
        painter.setPen(QPen(colors['title_fg'], 1))
        x = btn.width() // 2 - 7
        y = btn.height() // 2
        painter.drawLine(x, y, x + 12, y)
        
        painter.end()



    def paint_maximize_icon(self, event, btn):
        """Отрисовка иконки развернуть/восстановить"""
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Получаем цвета из текущей темы
        if hasattr(self.parent, 'current_theme'):
            colors = THEMES[self.parent.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        # Рисуем фон при наведении
        if btn.underMouse():
            painter.fillRect(btn.rect(), QColor(255, 255, 255, 30))
        
        # Определяем, максимизировано ли окно
        is_maximized = self.is_window_maximized
        
        painter.setPen(QPen(colors['title_fg'], 1))
        
        if is_maximized:
            # Иконка восстановления (два перекрывающихся квадрата)
            # Внешний квадрат
            x1 = btn.width() // 2 - 8 + 2
            y1 = btn.height() // 2 - 2
            painter.drawRect(x1, y1, 8, 8)
            
            # Внутренний квадрат (смещенный)
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
            # Иконка развернуть (один квадрат)
            x = btn.width() // 2 - 4
            y = btn.height() // 2 - 4
            painter.drawRect(x, y, 8, 8)
        
        painter.end()

    def paint_close_icon(self, event, btn):
        """Отрисовка иконки закрыть со скруглением только правого верхнего угла"""
        painter = QPainter(btn)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Получаем цвета из текущей темы
        if hasattr(self.parent, 'current_theme'):
            colors = THEMES[self.parent.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        # Рисуем фон при наведении со скруглением только правого верхнего угла
        if btn.underMouse():
            rect = btn.rect()
            painter.setBrush(QColor(196, 43, 28))  # #c42b1c
            painter.setPen(Qt.PenStyle.NoPen)
            
            # Создаем путь для прямоугольника со скругленным только правым верхним углом
            path = QPainterPath()
            width = rect.width()
            height = rect.height()
            
            # Начинаем с левого верхнего угла (без скругления)
            path.moveTo(rect.left(), rect.top())
            # Верхняя сторона до правого верхнего угла
            
            # Скругление правого верхнего угла (8px)
            path.arcTo(rect.right() - 14.5, rect.top(), 20, 70, 90, -90)
            # Правая сторона вниз
            path.lineTo(rect.right(), rect.bottom() + 1)
            # Нижняя сторона влево
            path.lineTo(rect.left(), rect.bottom() + 1)
            # Левая сторона вверх
            path.lineTo(rect.left(), rect.top())
            
            painter.drawPath(path)
            
            # Рисуем крестик белым цветом
            painter.setPen(QPen(Qt.GlobalColor.white, 2))
        else:
            # Без фона, только крестик цветом темы
            painter.setPen(QPen(colors['title_fg'], 1.5))
        
        # Рисуем крестик
        x1 = btn.width() // 2 - 5
        y1 = btn.height() // 2 - 5
        x2 = btn.width() // 2 + 5
        y2 = btn.height() // 2 + 5
        
        painter.drawLine(x1, y1, x2, y2)
        painter.drawLine(x2, y1, x1, y2)
        
        painter.end()

    def update_style(self):
        """Обновить стиль в соответствии с темой"""
        if hasattr(self.parent, 'current_theme'):
            colors = THEMES[self.parent.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        # Проверяем, максимизировано ли окно
        is_maximized = self.parent.isMaximized() if self.parent else False
        
        # Устанавливаем закругления только если окно не максимизировано
        border_radius = "7px" if not is_maximized else "0px"
        
        # Базовый стиль для заголовка с закругленными верхними углами
        self.setStyleSheet(f"""
            CustomTitleBar {{
                background-color: {colors['title_bg'].name()};
                border-top-left-radius: {border_radius};
                border-top-right-radius: {border_radius};
            }}
            QLabel {{
                color: {colors['title_fg'].name()};
                background-color: transparent;
            }}
        """)
        
        # Стиль для кнопок меню
        for i in range(self.menu_layout.count()):
            btn = self.menu_layout.itemAt(i).widget()
            if isinstance(btn, QToolButton):
                btn.setStyleSheet(f"""
                    QToolButton {{
                        background-color: transparent;
                        color: {colors['title_fg'].name()};
                        border: none;
                        padding: 5px 12px;
                        font-size: 12px;
                    }}
                    QToolButton:hover {{
                        background-color: {colors['selection_bg'].name()};
                        color: {colors['selection_fg'].name()};
                    }}
                    QToolButton:pressed {{
                        background-color: {colors['selection_bg'].darker(120).name()};
                    }}
                    QToolButton::menu-indicator {{
                        image: none;
                    }}
                """)
        
        # Для кнопок управления окном обновляем их
        self.minimize_btn.update()
        self.maximize_btn.update()
        self.close_btn.update()
        
    def add_menu(self, menu_bar):
        """Добавить меню в заголовок"""
        # Очищаем контейнер
        for i in reversed(range(self.menu_layout.count())):
            widget = self.menu_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
            
        # Добавляем все действия из menu_bar
        for action in menu_bar.actions():
            if action.menu():
                # Это меню
                btn = QToolButton()
                btn.setText(action.text())
                btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
                btn.setMenu(action.menu())
                self.menu_layout.addWidget(btn)
                
        # Обновляем стиль для новых кнопок
        self.update_style()
                
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            # Проверяем, что клик не по кнопке
            pos = event.pos()
            if not (self.minimize_btn.geometry().contains(pos) or 
                    self.maximize_btn.geometry().contains(pos) or 
                    self.close_btn.geometry().contains(pos)):
                self.dragging = True
                self.drag_position = event.globalPosition().toPoint() - self.parent.frameGeometry().topLeft()
                event.accept()
                return
        super().mousePressEvent(event)
            
    def mouseMoveEvent(self, event):
        if self.dragging and event.buttons() & Qt.MouseButton.LeftButton:
            self.parent.move(event.globalPosition().toPoint() - self.drag_position)
            self.windowMoved.emit(self.parent.pos())
            event.accept()
            
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.dragging = False
            event.accept()
            
    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            # Проверяем, что двойной клик не по кнопке
            pos = event.pos()
            if not (self.minimize_btn.geometry().contains(pos) or 
                    self.maximize_btn.geometry().contains(pos) or 
                    self.close_btn.geometry().contains(pos)):
                self.windowMaximized.emit()
                event.accept()


# ----------------------------------------------------------------------
# Кнопка запуска (круглая)
# ----------------------------------------------------------------------
class RunButton(QPushButton):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setText("▶")
        self.setFixedSize(40, 40)
        
    def update_style(self):
        """Обновить стиль в соответствии с темой"""
        if hasattr(self.window(), 'current_theme'):
            colors = THEMES[self.window().current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
            
        self.setStyleSheet(f"""
            QPushButton {{
                background-color: {colors['autosave_on'].name()};
                color: {colors['bg'].name()};
                border: none;
                border-radius: 20px;
                font-size: 30px;
            }}
            QPushButton:hover {{
                background-color: {colors['autosave_on'].darker(110).name()};
                border: 2px solid {colors['ls_idle'].name()};
            }}
            QPushButton:pressed {{
                background-color: {colors['ls_idle'].name()};
            }}
        """)


# ----------------------------------------------------------------------
# Кастомный редактор с автозакрытием скобок и Ctrl+Click для @include
# ----------------------------------------------------------------------
class CustomScintilla(QsciScintilla):
    gotoDefinitionRequested = pyqtSignal(str, int)  # путь, строка
    contentChanged = pyqtSignal()  # сигнал об изменении содержимого

    INDICATOR_ERROR = 8
    WARNING_ERROR = 9
    ERROR_LINE_INDICATOR = 10
    WARNING_LINE_INDICATOR = 11

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMouseTracking(True)
        self.ctrlPressed = False
        self.main_window = None
        self.last_save_time = datetime.now()
        self.filename = None

        self.errors = []  # (line, start_col, end_col, message, type)
        self.error_lines = set()
        self.error_messages = {}  # Словарь {line: message} для быстрого доступа

        self.setup_error_indicator()
        self.setup_error_marker()
        self.textChanged.connect(self.on_text_changed)
        self.current_tooltip_msg = None

    def get_line_text(self, line):
        """Получить текст конкретной строки"""
        try:
            # Получаем позиции начала и конца строки
            start_pos = self.SendScintilla(self.SCI_POSITIONFROMLINE, line)
            if start_pos < 0:
                return ""
            
            # Получаем длину строки
            line_length = self.SendScintilla(self.SCI_LINELENGTH, line)
            if line_length <= 0:
                return ""
            
            # Получаем текст строки
            text_bytes = self.SendScintilla(self.SCI_GETTEXTRANGE, start_pos, start_pos + line_length)
            if isinstance(text_bytes, bytes):
                return text_bytes.decode('utf-8', errors='replace')
            return str(text_bytes) if text_bytes else ""
        except Exception as e:
            print(f"Error getting line text: {e}")
            return ""

    def set_main_window(self, window):
        self.main_window = window

    def setup_error_indicator(self):
        if self.main_window:
            colors = THEMES[self.main_window.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
            
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.INDICATOR_ERROR
        )
        self.indicatorDefine(
            QsciScintilla.IndicatorStyle.ThickCompositionIndicator,
            self.WARNING_ERROR
        )
        self.setIndicatorForegroundColor(colors["error"], self.INDICATOR_ERROR)
        self.setIndicatorForegroundColor(colors["warning"], self.WARNING_ERROR)

    def setup_error_marker(self):
        """Настройка маркера для подсветки строк с ошибками"""
        if self.main_window:
            colors = THEMES[self.main_window.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        # Создаем полупрозрачный красный цвет для фона строки с ошибкой
        error_bg_color = QColor(colors["error"])
        error_bg_color.setAlpha(40)  # 40/255 прозрачности
        
        # Определяем маркер для подсветки всей строки
        self.markerDefine(QsciScintilla.MarkerSymbol.Background, self.ERROR_LINE_INDICATOR)
        self.setMarkerBackgroundColor(error_bg_color, self.ERROR_LINE_INDICATOR)

        warning_bg_color = QColor(colors["warning"])
        warning_bg_color.setAlpha(40)  # 40/255 прозрачности
        
        # Определяем маркер для подсветки всей строки
        self.markerDefine(QsciScintilla.MarkerSymbol.Background, self.WARNING_LINE_INDICATOR)
        self.setMarkerBackgroundColor(warning_bg_color, self.WARNING_LINE_INDICATOR)

    def set_errors(self, errors):
        self.clear_errors()
        for error in errors:
            self.add_error(*error)
        self.viewport().update()  # Принудительное обновление для отображения текста

    def on_text_changed(self):
        self.clear_errors()
        self.contentChanged.emit()

    def clear_errors(self):
        # Очищаем индикаторы
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.INDICATOR_ERROR)
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.WARNING_ERROR)
        
        # Очищаем маркеры строк с ошибками
        self.markerDeleteAll(self.ERROR_LINE_INDICATOR)
        self.markerDeleteAll(self.WARNING_LINE_INDICATOR)
        
        # Очищаем данные об ошибках
        self.errors.clear()
        self.error_lines.clear()
        self.error_messages.clear()
        
        self.viewport().update()  # Обновляем виджет

    def get_error_at_position(self, line, col):
        """Получить сообщение об ошибке для позиции"""
        for err_line, start_col, end_col, message, type_ in self.errors:
            if err_line == line and start_col <= col < end_col:
                return str(message)  # Явно преобразуем в строку
        return None

    def add_error(self, line, start_col, end_col, message, type_):
        start_col += 1
        end_col += 1
        
        # Убеждаемся, что message - строка
        if not isinstance(message, str):
            message = str(message)
        
        # Добавляем индикатор подчеркивания
        self.fillIndicatorRange(line, start_col, line, end_col, 
                            self.INDICATOR_ERROR if type_ == 0 else self.WARNING_ERROR)
        
        # Добавляем подсветку всей строки
        if type_ == 0:  # Ошибка
            self.markerAdd(line, self.ERROR_LINE_INDICATOR)
            self.error_messages[line] = f"Error: {message}"
        elif type_ == 1:  # Предупреждение
            self.markerAdd(line, self.WARNING_LINE_INDICATOR)
            self.error_messages[line] = f"Warning: {message}"
        
        self.error_lines.add(line)
        self.errors.append((line, start_col, end_col, message, type_))

    def paintEvent(self, event):
        super().paintEvent(event)
        
        if not self.error_messages:
            return
            
        try:
            painter = QPainter(self.viewport())
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            
            # Получаем цвета из текущей темы
            if self.main_window:
                colors = THEMES[self.main_window.current_theme]["colors"]
            else:
                colors = THEMES["Catppuccin Latte"]["colors"]
            
            # Получаем видимые строки
            first_line = self.SendScintilla(self.SCI_GETFIRSTVISIBLELINE)
            lines_visible = self.SendScintilla(self.SCI_LINESONSCREEN)
            
            # Получаем ширину поля с номерами строк
            margin_width = self.marginWidth(0)
            
            # Для каждой видимой строки с ошибкой
            for line in range(first_line, first_line + lines_visible + 1):
                if line in self.error_messages:
                    try:
                        # Получаем позицию строки на экране
                        pos = self.SendScintilla(self.SCI_POSITIONFROMLINE, line)
                        if pos < 0:
                            continue
                        y = self.SendScintilla(self.SCI_POINTYFROMPOSITION, 0, pos)

                        line_end_pos = self.SendScintilla(self.SCI_GETLINEENDPOSITION, line)

                        if line_end_pos > pos:
                            # Получаем X координату конца строки (без \n)
                            end_x = self.SendScintilla(self.SCI_POINTXFROMPOSITION, 0, line_end_pos)
                        else:
                            # Если строка пустая, используем отступ от поля с номерами
                            end_x = margin_width

                        
                        # Добавляем отступ после кода
                        x = end_x + 35 # 20 пикселей отступа после кода
                       
                        
                        # Определяем тип ошибки по сохраненным данным
                        error_type = 0  # По умолчанию ошибка
                        for err_line, _, _, _, type_ in self.errors:
                            if err_line == line:
                                error_type = type_
                                break
                        
                        # Выбираем цвет в зависимости от типа
                        if error_type == 0:
                            text_color = colors["error"]
                        else:
                            text_color = colors["warning"]
                        
                        # Делаем фон полупрозрачным
                        
                        
                        # Текст ошибки
                        error_text = self.error_messages[line]
                        
                        # Настройки шрифта
                        font = QFont("Consolas", 9)
                        if self.main_window:
                            font.setPointSize(self.main_window.global_font_size)
                        painter.setFont(font)
                        
                        # Измеряем текст
                        metrics = painter.fontMetrics()
                        text_width = metrics.horizontalAdvance(error_text)
                    
                        
                        # Ограничиваем ширину, чтобы не выходить за пределы окна
                        max_width = self.width() - x - 20
                        if text_width > max_width and max_width > 50:
                            # Если текст слишком длинный, обрезаем его
                            available_width = max_width - 30
                            if available_width > 20:
                                # Оцениваем количество символов, которое поместится
                                avg_char_width = metrics.averageCharWidth()
                                if avg_char_width > 0:
                                    chars_to_keep = int(available_width / avg_char_width)
                                    if chars_to_keep > 5:
                                        error_text = error_text[:chars_to_keep] + "..."
                                        text_width = metrics.horizontalAdvance(error_text)
                                    else:
                                        # Если совсем мало места, показываем только иконку
                                        
                                        text_width = metrics.horizontalAdvance(error_text)
                            else:
                                # Слишком мало места, пропускаем
                                continue
                        
                        
                        
                        # Рисуем текст
                        painter.setPen(QPen(text_color))
                        painter.drawText(x, y + metrics.ascent(), error_text)
                        
                    except Exception as e:
                        print(f"Error painting error for line {line}: {e}")
                        continue
            
            painter.end()
            
        except Exception as e:
            print(f"Error in paintEvent: {e}")
            
    

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
        try:
            pos = event.pos()
            position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
            line, col = self.lineIndexFromPosition(position)

            error_msg = self.get_error_at_position(line, col) if line >= 0 and col >= 0 else None

            if error_msg != self.current_tooltip_msg:
                if error_msg:
                    # Убеждаемся, что передаем строку
                    QToolTip.showText(event.globalPosition().toPoint(), str(error_msg), self)
                else:
                    QToolTip.hideText()
                self.current_tooltip_msg = error_msg

            super().mouseMoveEvent(event)
        except Exception as e:
            print(f"Error in mouseMoveEvent: {e}")
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
# Лексер TwistLang с поддержкой тем из словаря
# ----------------------------------------------------------------------
class TwistLangLexer(QsciLexerCustom):
    def __init__(self, parent=None, theme_name="Catppuccin Latte", font_size=12):
        super().__init__(parent)
        self.theme_name = theme_name
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

    def set_theme(self, theme_name):
        self.theme_name = theme_name
        self.setup_styles()

    def set_font_size(self, size):
        self.font_size = size
        self.setup_styles()

    def wordCharacters(self):
        return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"

    def setup_styles(self):
        theme = THEMES[self.theme_name]
        colors = theme["colors"]

        self.setDefaultPaper(colors["bg"])
        self.setDefaultColor(colors["fg"])

        safe_font = self.get_safe_font("Consolas", self.font_size)
        self.setFont(safe_font)

        # Цвета для разных стилей
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
        }

        for style, color in style_colors.items():
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
        self.tabBar().tabMoved.connect(self.on_tab_moved)

    def addTab(self, widget, title):
        index = super().addTab(widget, title)
        self._add_close_button(index)
        return index

    def insertTab(self, index, widget, title):
        index = super().insertTab(index, widget, title)
        self._add_close_button(index)
        return index

    def on_tab_moved(self, from_index, to_index):
        """Обновляем индексы у кнопок закрытия после перемещения вкладок"""
        for i in range(self.count()):
            self._update_close_button(i)

    def _update_close_button(self, index):
        """Обновить кнопку закрытия для конкретной вкладки"""
        tab_bar = self.tabBar()
        old_btn = tab_bar.tabButton(index, QTabBar.ButtonPosition.RightSide)
        if old_btn:
            old_btn.deleteLater()
        
        btn = QToolButton(tab_bar)
        btn.setIcon(self._create_close_icon())
        btn.setIconSize(QSize(14, 14))
        btn.setStyleSheet("""
            QToolButton {
                background-color: transparent;
                border: none;
                border-radius: 3px;
                padding: 2px;
            }
            QToolButton:hover {
                background-color: rgba(255, 255, 255, 30);
            }
            QToolButton:pressed {
                background-color: rgba(255, 255, 255, 60);
            }
        """)
        
        btn.clicked.connect(lambda checked, idx=index: self._on_close_clicked(idx))
        tab_bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, btn)

    def _add_close_button(self, index):
        self._update_close_button(index)

    def _create_close_icon(self):
        pixmap = QPixmap(14, 14)
        pixmap.fill(Qt.GlobalColor.transparent)
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        main_window = self.window()
        if hasattr(main_window, 'current_theme'):
            colors = THEMES[main_window.current_theme]["colors"]
            painter.setPen(QPen(colors["fg"], 1.5))
        else:
            painter.setPen(QPen(QColor("#cdd6f4"), 1.5))
        
        painter.drawLine(4, 4, 10, 10)
        painter.drawLine(10, 4, 4, 10)
        painter.end()
        return QIcon(pixmap)

    def _on_close_clicked(self, index):
        """Обработчик клика по кнопке закрытия"""
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
        
        if self.count() == 0:
            main_window = self.window()
            if hasattr(main_window, 'new_file'):
                main_window.new_file()


# ----------------------------------------------------------------------
# Кастомное меню с закругленными углами (восстановлено)
# ----------------------------------------------------------------------
class RoundedMenu(QMenu):
    def __init__(self, title=None, parent=None):
        super().__init__(title, parent)
        self.setWindowFlags(
            Qt.WindowType.Popup | 
            Qt.WindowType.FramelessWindowHint | 
            Qt.WindowType.NoDropShadowWindowHint
        )
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)

    def showEvent(self, event):
        self.adjustSize()
        super().showEvent(event)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        main_window = self.window()
        if hasattr(main_window, 'current_theme'):
            colors = THEMES[main_window.current_theme]["colors"]
        else:
            colors = THEMES[CURRENT_THEME]["colors"]
        
        rect = self.rect()
        
        # Рисуем фон с закругленными углами
        painter.setBrush(QColor(colors['bg']))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(rect, 8, 8)
        
        # Рисуем тонкую рамку
        painter.setPen(QPen(QColor(colors['status_border']), 1))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 7, 7)
        
        super().paintEvent(event)


# ----------------------------------------------------------------------
# Главное окно редактора
# ----------------------------------------------------------------------
class TwistLangEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        
        # Убираем стандартный заголовок и делаем окно с закругленными углами
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)  # Добавляем прозрачность для закруглений
        
        self.setGeometry(100, 100, 1200, 800)
        self.setWindowTitle("Lumen IDE")
        
        
        # Центральный виджет с закругленными углами
        central_widget = QWidget()
        central_widget.setObjectName("centralWidget")
        central_widget.setStyleSheet("""
            QWidget#centralWidget {
                border-radius: 10px;
            }
        """)
        self.setCentralWidget(central_widget)
        
        # Основной layout
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        
        # Кастомный заголовок (теперь он напрямую в central_widget)
        self.title_bar = CustomTitleBar(self)
        self.title_bar.setObjectName("titleBar")
        main_layout.addWidget(self.title_bar)
        
        # Контейнер для контента (внутренняя часть с фоном)
        content_widget = QWidget()
        content_widget.setObjectName("contentWidget")
        content_layout = QVBoxLayout(content_widget)
        content_layout.setContentsMargins(0, 0, 0, 0)
        content_layout.setSpacing(0)
        main_layout.addWidget(content_widget, 1)

        # Виджет вкладок
        self.tab_widget = EditorTabWidget(self)
        content_layout.addWidget(self.tab_widget)

        # Кнопка запуска
        self.run_button = RunButton(self)
        self.run_button.hide()  # Скрываем, пока не применим тему
        
        # Статус бар с закругленными нижними углами
        self.status_bar = QStatusBar()
        self.status_bar.setObjectName("statusBar")
        self.setStatusBar(self.status_bar)

        self.ls_processes = {}
        self.current_file = None
        self.current_theme = CURRENT_THEME
        self.global_font_size = 12

        self.autosave_timer = QTimer(self)
        self.autosave_timer.timeout.connect(self.autosave_all_files)
        self.autosave_interval = 100
        self.autosave_timer.start(self.autosave_interval)

        self.error_check_timer = QTimer(self)
        self.error_check_timer.timeout.connect(self.check_for_errors)
        self.error_check_timer.start(300)

        # Создаем меню (но не добавляем в стандартный menuBar)
        self.create_menu()
        
        # Добавляем меню в кастомный заголовок
        self.title_bar.add_menu(self.menuBar())
        
        self.setup_shortcuts()

        self.setAcceptDrops(True)
        self.tab_widget.currentChanged.connect(self.on_tab_changed)

        self.autosave_label = QLabel()
        self.status_bar.addPermanentWidget(self.autosave_label)

        self.error_label = QLabel()
        self.status_bar.addPermanentWidget(self.error_label)

        self.ls_status_label = QLabel()
        self.status_bar.addPermanentWidget(self.ls_status_label)

        self.autosave_count = 0
        
        # Подключаем сигналы заголовка
        self.title_bar.windowMinimized.connect(self.showMinimized)
        self.title_bar.windowMaximized.connect(self.toggle_maximize)
        self.title_bar.windowClosed.connect(self.close)
        
        # Применяем начальную тему
        self.apply_theme(self.current_theme)

    def toggle_maximize(self):
        """Переключение между максимизированным и нормальным состоянием"""
        if self.title_bar.is_window_maximized:
            self.showNormal()
        else:
            self.showMaximized()
        
        # Обновляем состояние в title_bar после изменения
        self.title_bar.set_maximized_state(self.isMaximized())
    
    


    def apply_theme(self, theme_name):
        """Применить тему по имени"""
        self.current_theme = theme_name
        colors = THEMES[theme_name]["colors"]

        # Обновляем иконку приложения с цветом текущей темы
        icon = create_colored_svg_icon(r"data\app_icon.svg", colors['title_fg'])
        if icon and not icon.isNull():
            self.setWindowIcon(icon)
            # Обновляем иконку в заголовке
            self.title_bar.update_icon(icon)
            print(f"✅ Icon updated for theme: {theme_name}")
        else:
            print(f"⚠️ Could not update icon for theme: {theme_name}")
            # Запасной вариант
            if os.path.exists(r'data\app_icon.png'):
                png_icon = QIcon(r'data\app_icon.png')
                self.setWindowIcon(png_icon)
                self.title_bar.update_icon(png_icon)
        
        # Обновляем заголовок
        self.title_bar.update_style()
        
        # Обновляем кнопку запуска
        self.run_button.update_style()
        self.run_button.show()
        
        # Обновляем центральный виджет (прозрачный, только для закруглений)
        self.centralWidget().setStyleSheet(f"""
            QWidget#centralWidget {{
                border-radius: 10px;
            }}
        """)
        
        # Обновляем контейнер для контента (здесь будет основной фон)
        content_widget = self.findChild(QWidget, "contentWidget")
        if content_widget:
            content_widget.setStyleSheet(f"""
                QWidget#contentWidget {{
                    background-color: {colors['title_bg'].name()};
                }}
            """)
        
        # Обновляем статус бар с закруглениями
        self.status_bar.setStyleSheet(f"""
            QStatusBar#statusBar {{
                background-color: {colors['status_bg'].name()};
                color: {colors['status_fg'].name()};
                border-top: 1px solid {colors['status_border'].name()};
                border-bottom-left-radius: 7px;
                border-bottom-right-radius: 7px;
            }}
            QStatusBar::item {{
                border: none;
            }}
        """)
        
        # Обновляем стиль для таб-бара (старый стиль, но с цветами из темы)
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
            }}
            
            QTabBar QToolButton:hover {{
                background-color: {colors['selection_bg'].name()};
            }}
        """)
        
        # Обновляем лейблы
        self.update_status_labels()
        
        # Обновляем все редакторы
        self.update_all_editor_themes()
        
        # Обновляем палитру приложения
        self.update_application_palette()
        
        # Обновляем иконки закрытия
        self.update_all_close_icons()

    def update_status_labels(self):
        """Обновить цвета статусных лейблов согласно текущей теме"""
        colors = THEMES[self.current_theme]["colors"]
        
        if self.autosave_action.isChecked():
            self.autosave_label.setText(f"⚡ Auto-save: ON ({self.autosave_interval//1000}s)")
            self.autosave_label.setStyleSheet(f"color: {colors['autosave_on'].name()}; padding: 2px 5px;")
        else:
            self.autosave_label.setText("⚡ Auto-save: OFF")
            self.autosave_label.setStyleSheet(f"color: {colors['autosave_off'].name()}; padding: 2px 5px;")
        
        current = self.current_editor()
        if current and len(current.errors) > 0:
            self.error_label.setText(f"✗ {len(current.errors)} error(s)")
            self.error_label.setStyleSheet(f"color: {colors['error'].name()}; padding: 2px 5px;")
        else:
            self.error_label.setText("✓ No errors")
            self.error_label.setStyleSheet(f"color: {colors['autosave_on'].name()}; padding: 2px 5px;")
        
        active_ls_count = sum(1 for proc in self.ls_processes.values() if proc.poll() is None)
        self.ls_status_label.setText(f"⚙ LS: {active_ls_count} active")
        
        if active_ls_count > 0:
            self.ls_status_label.setStyleSheet(f"color: {colors['ls_active'].name()}; padding: 2px 5px;")
        else:
            self.ls_status_label.setStyleSheet(f"color: {colors['ls_idle'].name()}; padding: 2px 5px;")

    def update_all_close_icons(self):
        """Обновить иконки закрытия на всех вкладках при смене темы"""
        for i in range(self.tab_widget.count()):
            tab_bar = self.tab_widget.tabBar()
            btn = tab_bar.tabButton(i, QTabBar.ButtonPosition.RightSide)
            if btn and isinstance(btn, QToolButton):
                btn.setIcon(self.tab_widget._create_close_icon())

    def select_theme(self, theme_name):
        """Выбор темы из меню"""
        self.apply_theme(theme_name)
        
        if hasattr(self, 'theme_actions'):
            for action in self.theme_actions:
                action.setChecked(action.text() == theme_name)
        
        self.status_bar.showMessage(f"Theme switched to {theme_name}", 2000)

    def update_all_editor_themes(self):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                text = editor.text()
                line, col = editor.getCursorPosition()
                scroll_pos = editor.SendScintilla(editor.SCI_GETFIRSTVISIBLELINE)

                new_lexer = TwistLangLexer(editor, self.current_theme, self.global_font_size)
                editor.setLexer(new_lexer)

                editor.setText(text)
                editor.setCursorPosition(line, col)
                editor.SendScintilla(editor.SCI_SETFIRSTVISIBLELINE, scroll_pos)

                self.apply_editor_theme(editor)
                self.update_autocompletion_icons(editor)
                editor.setMarginsFont(new_lexer.font(new_lexer.STYLE_DEFAULT))
                self.update_margin_width(editor)

                editor.repaint()
        self.update_all_close_icons()

    def apply_editor_theme(self, editor: CustomScintilla):
        colors = THEMES[self.current_theme]["colors"]
        
        editor.setCaretForegroundColor(colors["caret"])
        editor.setCaretLineBackgroundColor(colors["caret_line"])
        editor.setMarginsBackgroundColor(colors["margin_bg"])
        editor.setMarginsForegroundColor(colors["margin_fg"])
        editor.setSelectionBackgroundColor(colors["selection_bg"])
        editor.setSelectionForegroundColor(colors["selection_fg"])
        editor.setMatchedBraceBackgroundColor(colors["brace_bg"])
        editor.setMatchedBraceForegroundColor(colors["brace_fg"])
        
        editor.setIndicatorForegroundColor(colors["error"], CustomScintilla.INDICATOR_ERROR)
        editor.setIndicatorForegroundColor(colors["warning"], CustomScintilla.WARNING_ERROR)
        
        error_bg_color = QColor(colors["error"])
        error_bg_color.setAlpha(40)
        editor.setMarkerBackgroundColor(error_bg_color, CustomScintilla.ERROR_LINE_INDICATOR)
        
        warning_bg_color = QColor(colors["warning"])
        warning_bg_color.setAlpha(40)
        editor.setMarkerBackgroundColor(warning_bg_color, CustomScintilla.WARNING_LINE_INDICATOR)
        
        editor.update()
        editor.viewport().update()

    def update_application_palette(self):
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

    def resizeEvent(self, event):
        super().resizeEvent(event)
        # Позиционируем кнопку запуска
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
        menubar.setVisible(False)  # Скрываем стандартный менюбар
        
        file_menu = RoundedMenu("File", self)
        menubar.addMenu(file_menu)
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

        autosave_menu = RoundedMenu("Auto save", self)
        file_menu.addMenu(autosave_menu)
        self.autosave_action = QAction("Enable Auto-save", self)
        self.autosave_action.setCheckable(True)
        self.autosave_action.setChecked(True)
        self.autosave_action.triggered.connect(self.toggle_autosave)
        autosave_menu.addAction(self.autosave_action)

        autosave_menu.addSeparator()
        interval_menu = RoundedMenu("Interval", self)
        autosave_menu.addMenu(interval_menu)
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

        edit_menu = RoundedMenu("Edit", self)
        menubar.addMenu(edit_menu)
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

        view_menu = RoundedMenu("Editor", self)
        menubar.addMenu(view_menu)
        zoom_in_action = QAction("Zoom In", self)
        zoom_in_action.setShortcut(QKeySequence("Ctrl+="))
        zoom_in_action.triggered.connect(self.zoom_in)
        view_menu.addAction(zoom_in_action)

        zoom_out_action = QAction("Zoom Out", self)
        zoom_out_action.setShortcut(QKeySequence("Ctrl+-"))
        zoom_out_action.triggered.connect(self.zoom_out)
        view_menu.addAction(zoom_out_action)

        view_menu.addSeparator()
        
        theme_menu = RoundedMenu("Theme", self)
        view_menu.addMenu(theme_menu)
        self.theme_actions = []
        
        for theme_name in THEMES.keys():
            theme_action = QAction(theme_name, self)
            theme_action.setCheckable(True)
            if theme_name == self.current_theme:
                theme_action.setChecked(True)
            theme_action.triggered.connect(lambda checked, tn=theme_name: self.select_theme(tn))
            theme_menu.addAction(theme_action)
            self.theme_actions.append(theme_action)

        run_menu = RoundedMenu("Run", self)
        menubar.addMenu(run_menu)
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
        editor.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        index = self.tab_widget.addTab(editor, title)
        self.tab_widget.setCurrentIndex(index)
        editor.filename = filename
        editor.modificationChanged.connect(lambda modified: self.update_tab_title(editor, modified))
        editor.gotoDefinitionRequested.connect(self.open_include_file)
        self.setup_editor_widget(editor)
        self.apply_editor_theme(editor)
        
        if filename:
            self.start_language_server(filename)

    def setup_editor_widget(self, editor):
        editor.setUtf8(True)
        editor.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        editor.setScrollWidthTracking(True)
        editor.setScrollWidth(1)
        safe_font = self.get_safe_font("Consolas", self.global_font_size)
        editor.setFont(safe_font)
        editor.setMarginsFont(safe_font)
        editor.setCaretLineVisible(True)
        editor.setMarginType(0, QsciScintilla.MarginType.NumberMargin)
        self.update_margin_width(editor)
        editor.setBraceMatching(QsciScintilla.BraceMatch.SloppyBraceMatch)
        editor.setIndentationsUseTabs(False)
        editor.setTabWidth(4)
        editor.setIndentationGuides(True)
        editor.textChanged.connect(lambda: self.update_margin_width(editor))

        lexer = TwistLangLexer(editor, self.current_theme, self.global_font_size)
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
        colors = THEMES[self.current_theme]["colors"]
        size = lexer.font_size
        
        img_keyword = self.create_type_pixmap("K", colors["keyword"], size)
        img_modifier = self.create_type_pixmap("M", colors["modifier"], size)
        img_type = self.create_type_pixmap("T", colors["type"], size)
        img_literal = self.create_type_pixmap("L", colors["literal"], size)
        img_directive = self.create_type_pixmap("D", colors["directive"], size)
        img_special = self.create_type_pixmap("S", colors["special"], size)
        img_function = self.create_type_pixmap("F", colors["function"], size)

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
        
        colors = THEMES[self.current_theme]["colors"]
        size = lexer.font_size
        
        img_keyword = self.create_type_pixmap("K", colors["keyword"], size)
        img_modifier = self.create_type_pixmap("M", colors["modifier"], size)
        img_type = self.create_type_pixmap("T", colors["type"], size)
        img_literal = self.create_type_pixmap("L", colors["literal"], size)
        img_directive = self.create_type_pixmap("D", colors["directive"], size)
        img_special = self.create_type_pixmap("S", colors["special"], size)
        img_function = self.create_type_pixmap("F", colors["function"], size)

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

        self.update_status_labels()

    def check_file_errors(self, editor):
        if not editor or not editor.filename:
            return

        error_filename = self.get_error_filename(editor.filename)
        if error_filename and os.path.exists(error_filename):
            errors = self.parse_error_file(error_filename)
            editor.set_errors(errors)
        else:
            editor.clear_errors()

        self.update_status_labels()

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
            self.update_status_labels()

            editor = self.find_editor_by_filename(file_path)
            if editor:
                QTimer.singleShot(500, lambda: self.check_file_errors(editor))

        except Exception as e:
            print(f"Failed to start LS for {file_path}: {e}")

    def find_editor_by_filename(self, filename):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and editor.filename == filename:
                return editor
        return None

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
            self.update_status_labels()
        except Exception as e:
            print(f"Failed to stop LS for {file_path}: {e}")

    def stop_all_language_servers(self):
        for file_path in list(self.ls_processes.keys()):
            self.stop_language_server(file_path)

    def on_tab_changed(self, index):
        editor = self.tab_widget.widget(index)
        if not isinstance(editor, CustomScintilla):
            return

        self.current_file = editor.filename
        self.update_status_labels()

        if self.current_file:
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
            self.update_status_labels()

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

            if filename in self.ls_processes:
                self.stop_language_server(filename)
                QTimer.singleShot(100, lambda: self.start_language_server(filename))

        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not save file: {e}")

    def toggle_autosave(self, checked):
        if checked:
            self.autosave_timer.start(self.autosave_interval)
            self.update_status_labels()
            self.status_bar.showMessage("Auto-save enabled", 2000)
        else:
            self.autosave_timer.stop()
            self.update_status_labels()
            self.status_bar.showMessage("Auto-save disabled", 2000)

    def set_autosave_interval(self, ms):
        self.autosave_interval = ms
        if self.autosave_action.isChecked():
            self.autosave_timer.start(ms)

        self.update_status_labels()

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
                colors = THEMES[self.current_theme]["colors"]
                self.autosave_label.setStyleSheet(f"color: {colors['warning'].name()}; padding: 2px 5px;")
                QTimer.singleShot(1000, self.update_status_labels)
        elif manual:
            self.status_bar.showMessage("No files to save", 2000)

    def clear_all_errors(self):
        editor = self.current_editor()
        if editor:
            editor.clear_errors()
            self.update_status_labels()
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
    app.setAttribute(Qt.ApplicationAttribute.AA_DontUseNativeMenuBar, True)

    default_font = QFont("Segoe UI", 10)
    if not default_font.exactMatch():
        default_font = QFont("Arial", 10)
    app.setFont(default_font)

    colors = THEMES[CURRENT_THEME]["colors"]
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
    sys.exit(app.exec())


if __name__ == "__main__":
    main()