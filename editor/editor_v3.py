import sys
import os
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QTextEdit, QTreeView, QTabWidget,
    QDockWidget, QFileDialog, QMessageBox, QStatusBar
)
from PyQt6.QtCore import Qt, QDir, pyqtSignal
from PyQt6.QtGui import QAction, QFileSystemModel, QKeySequence, QColor, QPalette


COLORS = {
    "bg": "#1e1e2e",
    "fg": "#cdd6f4",
    "title_bg": "#313244",
    "title_hover": "#45475a",
    "selection_bg": "#585b70",
    "selection_fg": "#cdd6f4",
    "border": "#45475a",
    "comment": "#6c7086",
    "accent": "#89b4fa",
    "error": "#f38ba8",
    "success": "#a6e3a1",
}

STYLESHEET = f"""
QMainWindow {{
    background-color: {COLORS["bg"]};
}}

QTabWidget::pane {{
    background-color: {COLORS["bg"]};
    border: 1px solid {COLORS["border"]};
    border-radius: 6px;
}}

QTabBar::tab {{
    background-color: {COLORS["title_bg"]};
    color: {COLORS["fg"]};
    padding: 6px 12px;
    margin-right: 2px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}}
QTabBar::tab:selected {{
    background-color: {COLORS["bg"]};
    border-bottom: 2px solid {COLORS["accent"]};
}}
QTabBar::tab:hover:!selected {{
    background-color: {COLORS["title_hover"]};
}}

QTextEdit {{
    background-color: {COLORS["bg"]};
    color: {COLORS["fg"]};
    border: none;
    font-family: "Consolas", "Fira Code", monospace;
    font-size: 12px;
    selection-background-color: {COLORS["selection_bg"]};
    selection-color: {COLORS["selection_fg"]};
}}

QTreeView {{
    background-color: {COLORS["bg"]};
    color: {COLORS["fg"]};
    border: none;
    outline: none;
    font-family: "Segoe UI", sans-serif;
    font-size: 11px;
}}
QTreeView::item {{
    padding: 4px;
}}
QTreeView::item:hover {{
    background-color: {COLORS["title_hover"]};
}}
QTreeView::item:selected {{
    background-color: {COLORS["selection_bg"]};
    color: {COLORS["selection_fg"]};
}}

QDockWidget::title {{
    background-color: {COLORS["title_bg"]};
    color: {COLORS["fg"]};
    padding: 4px;
    font-weight: bold;
}}
QDockWidget::close-button, QDockWidget::float-button {{
    background-color: transparent;
    border: none;
}}
QDockWidget::close-button:hover, QDockWidget::float-button:hover {{
    background-color: {COLORS["title_hover"]};
    border-radius: 4px;
}}

QStatusBar {{
    background-color: {COLORS["title_bg"]};
    color: {COLORS["fg"]};
    border-top: 1px solid {COLORS["border"]};
}}

QMenuBar {{
    background-color: {COLORS["title_bg"]};
    color: {COLORS["fg"]};
}}
QMenuBar::item {{
    background-color: transparent;
    padding: 4px 8px;
}}
QMenuBar::item:selected {{
    background-color: {COLORS["title_hover"]};
    border-radius: 4px;
}}

QMenu {{
    background-color: {COLORS["bg"]};
    color: {COLORS["fg"]};
    border: 1px solid {COLORS["border"]};
    border-radius: 8px;
    padding: 4px;
}}
QMenu::item {{
    padding: 6px 25px 6px 25px;
}}
QMenu::item:selected {{
    background-color: {COLORS["selection_bg"]};
    border-radius: 6px;
}}
QMenu::separator {{
    height: 1px;
    background-color: {COLORS["border"]};
    margin: 4px 8px;
}}

QToolBar {{
    background-color: {COLORS["title_bg"]};
    border: none;
    spacing: 4px;
}}
QToolBar QToolButton {{
    background-color: transparent;
    color: {COLORS["fg"]};
    padding: 6px;
    border-radius: 4px;
}}
QToolBar QToolButton:hover {{
    background-color: {COLORS["title_hover"]};
}}
"""

