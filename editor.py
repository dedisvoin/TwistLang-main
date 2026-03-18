"""
Lumen IDE - Integrated Development Environment for TwistLang
"""

import sys
import os
import subprocess
import platform
import shutil
import re
import random
import math
from datetime import datetime
from typing import Optional, Dict, List, Tuple, Set, Any
from dataclasses import dataclass
from enum import Enum

from PyQt6.QtGui import QPainterPath
from PyQt6.Qsci import QsciScintilla, QsciLexerCustom, QsciAPIs
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QMenu, QStatusBar,
    QLabel, QTabWidget, QTabBar, QToolButton,
    QFileDialog, QMessageBox, QToolTip, QWidget, QHBoxLayout,
    QVBoxLayout, QFrame, QStackedWidget
)
from PyQt6.QtGui import (
    QColor, QFont, QAction, QKeySequence, QShortcut,
    QPainter, QPen, QIcon, QPixmap, QMouseEvent,
    QLinearGradient, QRadialGradient
)
from PyQt6.QtCore import QByteArray, QRect, Qt, pyqtSignal, QTimer, QSize, QPoint, QPropertyAnimation, QEasingCurve
from PyQt6.QtSvg import QSvgRenderer

from themes import *


# =============================================================================
# CONSTANTS & CONFIGURATION
# =============================================================================

MAX_FONT_SIZE = 30
MIN_FONT_SIZE = 8

DEFAULT_THEME = "Lumen Classic"
WINDOW_MIN_WIDTH = 400
WINDOW_MIN_HEIGHT = 300
TITLE_BAR_HEIGHT = 32
RESIZE_MARGIN = 8
DEFAULT_FONT_SIZE = 12
AUTOSAVE_INTERVALS = [
    ("0.3 seconds", 300),
    ("1 seconds", 1000),
    ("1 minute", 60000),
    ("2 minutes", 120000),
    ("5 minutes", 300000)
]


class ErrorType(Enum):
    """Error severity levels"""
    ERROR = 0
    WARNING = 1


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
# UTILITY FUNCTIONS
# =============================================================================

def create_svg_icon(svg_path: str, color: QColor) -> Optional[QIcon]:
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
        
        pixmap = QPixmap(32, 32)
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
# WELCOME WIDGET
# =============================================================================

class WelcomeWidget(QWidget):
    """Welcome screen shown when no files are open"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        
        # Initialize floating text manager
        
        
    def resizeEvent(self, event):
        """Recreate floating texts when widget is resized"""
        super().resizeEvent(event)
        
        
    def paintEvent(self, event):
        painter = QPainter(self)
        
        
        colors = self._get_theme_colors()
        
        # Fill background with gradient
        gradient = QLinearGradient(0, 0, self.width(), self.height())
        gradient.setColorAt(0, colors['bg'])
        gradient.setColorAt(1, colors['title_bg'])
        painter.fillRect(self.rect(), gradient)
        
        
        # Draw large "Lumen IDE" text
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['fg'], 1))
        
        # Use large font
        font = QFont("Segoe UI", 100, QFont.Weight.Light)
        font.setLetterSpacing(QFont.SpacingType.PercentageSpacing, 110)
        painter.setFont(font)
        
        # Center text
        text = "Lumen IDE"
        metrics = painter.fontMetrics()
        text_width = metrics.horizontalAdvance(text)
        text_height = metrics.height()
        
        x = (self.width() - text_width) // 2
        y = (self.height() - text_height) // 2 + metrics.ascent()
        
        # Draw with glow effect
        for i in range(5):
            offset = i * 2
            painter.setOpacity(0.1 * (5 - i))
            painter.setPen(QPen(colors['title_fg'], 1))
            painter.drawText(x + offset, y + offset, text)
        
        painter.setOpacity(1.0)
        painter.setPen(QPen(colors['title_fg'], 1))
        painter.drawText(x, y, text)
        
        # Draw subtitle
        painter.setPen(QPen(colors['title_fg'], 1))
        subtitle_font = QFont("Consolas", 14)
        painter.setFont(subtitle_font)
        
        subtitle = "Integrated development environment for Lumen"
        metrics = painter.fontMetrics()
        subtitle_width = metrics.horizontalAdvance(subtitle)
        
        subtitle_x = (self.width() - subtitle_width) // 2
        subtitle_y = y + text_height // 2 - 50
        
        painter.drawText(subtitle_x, subtitle_y, subtitle)
        
        # Draw static author text at bottom
        author_font = QFont("Consolas", 11)
        painter.setFont(author_font)
        painter.setPen(QPen(colors['title_fg'], 1))
        painter.setOpacity(0.8)
        
        author_text = "By Pavlov Ivan Alexeevich"
        metrics = painter.fontMetrics()
        author_x = 10
        author_y = self.height() - 10

        #painter.drawText(author_x, author_y, author_text)
        
        painter.end()
        
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]


# =============================================================================
# CUSTOM WIDGETS
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
        self._animation_timer.setInterval(16)  # ~60 FPS
        
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
        
        # Draw background with fade
        if self._opacity > 0:
            green_color = QColor(colors['autosave_on'])
            green_color.setAlphaF(self._opacity)
            painter.fillRect(self.rect(), green_color)
        
        # Draw play triangle
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
    Mimics Windows 10 style with rounded corners.
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
        """Initialize UI components"""
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # Title section with icon
        title_widget = self._create_title_section()
        layout.addWidget(title_widget)
        
        # Menu container
        self.menu_container = QWidget()
        self.menu_layout = QHBoxLayout(self.menu_container)
        self.menu_layout.setContentsMargins(5, 0, 0, 0)
        self.menu_layout.setSpacing(0)
        layout.addWidget(self.menu_container)
        
        layout.addStretch()
        
        # Run button
        self.run_button = RunButton(self)
        self.run_button.clicked.connect(self.run_clicked.emit)
        layout.addWidget(self.run_button)
        
        # Window controls
        self.minimize_btn = self._create_minimize_button()
        self.maximize_btn = self._create_maximize_button()
        self.close_btn = self._create_close_button()
        
        layout.addWidget(self.minimize_btn)
        layout.addWidget(self.maximize_btn)
        layout.addWidget(self.close_btn)
        
    def _create_title_section(self) -> QWidget:
        """Create title section with icon and window title"""
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(10, 0, 0, 0)
        layout.setSpacing(8)
        
        self.icon_label = QLabel()
        layout.addWidget(self.icon_label)
        
        self.title_label = QLabel("Lumen IDE")
        self.title_label.setStyleSheet("font-size: 12px;")
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
            # Restore icon (overlapping squares)
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
            # Maximize icon (single square)
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
        
        # Draw X
        x1 = btn.width() // 2 - 5
        y1 = btn.height() // 2 - 5
        x2 = btn.width() // 2 + 5
        y2 = btn.height() // 2 + 5
        
        painter.drawLine(x1, y1, x2, y2)
        painter.drawLine(x2, y1, x1, y2)
        painter.end()
        
    def _draw_close_hover_background(self, painter, btn):
        """Draw red background for close button hover with rounded top-right corner"""
        rect = btn.rect()
        painter.setBrush(QColor(196, 43, 28))  # #c42b1c
        painter.setPen(Qt.PenStyle.NoPen)
        
        path = QPainterPath()
        path.moveTo(rect.left(), rect.top())
        path.arcTo(rect.right() - 14.5, rect.top(), 20, 70, 90, -90)
        path.lineTo(rect.right(), rect.bottom() + 1)
        path.lineTo(rect.left(), rect.bottom() + 1)
        path.lineTo(rect.left(), rect.top())
        
        painter.drawPath(path)
        
    def _get_theme_colors(self):
        if hasattr(self.parent_window, 'current_theme'):
            return THEMES[self.parent_window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]
        
    def update_icon(self, icon):
        """Update title bar icon"""
        if icon and not icon.isNull():
            self.icon_label.setPixmap(icon.pixmap(16, 16))
            
    def set_maximized_state(self, maximized: bool):
        """Update maximize button state"""
        self.is_maximized = maximized
        self.maximize_btn.update()
        self.update_style()  # Обновляем стиль для скругления углов
        
    def update_style(self):
        """Apply current theme styles"""
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
            }}
        """)
        
        # Update menu buttons
        for i in range(self.menu_layout.count()):
            btn = self.menu_layout.itemAt(i).widget()
            if isinstance(btn, QToolButton):
                btn.setStyleSheet(f"""
                    QToolButton {{
                        background-color: transparent;
                        color: {colors['title_fg'].name()};
                        border: none;
                        padding: 8px 12px;
                        font-size: 12px;
                    }}
                    QToolButton:hover {{
                        background-color: {colors['title_bg_darker'].name()};
                        color: {colors['selection_fg'].name()};
                    }}
                    QToolButton::menu-indicator {{
                        image: none;
                    }}
                """)
        
        # Update window controls
        self.minimize_btn.update()
        self.maximize_btn.update()
        self.close_btn.update()
        self.run_button.update()
        
    def add_menu(self, menu_bar):
        """Add menu bar items to title bar"""
        # Clear container
        for i in reversed(range(self.menu_layout.count())):
            widget = self.menu_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
        
        # Add menu actions as buttons
        for action in menu_bar.actions():
            if action.menu():
                btn = QToolButton()
                btn.setText(action.text())
                btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
                btn.setMenu(action.menu())
                # Убираем стрелку индикатора меню
                btn.setStyleSheet("""
                    QToolButton::menu-indicator {
                        image: none;
                    }
                """)
                self.menu_layout.addWidget(btn)
        
        self.update_style()
        
    # Mouse event handlers for window dragging
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            # Не начинаем перетаскивание, если окно развернуто на весь экран
            if self.parent_window and self.is_maximized:
                super().mousePressEvent(event)
                return
                
            pos = event.pos()
            # Don't start drag if clicking on buttons
            if not any(btn.geometry().contains(pos) for btn in [
                self.minimize_btn, self.maximize_btn, self.close_btn, self.run_button
            ]):
                self.dragging = True
                self.drag_position = event.globalPosition().toPoint() - self.parent_window.frameGeometry().topLeft()
                event.accept()
                return
        super().mousePressEvent(event)
        
    def mouseMoveEvent(self, event):
        # Не перемещаем окно, если оно развернуто на весь экран
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


class RoundedMenu(QMenu):
    """Custom menu with rounded corners"""
    
    def __init__(self, title=None, parent=None):
        super().__init__(title, parent)
        self.setWindowFlags(
            Qt.WindowType.Popup | 
            Qt.WindowType.FramelessWindowHint | 
            Qt.WindowType.NoDropShadowWindowHint
        )
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        rect = self.rect()
        
        # Draw background
        painter.setBrush(QColor(colors['bg']))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(rect, 8, 8)
        
        # Draw border
        painter.setPen(QPen(QColor(colors['status_border']), 1))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 7, 7)
        
        super().paintEvent(event)
        
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
        """Enable moving only if more than one tab"""
        self.setMovable(self.count() > 1)
        
    def _on_tab_moved(self, from_index, to_index):
        """Update close buttons after tab move"""
        for i in range(self.count()):
            self._update_close_button(i)
            
    def _add_close_button(self, index):
        self._update_close_button(index)
        
    def _update_close_button(self, index):
        """Create or update close button for a tab"""
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
        
    def _create_close_icon(self) -> QIcon:
        """Create close icon with current theme color"""
        pixmap = QPixmap(14, 14)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = self._get_theme_colors()
        painter.setPen(QPen(colors["fg"], 1.5))
        
        painter.drawLine(4, 4, 10, 10)
        painter.drawLine(10, 4, 4, 10)
        painter.end()
        
        return QIcon(pixmap)
        
    def _get_theme_colors(self):
        window = self.window()
        if hasattr(window, 'current_theme'):
            return THEMES[window.current_theme]["colors"]
        return THEMES[DEFAULT_THEME]["colors"]
        
    def _on_close_clicked(self, index):
        """Handle close button click"""
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
        """Close tab with confirmation if modified"""
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
    Supports multiple themes and configurable font sizes.
    """
    
    # Style constants
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
    
    def __init__(self, parent=None, theme_name: str = DEFAULT_THEME, font_size: int = DEFAULT_FONT_SIZE):
        super().__init__(parent)
        self.theme_name = theme_name
        self.font_size = font_size
        
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
            self.STYLE_OBJECT: "Object"
        }
        return descriptions.get(style, "")
        
    def set_theme(self, theme_name: str):
        """Change current theme"""
        self.theme_name = theme_name
        self.setup_styles()
        
    def set_font_size(self, size: int):
        """Change font size"""
        self.font_size = size
        self.setup_styles()
        
    def setup_styles(self):
        """Apply current theme colors and font"""
        theme = THEMES[self.theme_name]
        colors = theme["colors"]
        
        self.setDefaultPaper(colors["bg"])
        self.setDefaultColor(colors["fg"])
        
        safe_font = get_safe_monospace_font("Consolas", self.font_size)
        self.setFont(safe_font)
        
        # Style colors
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
            
        # Special font styles
        italic_font = get_safe_monospace_font("Consolas", self.font_size)
        italic_font.setItalic(True)
        self.setFont(italic_font, self.STYLE_COMMENT)
        
        bold_font = get_safe_monospace_font("Consolas", self.font_size)
        bold_font.setBold(True)
        self.setFont(bold_font, self.STYLE_KEYWORD)

        
        
        
    def styleText(self, start: int, end: int):
        """Apply syntax highlighting to text range"""
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
                
            # Single-line comment
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
                    
            # String literals
            if ch in ('"', "'"):
                expecting_namespace = False
                quote = ch
                j = pos + ch_len
                escaped = False
                while j < end:
                    c, l = self._get_char_at(text_bytes, j, total_bytes)
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
                
            # Numbers
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
                
            # Identifiers and keywords
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
                
            # Operators
            if ch in '+-*/%=&|^!<>~?.:;(){}[]':
                expecting_namespace = False
                j = pos + ch_len
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
                
            # Default
            self.setStyling(ch_len, self.STYLE_DEFAULT)
            pos += ch_len
        self.update_folding_levels(start, end)
        
    def update_folding_levels(self, start, end):
        editor = self.editor()
        if not editor: return
        
        # Берем строку начала и конца
        start_line = editor.SendScintilla(editor.SCI_LINEFROMPOSITION, start)
        end_line = editor.SendScintilla(editor.SCI_LINEFROMPOSITION, end)
        
        # Получаем уровень вложенности от предыдущей строки
        current_level = 1024 # Базовый уровень
        if start_line > 0:
            prev_level = editor.SendScintilla(editor.SCI_GETFOLDLEVEL, start_line - 1)
            current_level = prev_level & 0x0FFF
            
        for line in range(start_line, end_line + 1):
            text = editor.text(line).strip()
            
            # Считаем разницу скобок
            open_count = text.count('{') + text.count('(')
            close_count = text.count('}') + text.count(')')
            
            # Флаг того, что строка является "шапкой" (на ней будет кнопка)
            is_header = 0x2000 if open_count > close_count else 0
            
            # Устанавливаем уровень
            editor.SendScintilla(editor.SCI_SETFOLDLEVEL, line, current_level | is_header)
            
            # Обновляем уровень для следующей строки
            current_level += (open_count - close_count)
            
    def _get_char_at(self, text_bytes: bytes, pos: int, total_bytes: int) -> Tuple[Optional[str], int]:
        """Safely get character at byte position (UTF-8 aware)"""
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


# =============================================================================
# EDITOR
# =============================================================================

@dataclass
class ErrorInfo:
    """Information about a syntax error or warning"""
    line: int
    start_col: int
    end_col: int
    message: str
    type: ErrorType


class CustomScintilla(QsciScintilla):
    """
    Enhanced Scintilla editor with:
    - Auto-closing brackets
    - Ctrl+Click to follow includes
    - Error highlighting
    - Tooltips for errors
    - Zoom with Ctrl+Wheel
    """
    
    goto_definition_requested = pyqtSignal(str, int)  # path, line
    content_changed = pyqtSignal()
    
    # Indicator constants
    INDICATOR_ERROR = 8
    WARNING_ERROR = 9
    ERROR_LINE_MARKER = 10
    WARNING_LINE_MARKER = 11
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.main_window = None
        self.filename = None
        self.last_save_time = datetime.now()
        
        self.ctrl_pressed = False
        self.current_tooltip_msg = None
        
        self.errors: List[ErrorInfo] = []
        self.error_lines: Set[int] = set()
        self.error_messages: Dict[int, str] = {}
        
        self.setMouseTracking(True)
        self._setup_editor()
        self._setup_error_highlighting()
        self.cursorPositionChanged.connect(self.highlightMatchingBrace)
        # Убираем подключение textChanged, которое вызывало лишние проверки
        # self.textChanged.connect(self._on_text_changed)

    def highlightMatchingBrace(self):
        """Custom highlight for matching braces including angle brackets"""
        pos = self.SendScintilla(self.SCI_GETCURRENTPOS)
        
        # Очищаем предыдущие индикаторы
        self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
        self.SendScintilla(self.SCI_INDICATORCLEARRANGE, 0, self.length())
        
        self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
        self.SendScintilla(self.SCI_INDICATORCLEARRANGE, 0, self.length())
        
        # Получаем символ под курсором
        char = chr(self.SendScintilla(self.SCI_GETCHARAT, pos)) if pos < self.length() else ''
        
        # Проверяем, является ли символ скобкой
        braces = {'(': ')', ')': '(', '[': ']', ']': '[', '{': '}', '}': '{', '<': '>', '>': '<'}
        
        if char in braces:
            # Находим парную скобку
            brace_pos = self.SendScintilla(self.SCI_BRACEMATCH, pos, 0)
            
            if brace_pos != -1:
                # Подсвечиваем текущую скобку
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, pos, 1)
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, pos, 1)
                
                # Подсвечиваем парную скобку
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 1)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, brace_pos, 1)
                self.SendScintilla(self.SCI_SETINDICATORCURRENT, 2)
                self.SendScintilla(self.SCI_INDICATORFILLRANGE, brace_pos, 1)
        
    # ===== Setup methods =====
    
    def _setup_editor(self):
        """Configure basic editor settings"""
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

        self.setFolding(QsciScintilla.FoldStyle.CircledTreeFoldStyle) # Или BoxedTreeFolding для более современного вида
        self.SendScintilla(self.SCI_SETMODEVENTMASK, self.SC_MOD_INSERTTEXT | self.SC_MOD_DELETETEXT)
        
        
        
    def _setup_error_highlighting(self):
        """Configure error indicators and markers"""
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
        
    def set_main_window(self, window):
        """Set reference to main window"""
        self.main_window = window
        
    # ===== Public API =====
    
    def set_errors(self, errors: List[tuple]):
        """Set errors for highlighting"""
        self.clear_errors()
        for error in errors:
            self.add_error(*error)
        self.viewport().update()
        
    def clear_errors(self):
        """Remove all error highlights"""
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.INDICATOR_ERROR)
        self.clearIndicatorRange(0, 0, self.lines(), 0, self.WARNING_ERROR)
        self.markerDeleteAll(self.ERROR_LINE_MARKER)
        self.markerDeleteAll(self.WARNING_LINE_MARKER)
        
        self.errors.clear()
        self.error_lines.clear()
        self.error_messages.clear()
        self.viewport().update()
        
    def add_error(self, line: int, start_col: int, end_col: int, message: str, error_type: int):
        """Add a single error highlight"""
        start_col += 1
        end_col += 1
        
        if not isinstance(message, str):
            message = str(message)
            
        self.fillIndicatorRange(line, start_col, line, end_col,
                               self.INDICATOR_ERROR if error_type == 0 else self.WARNING_ERROR)
        
        if error_type == 0:  # Error
            self.markerAdd(line, self.ERROR_LINE_MARKER)
            self.error_messages[line] = f"Error: {message}"
        else:  # Warning
            self.markerAdd(line, self.WARNING_LINE_MARKER)
            self.error_messages[line] = f"Warning: {message}"
            
        self.error_lines.add(line)
        self.errors.append(ErrorInfo(line, start_col, end_col, message, ErrorType(error_type)))
        
    def get_line_text(self, line: int) -> str:
        """Get text of a specific line"""
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
        """Check if line contains an include directive"""
        text = self.text(line).strip()
        return text.startswith('#include')
        
    def extract_include_path(self, line: int) -> Optional[str]:
        """Extract file path from include directive"""
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
    
    
    def get_error_at_position(self, line: int, col: int) -> Optional[str]:
        """Get error message at specific position"""
        for error in self.errors:
            if error.line == line and error.start_col <= col < error.end_col:
                return error.message
        return None
        
    # ===== Event handlers =====
    
    def keyPressEvent(self, event):
        """Handle key presses (bracket auto-close)"""
        # Auto-close brackets
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
        # Убираем emit отсюда
        
    def _insert_pair(self, open_char: str, close_char: str):
        """Insert matching bracket pair"""
        line, col = self.getCursorPosition()
        self.insertAt(open_char + close_char, line, col)
        self.setCursorPosition(line, col + 1)
        
    def mouseMoveEvent(self, event: QMouseEvent):
        """Handle mouse move (tooltips for errors)"""
        try:
            pos = event.pos()
            position = self.SendScintilla(self.SCI_POSITIONFROMPOINT, pos.x(), pos.y())
            line, col = self.lineIndexFromPosition(position)
            
            error_msg = self.get_error_at_position(line, col) if line >= 0 and col >= 0 else None
            
            if error_msg != self.current_tooltip_msg:
                if error_msg:
                    QToolTip.showText(event.globalPosition().toPoint(), str(error_msg), self)
                else:
                    QToolTip.hideText()
                self.current_tooltip_msg = error_msg
                
            super().mouseMoveEvent(event)
        except Exception as e:
            print(f"Error in mouseMoveEvent: {e}")
            super().mouseMoveEvent(event)
            
    def mousePressEvent(self, event: QMouseEvent):
        """Handle mouse press (Ctrl+Click for includes)"""
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
        """Handle wheel (zoom with Ctrl)"""
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0 and self.main_window:
                self.main_window.zoom_in()
            elif delta < 0 and self.main_window:
                self.main_window.zoom_out()
            event.accept()
        else:
            super().wheelEvent(event)
            
    def paintEvent(self, event):
        """Custom paint for error messages inline"""
        super().paintEvent(event)
        
        if not self.error_messages:
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
        """Paint error message for a specific line"""
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
            
            # Determine error type
            error_type = 0
            for err in self.errors:
                if err.line == line:
                    error_type = err.type.value
                    break
                    
            text_color = colors["error"] if error_type == 0 else colors["warning"]
            
            error_text = self.error_messages[line]
            
            font = QFont("Consolas", 9)
            if self.main_window:
                font.setPointSize(self.main_window.global_font_size)
            painter.setFont(font)
            
            # Truncate if too long
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


# =============================================================================
# MAIN WINDOW
# =============================================================================

class TwistLangEditor(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        
        self.current_theme = DEFAULT_THEME
        self.global_font_size = DEFAULT_FONT_SIZE
        self.autosave_interval = 300  # ms
        self.autosave_count = 0
        self.current_file = None
        
        self.ls_processes: Dict[str, subprocess.Popen] = {}
        self.theme_actions: List[QAction] = []
        
        self.resizing = False
        self.resize_direction = None
        self.resize_start_pos = None
        self.resize_start_geometry = None
        
        self._setup_window()
        self._setup_ui()
        self._setup_shortcuts()
        self._setup_timers()
        
        self.apply_theme(self.current_theme)
        
    # ===== Setup methods =====
    
    def _setup_window(self):
        """Configure main window"""
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setGeometry(100, 100, 1200, 800)
        self.setAcceptDrops(True)
        
        
    def _setup_ui(self):
        """Initialize UI components"""
        central_widget = QWidget()
        central_widget.setObjectName("centralWidget")
        central_widget.setStyleSheet("QWidget#centralWidget { border-radius: 10px; }")
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        
        # Title bar
        self.title_bar = CustomTitleBar(self)
        self.title_bar.setObjectName("titleBar")
        main_layout.addWidget(self.title_bar)
        
        # Content area with stacked widget for welcome screen and tabs
        self.content_stack = QStackedWidget()
        self.content_stack.setObjectName("contentStack")
        main_layout.addWidget(self.content_stack, 1)
        
        # Welcome widget
        self.welcome_widget = WelcomeWidget()
        self.content_stack.addWidget(self.welcome_widget)
        
        # Tab widget container with solid background
        self.tab_container = QWidget()
        self.tab_container.setObjectName("tabContainer")
        print(THEMES[DEFAULT_THEME]['colors']['title_fg'])
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
        
        # Status bar
        self.status_bar = QStatusBar()
        self.status_bar.setObjectName("statusBar")
        self.status_bar.setSizeGripEnabled(False)
        self.setStatusBar(self.status_bar)
        
        self.autosave_label = QLabel()
        self.error_label = QLabel()
        self.ls_status_label = QLabel()
        
        self.status_bar.addPermanentWidget(self.autosave_label)
        self.status_bar.addPermanentWidget(self.error_label)
        self.status_bar.addPermanentWidget(self.ls_status_label)
        
        # Menu
        self._create_menu()
        self.title_bar.add_menu(self.menuBar())
        
        # Connect title bar signals
        self.title_bar.window_minimized.connect(self.showMinimized)
        self.title_bar.window_maximized.connect(self._toggle_maximize)
        self.title_bar.window_closed.connect(self.close)
        self.title_bar.run_clicked.connect(self.run_current_file)
        
        # Initially show welcome widget
        self._update_content_display()
        
    def _create_menu(self):
        """Create application menus"""
        menubar = self.menuBar()
        menubar.setVisible(False)  # Hide default menubar
        
        # File menu
        file_menu = RoundedMenu("File", self)
        menubar.addMenu(file_menu)
        
        new_action = self._create_action("New", "Ctrl+N", self.new_file)
        file_menu.addAction(new_action)
        
        open_action = self._create_action("Open...", "Ctrl+O", self.open_file_dialog)
        file_menu.addAction(open_action)
        
        save_action = self._create_action("Save", "Ctrl+S", self.save_current_file)
        file_menu.addAction(save_action)
        
        save_as_action = self._create_action("Save As...", "Ctrl+Shift+S", self.save_current_file_as)
        file_menu.addAction(save_as_action)
        
        # Auto-save submenu
        autosave_menu = RoundedMenu("Auto save", self)
        file_menu.addMenu(autosave_menu)
        
        self.autosave_action = QAction("Enable Auto-save", self)
        self.autosave_action.setCheckable(True)
        self.autosave_action.setChecked(True)
        self.autosave_action.triggered.connect(self._toggle_autosave)
        autosave_menu.addAction(self.autosave_action)
        
        autosave_menu.addSeparator()
        
        interval_menu = RoundedMenu("Interval", self)
        autosave_menu.addMenu(interval_menu)
        
        for name, ms in AUTOSAVE_INTERVALS:
            action = QAction(name, self)
            action.setCheckable(True)
            action.setData(ms)
            if ms == self.autosave_interval:
                action.setChecked(True)
            action.triggered.connect(lambda checked, ms=ms: self._set_autosave_interval(ms))
            interval_menu.addAction(action)
            
        autosave_menu.addSeparator()
        
        save_now_action = self._create_action("Save Now", "Ctrl+Shift+S", 
                                              lambda: self.autosave_all_files(manual=True))
        autosave_menu.addAction(save_now_action)
        
        file_menu.addSeparator()
        
        close_action = self._create_action("Close", "Ctrl+W", self.close_current_tab)
        file_menu.addAction(close_action)
        
        close_all_action = self._create_action("Close All", None, self.close_all_tabs)
        file_menu.addAction(close_all_action)
        
        file_menu.addSeparator()
        
        exit_action = self._create_action("Exit", None, self.close)
        file_menu.addAction(exit_action)
        
        # Edit menu
        edit_menu = RoundedMenu("Edit", self)
        menubar.addMenu(edit_menu)
        
        edit_menu.addAction(self._create_action("Undo", "Ctrl+Z", lambda: self.current_editor().undo()))
        edit_menu.addAction(self._create_action("Redo", "Ctrl+Y", lambda: self.current_editor().redo()))
        edit_menu.addSeparator()
        edit_menu.addAction(self._create_action("Cut", "Ctrl+X", lambda: self.current_editor().cut()))
        edit_menu.addAction(self._create_action("Copy", "Ctrl+C", lambda: self.current_editor().copy()))
        edit_menu.addAction(self._create_action("Paste", "Ctrl+V", lambda: self.current_editor().paste()))
        
        # View menu
        view_menu = RoundedMenu("Editor", self)
        menubar.addMenu(view_menu)
        
        view_menu.addAction(self._create_action("Zoom In", "Ctrl+=", self.zoom_in))
        view_menu.addAction(self._create_action("Zoom Out", "Ctrl+-", self.zoom_out))
        view_menu.addSeparator()
        
        # Theme submenu
        theme_menu = RoundedMenu("Theme", self)
        view_menu.addMenu(theme_menu)
        
        for theme_name in THEMES.keys():
            theme_action = QAction(theme_name, self)
            theme_action.setCheckable(True)
            if theme_name == self.current_theme:
                theme_action.setChecked(True)
            theme_action.triggered.connect(lambda checked, tn=theme_name: self.select_theme(tn))
            theme_menu.addAction(theme_action)
            self.theme_actions.append(theme_action)
            
        # Run menu
        run_menu = RoundedMenu("Run", self)
        menubar.addMenu(run_menu)
        
        run_action = self._create_action("Run Code", "F5", self.run_current_file)
        run_menu.addAction(run_action)
        
        go_to_action = self._create_action("Go to Include", "F12", self.goto_include_under_cursor)
        run_menu.addAction(go_to_action)
        
        run_menu.addSeparator()
        
        clear_errors_action = self._create_action("Clear Errors", "Ctrl+E", self.clear_all_errors)
        run_menu.addAction(clear_errors_action)
        
        # Добавляем действие для ручной проверки ошибок
        check_errors_action = self._create_action("Check Errors", "F4", self.check_current_file_errors)
        run_menu.addAction(check_errors_action)
        
    def _create_action(self, text: str, shortcut: Optional[str], slot) -> QAction:
        """Create a standard action with shortcut"""
        action = QAction(text, self)
        if shortcut:
            action.setShortcut(QKeySequence(shortcut))
        action.triggered.connect(slot)
        return action
        
    def _setup_shortcuts(self):
        """Setup keyboard shortcuts not in menus"""
        QShortcut(QKeySequence("F8"), self).activated.connect(self.goto_next_error)
        QShortcut(QKeySequence("Shift+F8"), self).activated.connect(self.goto_prev_error)
        
    def _setup_timers(self):
        """Setup timers for autosave and error checking"""
        self.autosave_timer = QTimer(self)
        self.autosave_timer.timeout.connect(self.autosave_all_files)
        self.autosave_timer.start(self.autosave_interval)
        
        # Убираем постоянный таймер проверки ошибок
        # self.error_check_timer = QTimer(self)
        # self.error_check_timer.timeout.connect(self._check_for_errors)
        # self.error_check_timer.start(300)
        
    # ===== Content display management =====
    
    def _update_content_display(self):
        """Switch between welcome screen and tab widget based on open tabs"""
        if self.tab_widget.count() == 0:
            self.content_stack.setCurrentWidget(self.welcome_widget)
        else:
            self.content_stack.setCurrentWidget(self.tab_container)
    
    # ===== Theme management =====

    def show_status_message(self, message: str, timeout: int = 2000):
        """Show message in status bar with consistent styling"""
        colors = THEMES[self.current_theme]["colors"]
        status_bg = colors['status_bg']
        text_color = QColor(status_bg).lighter(300)  # Осветляем на 300%
        self.status_bar.setFont(QFont("Consolas"))
        
        self.status_bar.setStyleSheet(f"""
            QStatusBar#statusBar {{
                background-color: {colors['status_bg'].name()};
                border-top: 1px solid {colors['status_border'].name()};
                border-bottom-left-radius: 7px;
                border-bottom-right-radius: 7px;
                color: {text_color.name()};
            }}
            QStatusBar::item {{
                border: none;
            }}
            QStatusBar QLabel {{
                color: {text_color.name()};
                padding: 2px 5px;
                font-family: Consolas;
                font-size: 10pt;
            }}
        """)
        
        self.status_bar.showMessage(message, timeout)
    
    def apply_theme(self, theme_name: str):
        """Apply theme to all components"""
        self.current_theme = theme_name
        colors = THEMES[theme_name]["colors"]
        
        # Update window icon
        icon = create_svg_icon(r"data\app_icon.svg", colors['title_fg'])
        if icon and not icon.isNull():
            self.setWindowIcon(icon)
            self.title_bar.update_icon(icon)
            
        # Update title bar
        self.title_bar.update_style()
        
        # Update content widget
        content_widget = self.findChild(QWidget, "contentWidget")
        if content_widget:
            content_widget.setStyleSheet(f"""
                QWidget#contentWidget {{
                    background-color: {colors['title_bg'].name()};
                }}
            """)
        
        # Update tab container background
        self.tab_container.setStyleSheet(f"""
            QWidget#tabContainer {{
                background-color: {THEMES[self.current_theme]['colors']['title_bg'].name()};
            }}
        """)

        # Update status bar
        self.status_bar.setStyleSheet(f"""
            QStatusBar#statusBar {{
                background-color: {colors['status_bg'].name()};
                border-top: 1px solid {colors['status_border'].name()};
                border-bottom-left-radius: 7px;
                border-bottom-right-radius: 7px;
            }}
            QStatusBar::item {{
                border: none;
            }}
        """)
        
        # Обновляем стили для полос прокрутки и других элементов
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
        
        # Применяем стили для полос прокрутки ко всему приложению
        QApplication.instance().setStyleSheet(scrollbar_style)
        
        # Обновляем стили для всех виджетов, которые могут иметь полосы прокрутки
        for widget in QApplication.instance().allWidgets():
            if isinstance(widget, (QsciScintilla, QTabWidget)):
                widget.style().unpolish(widget)
                widget.style().polish(widget)
        
        # Update tab widget
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
        
        # Update all editors
        self._update_all_editor_themes()
        self._update_status_labels()
        self._update_application_palette()

        
        
        # Принудительно обновляем все виджеты
        self.repaint()
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                editor.repaint()
        
        # Update welcome widget (force repaint)
        self.welcome_widget.update()
        
    def select_theme(self, theme_name: str):
        """Select theme from menu"""
        self.apply_theme(theme_name)
        
        for action in self.theme_actions:
            action.setChecked(action.text() == theme_name)
            
        self.show_status_message(f"Theme switched to {theme_name}", 2000)
        
    def _update_all_editor_themes(self):
        """Update theme for all open editors"""
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                self._update_editor_theme(editor)


                
               
    def _update_editor_theme(self, editor: CustomScintilla):
        """Update theme for a single editor"""
        colors = THEMES[self.current_theme]["colors"]
        
        # Save state
        text = editor.text()
        line, col = editor.getCursorPosition()
        scroll_pos = editor.SendScintilla(editor.SCI_GETFIRSTVISIBLELINE)
        
        # Update lexer
        new_lexer = TwistLangLexer(editor, self.current_theme, self.global_font_size)
        editor.setLexer(new_lexer)
        
        # Restore state
        editor.setText(text)
        editor.setCursorPosition(line, col)
        editor.SendScintilla(editor.SCI_SETFIRSTVISIBLELINE, scroll_pos)
        
        # Apply colors
        editor.setCaretForegroundColor(colors["caret"])
        editor.setCaretLineBackgroundColor(colors["caret_line"])
        editor.setMarginsBackgroundColor(colors["margin_bg"])
        editor.setMarginsForegroundColor(colors["margin_fg"])
        editor.setSelectionBackgroundColor(colors["selection_bg"])
        editor.setSelectionForegroundColor(colors["selection_fg"])
        
        # Настройка подсветки парных скобок через индикаторы
        editor.setBraceMatching(QsciScintilla.BraceMatch.NoBraceMatch)  # Отключаем встроенную подсветку
        
        # Устанавливаем цвета для парных скобок через индикаторы
        # Индикатор 1 - для выделения скобок
        editor.SendScintilla(editor.SCI_INDICSETFORE, 1, colors["brace_fg"])
        editor.SendScintilla(editor.SCI_INDICSETSTYLE, 1, QsciScintilla.INDIC_FULLBOX)
        editor.SendScintilla(editor.SCI_INDICSETALPHA, 1, 30)  # Прозрачность
        
        # Индикатор 2 - для подсветки фона
        editor.SendScintilla(editor.SCI_INDICSETFORE, 2, colors["brace_bg"])
        editor.SendScintilla(editor.SCI_INDICSETSTYLE, 2, QsciScintilla.INDIC_FULLBOX)
        editor.SendScintilla(editor.SCI_INDICSETALPHA, 2, 30)  # Прозрачность
        
        editor.setIndicatorForegroundColor(colors["error"], CustomScintilla.INDICATOR_ERROR)
        editor.setIndicatorForegroundColor(colors["warning"], CustomScintilla.WARNING_ERROR)
        
        error_bg = QColor(colors["error"])
        error_bg.setAlpha(40)
        editor.setMarkerBackgroundColor(error_bg, CustomScintilla.ERROR_LINE_MARKER)
        
        warning_bg = QColor(colors["warning"])
        warning_bg.setAlpha(40)
        editor.setMarkerBackgroundColor(warning_bg, CustomScintilla.WARNING_LINE_MARKER)
        
        editor.setMarginsFont(new_lexer.font(new_lexer.STYLE_DEFAULT))
        self._update_margin_width(editor)
        self._setup_autocompletion_icons(editor, new_lexer)

        editor.setFoldMarginColors(colors["margin_bg"], colors["margin_bg"])

        
        
        
        
        editor.repaint()
        
    def _update_application_palette(self):
        """Update global application palette"""
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
        
    # ===== Window resize handling =====
    
    def _get_resize_direction(self, pos: QPoint) -> Optional[str]:
        """Determine resize direction from mouse position"""
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
                
            # Enforce minimum size
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
            pos = event.pos()
            
            if pos.y() <= self.title_bar.height():
                self.setCursor(Qt.CursorShape.ArrowCursor)
                super().mouseMoveEvent(event)
                return
                
            direction = self._get_resize_direction(pos)
            
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
            
            if direction:
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
        """Toggle between maximized and normal state"""
        if self.title_bar.is_maximized:
            self.showNormal()
        else:
            self.showMaximized()
        self.title_bar.is_maximized = not self.title_bar.is_maximized
        
        
    # ===== File operations =====
    
    def current_editor(self) -> Optional[CustomScintilla]:
        """Get currently active editor"""
        return self.tab_widget.currentWidget()
        
    def new_file(self):
        """Create new empty file"""
        self._add_editor_tab(CustomScintilla(), filename=None, title="Untitled")
        self._update_content_display()
        
    def open_file_dialog(self):
        """Show open file dialog"""
        filename, _ = QFileDialog.getOpenFileName(
            self, "Open File", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            self.open_file(filename)
            
    def open_file(self, filename: str):
        """Open file by path"""
        # Check if already open
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
            self._add_editor_tab(editor, filename=filename, title=os.path.basename(filename))
            self.show_status_message(f"Opened: {filename}", 2000)
            self._update_status_labels()
            
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not open file: {e}")
            
    def save_current_file(self):
        """Save currently active file"""
        editor = self.current_editor()
        if not editor:
            return
            
        filename = getattr(editor, 'filename', None)
        if filename:
            self.save_editor(editor, filename)
        else:
            self.save_editor_as(editor)
            
    def save_current_file_as(self):
        """Save current file with new name"""
        self.save_editor_as(self.current_editor())
        
    def save_editor_as(self, editor: CustomScintilla):
        """Save editor content with new filename"""
        filename, _ = QFileDialog.getSaveFileName(
            self, "Save File As", "", "TwistLang Files (*.twist);;All Files (*.*)"
        )
        if filename:
            if not filename.endswith('.twist'):
                filename += '.twist'
            self.save_editor(editor, filename)
            editor.filename = filename
            index = self.tab_widget.indexOf(editor)
            if index != -1:
                self.tab_widget.setTabText(index, os.path.basename(filename) + 
                                          ('*' if editor.isModified() else ''))
                
    def save_editor(self, editor: CustomScintilla, filename: str):
        """Save editor content to file"""
        try:
            text = editor.text()
            cleaned = text.replace('\x00', '')
            normalized = cleaned.replace('\r\n', '\n').replace('\r', '\n')
            
            with open(filename, 'w', encoding='utf-8', newline='') as f:
                f.write(normalized)
                
            editor.setModified(False)
            editor.last_save_time = datetime.now()
            self.show_status_message(f"Saved: {filename}", 2000)
            
            # НЕ перезапускаем сервер при сохранении, просто проверяем ошибки
            editor = self._find_editor_by_filename(filename)
            if editor:
                # Проверяем ошибки после сохранения, но сервер не перезапускаем
                QTimer.singleShot(100, lambda: self._check_file_errors(editor))
                
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not save file: {e}")
            
    def close_current_tab(self):
        """Close currently active tab"""
        index = self.tab_widget.currentIndex()
        if index >= 0:
            self.tab_widget.close_tab(index)
            
    def close_all_tabs(self):
        """Close all open tabs"""
        while self.tab_widget.count() > 0:
            self.tab_widget.close_tab(0)
        self._update_content_display()
            
    def _add_editor_tab(self, editor: CustomScintilla, filename: Optional[str], title: str):
        """Add new editor tab"""
        editor.set_main_window(self)
        index = self.tab_widget.addTab(editor, title)
        self.tab_widget.setCurrentIndex(index)
        editor.filename = filename
        
        editor.modificationChanged.connect(lambda modified: self._update_tab_title(editor, modified))
        editor.goto_definition_requested.connect(self.open_include_file)
        # Убираем подключение content_changed, так как не хотим проверять ошибки при каждом изменении
        # editor.content_changed.connect(self._on_editor_content_changed)
        
        self._setup_editor_widget(editor)
        self._update_editor_theme(editor)
        
        # Запускаем сервер ТОЛЬКО при открытии файла
        if filename:
            self.start_language_server(filename)
            
        self._update_content_display()
        
    def _setup_editor_widget(self, editor: CustomScintilla):
        """Configure editor widget"""
        font = get_safe_monospace_font("Consolas", self.global_font_size)
        editor.setFont(font)
        editor.setMarginsFont(font)
        self._update_margin_width(editor)
        
        lexer = TwistLangLexer(editor, self.current_theme, self.global_font_size)
        editor.setLexer(lexer)
        
        self._setup_autocompletion_icons(editor, lexer)

        
    def _setup_autocompletion_icons(self, editor: CustomScintilla, lexer: TwistLangLexer):
        """Setup icons for autocompletion"""
        colors = THEMES[self.current_theme]["colors"]
        size = lexer.font_size
        
        # Create icon pixmaps
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
            
        # Setup APIs
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
                
        api.prepare()
        lexer.setAPIs(api)
        
    def _create_type_pixmap(self, text: str, color: QColor, size: int) -> QPixmap:
        """Create pixmap for autocompletion icon"""
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setPen(QPen(color))
        painter.setFont(QFont("Arial", size, QFont.Weight.Bold))
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, text)
        painter.end()
        
        return pixmap
        
    def _update_margin_width(self, editor: CustomScintilla):
        """Update line number margin width based on line count"""
        lines = max(editor.lines(), 1)
        digits = len(str(lines))
        editor.setMarginWidth(0, "9" * (digits + 2))
        
    def _update_tab_title(self, editor: CustomScintilla, modified: bool):
        """Update tab title with modified indicator"""
        index = self.tab_widget.indexOf(editor)
        if index == -1:
            return
            
        base = self.tab_widget.tabText(index)
        if base.endswith('*'):
            base = base[:-1]
        self.tab_widget.setTabText(index, base + ('*' if modified else ''))
        
    # ===== Language Server =====
    
    def start_language_server(self, file_path: str):
        """Start language server for a file - только один раз при открытии"""
        if not file_path:
            return
            
        # Если сервер уже запущен для этого файла, ничего не делаем
        if file_path in self.ls_processes:
            proc = self.ls_processes[file_path]
            if proc.poll() is None:  # Сервер все еще работает
                print(f"LS already running for {os.path.basename(file_path)}")
                return
            else:
                # Сервер завершился, удаляем из словаря
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
            self._update_status_labels()
            
            # Первоначальная проверка ошибок через небольшую задержку
            editor = self._find_editor_by_filename(file_path)
            if editor:
                QTimer.singleShot(500, lambda: self._check_file_errors(editor))
                
        except Exception as e:
            print(f"Failed to start LS for {file_path}: {e}")
            
    def stop_language_server(self, file_path: str):
        """Stop language server for a file"""
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
            self._update_status_labels()
        except Exception as e:
            print(f"Failed to stop LS for {file_path}: {e}")
            
    def stop_all_language_servers(self):
        """Stop all running language servers"""
        for file_path in list(self.ls_processes.keys()):
            self.stop_language_server(file_path)
            
    def _find_editor_by_filename(self, filename: str) -> Optional[CustomScintilla]:
        """Find editor by filename"""
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla) and editor.filename == filename:
                return editor
        return None
        
    def _get_error_filename(self, file_path: str) -> Optional[str]:
        """Get error output filename for a source file"""
        if not file_path:
            return None
        base = os.path.splitext(os.path.basename(file_path))[0]
        return f"dbg/{base}_ls.dbg"
        
    def _parse_error_file(self, error_file_path: str) -> List[tuple]:
        """Parse language server error file"""
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
        
    def _check_file_errors(self, editor: CustomScintilla):
        """Check single file for errors"""
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
        """Manually check errors for current file (by F4)"""
        editor = self.current_editor()
        if editor and editor.filename:
            self._check_file_errors(editor)
            self.show_status_message("Error check completed", 2000)
        
    # ===== Run operations =====
    
    def run_current_file(self):
        """Run current file in terminal"""
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
                
        self.save_editor(editor, filename)
        
        try:
            system = platform.system()
            if system == "Windows":
                subprocess.Popen(f'start cmd /k bin\\twistc --file "{filename}"', shell=True)
                self.status_bar.showMessage(f"Running {os.path.basename(filename)}...", 3000)
            elif system == "Linux":
                self._run_on_linux(filename)
            else:
                QMessageBox.information(self, "Not Supported",
                                       f"Please run manually:\ntwistc --file {filename}")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run: {e}")
            
    def _run_on_linux(self, filename: str):
        """Run file on Linux with terminal detection"""
        terminals = ['gnome-terminal', 'xterm', 'konsole', 'xfce4-terminal']
        term_cmd = next((term for term in terminals if shutil.which(term)), None)
        
        if term_cmd:
            if term_cmd == 'gnome-terminal':
                subprocess.Popen([term_cmd, '--', 'bash', '-c', 
                                 f'twistc --file "{filename}"; read -p "Press enter..."'])
            else:
                subprocess.Popen([term_cmd, '-e', 
                                 f'twistc --file "{filename}"; read -p "Press enter..."'])
        else:
            subprocess.Popen(['twistc', '--file', filename])
            
        self.status_bar.showMessage(f"Running {os.path.basename(filename)}...", 3000)
        
    def open_include_file(self, path: str, line: int):
        """Open include file from path"""
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
        """Go to include file under cursor"""
        editor = self.current_editor()
        if not editor:
            return
            
        line, _ = editor.getCursorPosition()
        if editor.is_include_line(line):
            path = editor.extract_include_path(line)
            if path:
                self.open_include_file(path, line)
            else:
                self.show_status_message("No valid include path found", 2000)
        else:
            self.show_status_message("Not an include line", 2000)
            
    # ===== Error navigation =====
    
    def goto_next_error(self):
        """Jump to next error in current file"""
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.show_status_message("No errors", 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        for error in editor.errors:
            if error.line > current_line or (error.line == current_line and error.start_col > current_col):
                editor.setCursorPosition(error.line, error.start_col)
                editor.ensureLineVisible(error.line)
                self.show_status_message(f"Error at line {error.line + 1}", 2000)
                return
                
        self.show_status_message("No more errors", 2000)
        
    def goto_prev_error(self):
        """Jump to previous error in current file"""
        editor = self.current_editor()
        if not editor or not editor.errors:
            self.show_status_message("No errors", 2000)
            return
            
        current_line, current_col = editor.getCursorPosition()
        for error in reversed(editor.errors):
            if error.line < current_line or (error.line == current_line and error.start_col < current_col):
                editor.setCursorPosition(error.line, error.start_col)
                editor.ensureLineVisible(error.line)
                self.show_status_message(f"Error at line {error.line + 1}", 2000)
                return
                
        self.show_status_message("No previous errors", 2000)
        
    def clear_all_errors(self):
        """Clear all error highlights"""
        editor = self.current_editor()
        if editor:
            editor.clear_errors()
            self._update_status_labels()
            self.show_status_message("Errors cleared", 2000)
            
    # ===== Zoom operations =====
    
    def zoom_in(self):
        """Increase font size"""
        if self.global_font_size < MAX_FONT_SIZE:
            self.global_font_size += 1
            self._apply_global_font_to_all_editors()
            self.show_status_message(f"Font size: {self.global_font_size}pt", 2000)
            
    def zoom_out(self):
        """Decrease font size"""
        if self.global_font_size > MIN_FONT_SIZE:
            self.global_font_size -= 1
            self._apply_global_font_to_all_editors()
            self.show_status_message(f"Font size: {self.global_font_size}pt", 2000)
            
    def _apply_global_font_to_all_editors(self):
        """Apply current font size to all editors"""
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, CustomScintilla):
                lexer = editor.lexer()
                if isinstance(lexer, TwistLangLexer):
                    lexer.set_font_size(self.global_font_size)
                    editor.setFont(lexer.font(lexer.STYLE_DEFAULT))
                    editor.setMarginsFont(lexer.font(lexer.STYLE_DEFAULT))
                    self._update_margin_width(editor)
                    self._setup_autocompletion_icons(editor, lexer)
                    editor.recolor()
                    editor.repaint()
                    
    # ===== Auto-save =====
    
    def _toggle_autosave(self, checked: bool):
        """Enable/disable autosave"""
        if checked:
            self.autosave_timer.start(self.autosave_interval)
            self.show_status_message("Auto-save enabled", 2000)
        else:
            self.autosave_timer.stop()
            self.show_status_message("Auto-save disabled", 2000)
            
        self._update_status_labels()
        
    def _set_autosave_interval(self, ms: int):
        """Change autosave interval"""
        self.autosave_interval = ms
        if self.autosave_action.isChecked():
            self.autosave_timer.start(ms)
            
        self._update_status_labels()
        
        # Update menu checkmarks
        interval_menu = self.sender().parent()
        for action in interval_menu.actions():
            if action.data() == ms:
                action.setChecked(True)
            elif action.data() is not None:
                action.setChecked(False)
                
        self.show_status_message(f"Auto-save interval set to {ms//1000} seconds", 2000)
        
    def autosave_all_files(self, manual: bool = False):
        """Auto-save all modified files"""
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
                self.show_status_message(f"Manual save: {saved_count} file(s) saved", 2000)
            else:
                self.show_status_message(f"Auto-saved {saved_count} file(s) [{self.autosave_count}]", 1500)
                colors = THEMES[self.current_theme]["colors"]
                self.autosave_label.setStyleSheet(f"color: {colors['warning'].name()}; padding: 2px 5px;")
                QTimer.singleShot(1000, self._update_status_labels)
        elif manual:
            self.show_status_message("No files to save", 2000)
            
    # ===== Status updates =====
    
    def _update_status_labels(self):
        """Update status bar labels with current state"""
        colors = THEMES[self.current_theme]["colors"]
        
        # Используем светлый цвет для текста
        status_bg = colors['status_bg']
        text_color = QColor(status_bg).lighter(300)  # Осветляем на 300%
        
        # Autosave label
        if self.autosave_action.isChecked():
            self.autosave_label.setText(f"Auto-save: ON ({self.autosave_interval/1000}s)")
        else:
            self.autosave_label.setText("Auto-save: OFF")
        
        # Error label
        current = self.current_editor()
        if current and len(current.errors) > 0:
            self.error_label.setText(f"✗ {len(current.errors)} error(s)")
        else:
            self.error_label.setText("✓ No errors")
        
        # Language server label
        active_ls = sum(1 for proc in self.ls_processes.values() if proc.poll() is None)
        self.ls_status_label.setText(f"LS: {active_ls} active")
        
        # Применяем нормальный системный шрифт
        font = QFont("Consolas")
        font.setPointSize(10)  # Нормальный размер
        
        # Устанавливаем шрифт для всех лейблов
        self.autosave_label.setFont(font)
        self.error_label.setFont(font)
        self.ls_status_label.setFont(font)
        
        # Apply unified light text color to all labels
        unified_style = f"color: {text_color.name()}; padding: 2px 5px;"
        self.autosave_label.setStyleSheet(unified_style)
        self.error_label.setStyleSheet(unified_style)
        self.ls_status_label.setStyleSheet(unified_style)


    def _on_tab_changed(self, index: int):
        """Handle tab change"""
        editor = self.tab_widget.widget(index)
        if not isinstance(editor, CustomScintilla):
            return
            
        self.current_file = editor.filename
        self._update_status_labels()
        
        if self.current_file:
            self._check_file_errors(editor)
            
    # ===== Drag & drop =====
    
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            
    def dropEvent(self, event):
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if os.path.isfile(file_path):
                self.open_file(file_path)
        event.acceptProposedAction()
        
    # ===== Close event =====
    
    def closeEvent(self, event):
        """Clean up on close"""
        self.stop_all_language_servers()
        super().closeEvent(event)


# =============================================================================
# MAIN ENTRY POINT
# =============================================================================

def main():
    """Application entry point"""
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    app.setAttribute(Qt.ApplicationAttribute.AA_DontUseNativeMenuBar, True)
    
    # Set default font
    default_font = QFont("Segoe UI", 10)
    if not default_font.exactMatch():
        default_font = QFont("Arial", 10)
    app.setFont(default_font)
    
    # Apply initial theme palette
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
    
    # Create and show main window
    editor = TwistLangEditor()
    editor.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()