class EditorTab(QTextEdit):
    def __init__(self, filename=None, parent=None):
        super().__init__(parent)
        self.filename = filename
        if filename and os.path.exists(filename):
            with open(filename, 'r', encoding='utf-8') as f:
                self.setText(f.read())
            self.setWindowTitle(os.path.basename(filename))
        else:
            self.setWindowTitle("Untitled")
        self.document().modificationChanged.connect(self._on_modification_changed)

    def _on_modification_changed(self, modified):
        title = self.windowTitle()
        if modified and not title.endswith('*'):
            self.setWindowTitle(title + '*')
        elif not modified and title.endswith('*'):
            self.setWindowTitle(title[:-1])

    def save(self):
        if not self.filename:
            return self.save_as()
        with open(self.filename, 'w', encoding='utf-8') as f:
            f.write(self.toPlainText())
        self.document().setModified(False)
        return True

    def save_as(self):
        filename, _ = QFileDialog.getSaveFileName(self, "Save As", "", "Text Files (*.txt);;All Files (*.*)")
        if filename:
            self.filename = filename
            self.save()
            self.setWindowTitle(os.path.basename(filename))
            return True
        return False


class FileExplorer(QTreeView):
    file_opened = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.model = QFileSystemModel()
        self.model.setRootPath("")
        self.model.setFilter(QDir.Filter.AllDirs | QDir.Filter.Files | QDir.Filter.NoDotAndDotDot)
        self.setModel(self.model)
        self.setHeaderHidden(True)
        self.setIndentation(15)
        self.doubleClicked.connect(self._on_double_click)

    def set_root(self, path):
        self.setRootIndex(self.model.index(path))

    def _on_double_click(self, index):
        file_path = self.model.filePath(index)
        if os.path.isfile(file_path):
            self.file_opened.emit(file_path)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Stylized Minimal IDE")
        self.setGeometry(100, 100, 1000, 700)
        self.setStyleSheet(STYLESHEET)

        # Центральная область: вкладки редактора
        self.tab_widget = QTabWidget()
        self.tab_widget.setTabsClosable(True)
        self.tab_widget.tabCloseRequested.connect(self._close_tab)
        self.setCentralWidget(self.tab_widget)

        # Док-виджет: файловый проводник
        self.explorer = FileExplorer()
        self.explorer.file_opened.connect(self._open_file)
        self.explorer_dock = QDockWidget("File Explorer", self)
        self.explorer_dock.setWidget(self.explorer)
        self.explorer_dock.setObjectName("explorer_dock")
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, self.explorer_dock)

        self._create_actions()
        self._create_menus()
        self._create_toolbar()

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")

    def _create_actions(self):
        self.new_action = QAction("New", self)
        self.new_action.setShortcut(QKeySequence.StandardKey.New)
        self.new_action.triggered.connect(self._new_file)

        self.open_action = QAction("Open...", self)
        self.open_action.setShortcut(QKeySequence.StandardKey.Open)
        self.open_action.triggered.connect(self._open_file_dialog)

        self.save_action = QAction("Save", self)
        self.save_action.setShortcut(QKeySequence.StandardKey.Save)
        self.save_action.triggered.connect(self._save_current)

        self.save_as_action = QAction("Save As...", self)
        self.save_as_action.setShortcut(QKeySequence.StandardKey.SaveAs)
        self.save_as_action.triggered.connect(self._save_as_current)

        self.close_action = QAction("Close", self)
        self.close_action.setShortcut(QKeySequence.StandardKey.Close)
        self.close_action.triggered.connect(self._close_current_tab)

        self.exit_action = QAction("Exit", self)
        self.exit_action.setShortcut(QKeySequence.StandardKey.Quit)
        self.exit_action.triggered.connect(self.close)

        self.show_explorer_action = QAction("File Explorer", self)
        self.show_explorer_action.setCheckable(True)
        self.show_explorer_action.setChecked(True)
        self.show_explorer_action.triggered.connect(self._toggle_explorer)

    def _create_menus(self):
        menubar = self.menuBar()
        file_menu = menubar.addMenu("File")
        file_menu.addAction(self.new_action)
        file_menu.addAction(self.open_action)
        file_menu.addSeparator()
        file_menu.addAction(self.save_action)
        file_menu.addAction(self.save_as_action)
        file_menu.addSeparator()
        file_menu.addAction(self.close_action)
        file_menu.addSeparator()
        file_menu.addAction(self.exit_action)

        view_menu = menubar.addMenu("View")
        view_menu.addAction(self.show_explorer_action)

    def _create_toolbar(self):
        toolbar = self.addToolBar("File")
        toolbar.addAction(self.new_action)
        toolbar.addAction(self.open_action)
        toolbar.addAction(self.save_action)

    def _new_file(self):
        editor = EditorTab()
        index = self.tab_widget.addTab(editor, "Untitled")
        self.tab_widget.setCurrentIndex(index)

    def _open_file(self, filepath):
        for i in range(self.tab_widget.count()):
            editor = self.tab_widget.widget(i)
            if isinstance(editor, EditorTab) and editor.filename == filepath:
                self.tab_widget.setCurrentIndex(i)
                return
        editor = EditorTab(filepath)
        index = self.tab_widget.addTab(editor, os.path.basename(filepath))
        self.tab_widget.setCurrentIndex(index)
        self.status_bar.showMessage(f"Opened: {filepath}", 2000)

    def _open_file_dialog(self):
        filepath, _ = QFileDialog.getOpenFileName(self, "Open File", "", "All Files (*.*)")
        if filepath:
            self._open_file(filepath)

    def _save_current(self):
        editor = self.tab_widget.currentWidget()
        if isinstance(editor, EditorTab):
            if editor.save():
                self.status_bar.showMessage(f"Saved: {editor.filename or 'Untitled'}", 2000)

    def _save_as_current(self):
        editor = self.tab_widget.currentWidget()
        if isinstance(editor, EditorTab):
            if editor.save_as():
                self.status_bar.showMessage(f"Saved as: {editor.filename}", 2000)

    def _close_tab(self, index):
        editor = self.tab_widget.widget(index)
        if isinstance(editor, EditorTab):
            if editor.document().isModified():
                reply = QMessageBox.question(
                    self, "Save Changes",
                    f"Save changes to '{editor.windowTitle()}'?",
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No | QMessageBox.StandardButton.Cancel
                )
                if reply == QMessageBox.StandardButton.Cancel:
                    return
                elif reply == QMessageBox.StandardButton.Yes:
                    if not editor.save():
                        return
        self.tab_widget.removeTab(index)
        editor.deleteLater()

    def _close_current_tab(self):
        if self.tab_widget.count() > 0:
            self._close_tab(self.tab_widget.currentIndex())

    def _toggle_explorer(self, checked):
        self.explorer_dock.setVisible(checked)

    def closeEvent(self, event):
        while self.tab_widget.count() > 0:
            self._close_tab(0)
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    # Устанавливаем тёмную палитру для системных виджетов, которые не переопределены в QSS
    app.setStyle("Fusion")
    palette = QPalette()
    palette.setColor(QPalette.ColorRole.Window, QColor(COLORS["bg"]))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS["fg"]))
    palette.setColor(QPalette.ColorRole.Base, QColor(COLORS["bg"]))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor(COLORS["title_bg"]))
    palette.setColor(QPalette.ColorRole.Text, QColor(COLORS["fg"]))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(COLORS["selection_bg"]))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor(COLORS["selection_fg"]))
    app.setPalette(palette)

    window = MainWindow()
    window.show()
    sys.exit(app.exec())