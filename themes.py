from PyQt6.QtGui import QColor

THEMES = {
    "Lumen Dark": {  # Тёмная тема
        "type": "dark",
        "colors": {
            "bg": QColor("#1e1e2e"),
            "fg": QColor("#cdd6f4"),
            # Цвета заголовка окна
            "title_bg": QColor("#313244"),           # Фон заголовка
            "title_bg_darker": QColor("#29293b"),    # Чуть темнее title_bg
            "title_fg": QColor("#cdd6f4"),           # Текст заголовка
            "title_border": QColor("#45475a"),       # Граница заголовка
            "title_inactive_bg": QColor("#2a2a3a"),  # Неактивный фон
            "title_inactive_fg": QColor("#9399b2"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#f5c2e7"),
            "type": QColor("#cba6f7"),
            "comment": QColor("#6c7086"),
            "string": QColor("#a6e3a1"),
            "number": QColor("#fab387"),
            "operator": QColor("#6c7086"),
            "function": QColor("#89b4fa"),
            "modifier": QColor("#b4befe"),
            "directive": QColor("#f5e0dc"),
            "literal": QColor("#f9e2af"),
            "namespace": QColor("#94e2d5"),
            "special": QColor("#f2b5b5"),
            "object": QColor("#f2cdcd"),
            "caret": QColor("#ffffff"),
            "caret_line": QColor("#313244"),
            "margin_bg": QColor("#181825"),
            "margin_fg": QColor("#6c7086"),
            "selection_bg": QColor("#585b70"),
            "selection_fg": QColor("#ffffff"),
            "brace_bg": QColor("#f5c2e755"),
            "brace_fg": QColor("#f5c2e7"),
            "error": QColor("#f35815"),
            "warning": QColor("#ffc413"),
            "autosave_on": QColor("#a6e3a1"),
            "autosave_off": QColor("#f38ba8"),
            "ls_active": QColor("#a6e3a1"),
            "ls_idle": QColor("#89b4fa"),
            "status_bg": QColor("#181825"),
            "status_fg": QColor("#cdd6f4"),
            "status_border": QColor("#313244"),
        }
    },
    
    "Lumen Classic": {  # Светлая тема
        "type": "light",
        "colors": {
            "bg": QColor("#eff1f5"),
            "fg": QColor("#4c4f69"),
            # Цвета заголовка окна
            "title_bg": QColor("#dce0e8"),           # Фон заголовка
            "title_bg_darker": QColor("#c9cdd8"),    # Чуть темнее title_bg
            "title_fg": QColor("#4c4f69"),           # Текст заголовка
            "title_border": QColor("#bcc0cc"),       # Граница заголовка
            "title_inactive_bg": QColor("#e6e9ef"),  # Неактивный фон
            "title_inactive_fg": QColor("#6c6f85"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#859900"),
            "type": QColor("#b58900"),
            "comment": QColor("#93a1a1"),
            "string": QColor("#2aa198"),
            "number": QColor("#d33682"),
            "operator": QColor("#657b83"),
            "function": QColor("#155e93"),
            "modifier": QColor("#6c71c4"),
            "directive": QColor("#cb4b16"),
            "literal": QColor("#dc322f"),
            "namespace": QColor("#268bd2"),
            "special": QColor("#d33682"),
            "object": QColor("#2a9781"),
            "caret": QColor("#1e1e2e"),
            "caret_line": QColor("#e6e9ef"),
            "margin_bg": QColor("#dce0e8"),
            "margin_fg": QColor("#6c6f85"),
            "selection_bg": QColor("#acb0be"),
            "selection_fg": QColor("#1e1e2e"),
            "brace_bg": QColor("#8839ef55"),
            "brace_fg": QColor("#8839ef"),
            "error": QColor("#f35815"),
            "warning": QColor("#ffc413"),
            "autosave_on": QColor("#a6e3a1"),
            "autosave_off": QColor("#f38ba8"),
            "ls_active": QColor("#a6e3a1"),
            "ls_idle": QColor("#89b4fa"),
            "status_bg": QColor("#2d2d3a"),
            "status_fg": QColor("#ffffff"),
            "status_border": QColor("#4c4f69"),
        }
    },
    
    "Solarized Dark": {  # Тема Solarized Dark
        "type": "dark",
        "colors": {
            "bg": QColor("#002b36"),
            "fg": QColor("#839496"),
            # Цвета заголовка окна
            "title_bg": QColor("#073642"),           # Фон заголовка
            "title_bg_darker": QColor("#032f39"),    # Чуть темнее title_bg
            "title_fg": QColor("#fdf6e3"),           # Текст заголовка
            "title_border": QColor("#586e75"),       # Граница заголовка
            "title_inactive_bg": QColor("#04323e"),  # Неактивный фон
            "title_inactive_fg": QColor("#93a1a1"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#859900"),
            "type": QColor("#b58900"),
            "comment": QColor("#586e75"),
            "string": QColor("#2aa198"),
            "number": QColor("#d33682"),
            "operator": QColor("#657b83"),
            "function": QColor("#268bd2"),
            "modifier": QColor("#6c71c4"),
            "directive": QColor("#cb4b16"),
            "literal": QColor("#dc322f"),
            "namespace": QColor("#268bd2"),
            "special": QColor("#d33682"),
            "object": QColor("#b58900"),
            "caret": QColor("#ffffff"),
            "caret_line": QColor("#073642"),
            "margin_bg": QColor("#073642"),
            "margin_fg": QColor("#586e75"),
            "selection_bg": QColor("#586e75"),
            "selection_fg": QColor("#fdf6e3"),
            "brace_bg": QColor("#85990055"),
            "brace_fg": QColor("#859900"),
            "error": QColor("#dc322f"),
            "warning": QColor("#b58900"),
            "autosave_on": QColor("#2aa198"),
            "autosave_off": QColor("#dc322f"),
            "ls_active": QColor("#2aa198"),
            "ls_idle": QColor("#268bd2"),
            "status_bg": QColor("#073642"),
            "status_fg": QColor("#fdf6e3"),
            "status_border": QColor("#586e75"),
        }
    },
    
    "Solarized Light": {  # Светлая тема Solarized Light
        "type": "light",
        "colors": {
            "bg": QColor("#fdf6e3"),
            "fg": QColor("#657b83"),
            # Цвета заголовка окна
            "title_bg": QColor("#eee8d5"),           # Фон заголовка
            "title_bg_darker": QColor("#ddd6c3"),    # Чуть темнее title_bg
            "title_fg": QColor("#586e75"),           # Текст заголовка
            "title_border": QColor("#93a1a1"),       # Граница заголовка
            "title_inactive_bg": QColor("#f5efdb"),  # Неактивный фон
            "title_inactive_fg": QColor("#839496"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#859900"),
            "type": QColor("#b58900"),
            "comment": QColor("#93a1a1"),
            "string": QColor("#2aa198"),
            "number": QColor("#d33682"),
            "operator": QColor("#657b83"),
            "function": QColor("#268bd2"),
            "modifier": QColor("#6c71c4"),
            "directive": QColor("#cb4b16"),
            "literal": QColor("#dc322f"),
            "namespace": QColor("#268bd2"),
            "special": QColor("#d33682"),
            "object": QColor("#b58900"),
            "caret": QColor("#002b36"),
            "caret_line": QColor("#eee8d5"),
            "margin_bg": QColor("#eee8d5"),
            "margin_fg": QColor("#93a1a1"),
            "selection_bg": QColor("#93a1a1"),
            "selection_fg": QColor("#002b36"),
            "brace_bg": QColor("#85990055"),
            "brace_fg": QColor("#859900"),
            "error": QColor("#dc322f"),
            "warning": QColor("#b58900"),
            "autosave_on": QColor("#2aa198"),
            "autosave_off": QColor("#dc322f"),
            "ls_active": QColor("#2aa198"),
            "ls_idle": QColor("#268bd2"),
            "status_bg": QColor("#073642"),
            "status_fg": QColor("#fdf6e3"),
            "status_border": QColor("#586e75"),
        }
    },
    
    "Nord": {  # Тема Nord
        "type": "dark",
        "colors": {
            "bg": QColor("#2e3440"),
            "fg": QColor("#d8dee9"),
            # Цвета заголовка окна
            "title_bg": QColor("#3b4252"),           # Фон заголовка
            "title_bg_darker": QColor("#2f3644"),    # Чуть темнее title_bg
            "title_fg": QColor("#eceff4"),           # Текст заголовка
            "title_border": QColor("#4c566a"),       # Граница заголовка
            "title_inactive_bg": QColor("#343a46"),  # Неактивный фон
            "title_inactive_fg": QColor("#a5abb6"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#81a1c1"),
            "type": QColor("#b48ead"),
            "comment": QColor("#4c566a"),
            "string": QColor("#a3be8c"),
            "number": QColor("#b48ead"),
            "operator": QColor("#81a1c1"),
            "function": QColor("#88c0d0"),
            "modifier": QColor("#81a1c1"),
            "directive": QColor("#d08770"),
            "literal": QColor("#d8dee9"),
            "namespace": QColor("#8fbcbb"),
            "special": QColor("#d08770"),
            "object": QColor("#8fbcbb"),
            "caret": QColor("#eceff4"),
            "caret_line": QColor("#3b4252"),
            "margin_bg": QColor("#3b4252"),
            "margin_fg": QColor("#4c566a"),
            "selection_bg": QColor("#4c566a"),
            "selection_fg": QColor("#eceff4"),
            "brace_bg": QColor("#81a1c155"),
            "brace_fg": QColor("#81a1c1"),
            "error": QColor("#bf616a"),
            "warning": QColor("#ebcb8b"),
            "autosave_on": QColor("#a3be8c"),
            "autosave_off": QColor("#bf616a"),
            "ls_active": QColor("#a3be8c"),
            "ls_idle": QColor("#88c0d0"),
            "status_bg": QColor("#3b4252"),
            "status_fg": QColor("#d8dee9"),
            "status_border": QColor("#4c566a"),
        }
    },
    
    "Nord Light": {  # Светлая тема Nord Light
        "type": "light",
        "colors": {
            "bg": QColor("#e5e9f0"),
            "fg": QColor("#2e3440"),
            # Цвета заголовка окна
            "title_bg": QColor("#d8dee9"),           # Фон заголовка
            "title_bg_darker": QColor("#c7ceda"),    # Чуть темнее title_bg
            "title_fg": QColor("#2e3440"),           # Текст заголовка
            "title_border": QColor("#81a1c1"),       # Граница заголовка
            "title_inactive_bg": QColor("#dee3ed"),  # Неактивный фон
            "title_inactive_fg": QColor("#4c566a"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#5e81ac"),
            "type": QColor("#b48ead"),
            "comment": QColor("#81a1c1"),
            "string": QColor("#a3be8c"),
            "number": QColor("#b48ead"),
            "operator": QColor("#5e81ac"),
            "function": QColor("#88c0d0"),
            "modifier": QColor("#5e81ac"),
            "directive": QColor("#d08770"),
            "literal": QColor("#2e3440"),
            "namespace": QColor("#8fbcbb"),
            "special": QColor("#d08770"),
            "object": QColor("#8fbcbb"),
            "caret": QColor("#2e3440"),
            "caret_line": QColor("#d8dee9"),
            "margin_bg": QColor("#d8dee9"),
            "margin_fg": QColor("#81a1c1"),
            "selection_bg": QColor("#81a1c1"),
            "selection_fg": QColor("#2e3440"),
            "brace_bg": QColor("#5e81ac55"),
            "brace_fg": QColor("#5e81ac"),
            "error": QColor("#bf616a"),
            "warning": QColor("#ebcb8b"),
            "autosave_on": QColor("#a3be8c"),
            "autosave_off": QColor("#bf616a"),
            "ls_active": QColor("#a3be8c"),
            "ls_idle": QColor("#88c0d0"),
            "status_bg": QColor("#3b4252"),
            "status_fg": QColor("#d8dee9"),
            "status_border": QColor("#4c566a"),
        }
    },
    
    "Dracula": {  # Тёмная тема Dracula
        "type": "dark",
        "colors": {
            "bg": QColor("#282a36"),
            "fg": QColor("#f8f8f2"),
            # Цвета заголовка окна
            "title_bg": QColor("#44475a"),           # Фон заголовка
            "title_bg_darker": QColor("#373a4a"),    # Чуть темнее title_bg
            "title_fg": QColor("#f8f8f2"),           # Текст заголовка
            "title_border": QColor("#6272a4"),       # Граница заголовка
            "title_inactive_bg": QColor("#3d4050"),  # Неактивный фон
            "title_inactive_fg": QColor("#b8b8b2"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#ff79c6"),
            "type": QColor("#8be9fd"),
            "comment": QColor("#6272a4"),
            "string": QColor("#f1fa8c"),
            "number": QColor("#bd93f9"),
            "operator": QColor("#ff79c6"),
            "function": QColor("#50fa7b"),
            "modifier": QColor("#ffb86c"),
            "directive": QColor("#ff5555"),
            "literal": QColor("#bd93f9"),
            "namespace": QColor("#8be9fd"),
            "special": QColor("#ff79c6"),
            "object": QColor("#f1fa8c"),
            "caret": QColor("#f8f8f2"),
            "caret_line": QColor("#44475a"),
            "margin_bg": QColor("#21222c"),
            "margin_fg": QColor("#6272a4"),
            "selection_bg": QColor("#44475a"),
            "selection_fg": QColor("#f8f8f2"),
            "brace_bg": QColor("#ff79c655"),
            "brace_fg": QColor("#ff79c6"),
            "error": QColor("#ff5555"),
            "warning": QColor("#ffb86c"),
            "autosave_on": QColor("#50fa7b"),
            "autosave_off": QColor("#ff5555"),
            "ls_active": QColor("#50fa7b"),
            "ls_idle": QColor("#8be9fd"),
            "status_bg": QColor("#21222c"),
            "status_fg": QColor("#f8f8f2"),
            "status_border": QColor("#44475a"),
        }
    },
    
    "Monokai Pro": {  # Тёмная тема Monokai Pro
        "type": "dark",
        "colors": {
            "bg": QColor("#2d2a2e"),
            "fg": QColor("#fcfcfa"),
            # Цвета заголовка окна
            "title_bg": QColor("#403e41"),           # Фон заголовка
            "title_bg_darker": QColor("#333134"),    # Чуть темнее title_bg
            "title_fg": QColor("#fcfcfa"),           # Текст заголовка
            "title_border": QColor("#5b595c"),       # Граница заголовка
            "title_inactive_bg": QColor("#3a383b"),  # Неактивный фон
            "title_inactive_fg": QColor("#b8b8b6"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#ff6188"),
            "type": QColor("#78dce8"),
            "comment": QColor("#727072"),
            "string": QColor("#ffd866"),
            "number": QColor("#ab9df2"),
            "operator": QColor("#ff6188"),
            "function": QColor("#a9dc76"),
            "modifier": QColor("#fc9867"),
            "directive": QColor("#ff6188"),
            "literal": QColor("#ab9df2"),
            "namespace": QColor("#78dce8"),
            "special": QColor("#ff6188"),
            "object": QColor("#ffd866"),
            "caret": QColor("#fcfcfa"),
            "caret_line": QColor("#403e41"),
            "margin_bg": QColor("#221f22"),
            "margin_fg": QColor("#727072"),
            "selection_bg": QColor("#5b595c"),
            "selection_fg": QColor("#fcfcfa"),
            "brace_bg": QColor("#ff618855"),
            "brace_fg": QColor("#ff6188"),
            "error": QColor("#ff6188"),
            "warning": QColor("#fc9867"),
            "autosave_on": QColor("#a9dc76"),
            "autosave_off": QColor("#ff6188"),
            "ls_active": QColor("#a9dc76"),
            "ls_idle": QColor("#78dce8"),
            "status_bg": QColor("#221f22"),
            "status_fg": QColor("#fcfcfa"),
            "status_border": QColor("#403e41"),
        }
    },
    
    "One Dark Pro": {  # Тёмная тема One Dark Pro
        "type": "dark",
        "colors": {
            "bg": QColor("#282c34"),
            "fg": QColor("#abb2bf"),
            # Цвета заголовка окна
            "title_bg": QColor("#2c313a"),           # Фон заголовка
            "title_bg_darker": QColor("#21262e"),    # Чуть темнее title_bg
            "title_fg": QColor("#abb2bf"),           # Текст заголовка
            "title_border": QColor("#3e4451"),       # Граница заголовка
            "title_inactive_bg": QColor("#252a32"),  # Неактивный фон
            "title_inactive_fg": QColor("#7f8c9a"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#c678dd"),
            "type": QColor("#e5c07b"),
            "comment": QColor("#5c6370"),
            "string": QColor("#98c379"),
            "number": QColor("#d19a66"),
            "operator": QColor("#56b6c2"),
            "function": QColor("#61afef"),
            "modifier": QColor("#c678dd"),
            "directive": QColor("#e06c75"),
            "literal": QColor("#d19a66"),
            "namespace": QColor("#e5c07b"),
            "special": QColor("#c678dd"),
            "object": QColor("#98c379"),
            "caret": QColor("#abb2bf"),
            "caret_line": QColor("#2c313a"),
            "margin_bg": QColor("#21252b"),
            "margin_fg": QColor("#5c6370"),
            "selection_bg": QColor("#3e4451"),
            "selection_fg": QColor("#abb2bf"),
            "brace_bg": QColor("#c678dd55"),
            "brace_fg": QColor("#c678dd"),
            "error": QColor("#e06c75"),
            "warning": QColor("#e5c07b"),
            "autosave_on": QColor("#98c379"),
            "autosave_off": QColor("#e06c75"),
            "ls_active": QColor("#98c379"),
            "ls_idle": QColor("#61afef"),
            "status_bg": QColor("#21252b"),
            "status_fg": QColor("#abb2bf"),
            "status_border": QColor("#3e4451"),
        }
    },
    
    "Github Light": {  # Светлая тема Github Light
        "type": "light",
        "colors": {
            "bg": QColor("#ffffff"),
            "fg": QColor("#24292e"),
            # Цвета заголовка окна
            "title_bg": QColor("#f6f8fa"),           # Фон заголовка
            "title_bg_darker": QColor("#e4e7eb"),    # Чуть темнее title_bg
            "title_fg": QColor("#24292e"),           # Текст заголовка
            "title_border": QColor("#e1e4e8"),       # Граница заголовка
            "title_inactive_bg": QColor("#fafbfc"),  # Неактивный фон
            "title_inactive_fg": QColor("#586069"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#d73a49"),
            "type": QColor("#005cc5"),
            "comment": QColor("#6a737d"),
            "string": QColor("#032f62"),
            "number": QColor("#005cc5"),
            "operator": QColor("#d73a49"),
            "function": QColor("#6f42c1"),
            "modifier": QColor("#005cc5"),
            "directive": QColor("#d73a49"),
            "literal": QColor("#032f62"),
            "namespace": QColor("#005cc5"),
            "special": QColor("#d73a49"),
            "object": QColor("#032f62"),
            "caret": QColor("#24292e"),
            "caret_line": QColor("#f6f8fa"),
            "margin_bg": QColor("#f6f8fa"),
            "margin_fg": QColor("#6a737d"),
            "selection_bg": QColor("#e0e0e0"),
            "selection_fg": QColor("#ffffff"),
            "brace_bg": QColor("#0366d655"),
            "brace_fg": QColor("#0366d6"),
            "error": QColor("#d73a49"),
            "warning": QColor("#e36209"),
            "autosave_on": QColor("#28a745"),
            "autosave_off": QColor("#d73a49"),
            "ls_active": QColor("#28a745"),
            "ls_idle": QColor("#0366d6"),
            "status_bg": QColor("#24292e"),
            "status_fg": QColor("#f6f8fa"),
            "status_border": QColor("#e1e4e8"),
        }
    },
    
    "Visual Studio Light": {  # Светлая тема Visual Studio Light
        "type": "light",
        "colors": {
            "bg": QColor("#ffffff"),
            "fg": QColor("#000000"),
            # Цвета заголовка окна
            "title_bg": QColor("#f0f0f0"),           # Фон заголовка
            "title_bg_darker": QColor("#dddddd"),    # Чуть темнее title_bg
            "title_fg": QColor("#000000"),           # Текст заголовка
            "title_border": QColor("#cccccc"),       # Граница заголовка
            "title_inactive_bg": QColor("#f5f5f5"),  # Неактивный фон
            "title_inactive_fg": QColor("#666666"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#0000ff"),
            "type": QColor("#2b91af"),
            "comment": QColor("#008000"),
            "string": QColor("#a31515"),
            "number": QColor("#09885a"),
            "operator": QColor("#000000"),
            "function": QColor("#795e26"),
            "modifier": QColor("#0000ff"),
            "directive": QColor("#008000"),
            "literal": QColor("#a31515"),
            "namespace": QColor("#267f99"),
            "special": QColor("#ff0000"),
            "object": QColor("#2b91af"),
            "caret": QColor("#000000"),
            "caret_line": QColor("#f0f0f0"),
            "margin_bg": QColor("#f0f0f0"),
            "margin_fg": QColor("#606060"),
            "selection_bg": QColor("#fff8d3"),
            "selection_fg": QColor("#000000"),
            "brace_bg": QColor("#0078d755"),
            "brace_fg": QColor("#0078d7"),
            "error": QColor("#ff0000"),
            "warning": QColor("#ff8c00"),
            "autosave_on": QColor("#107c10"),
            "autosave_off": QColor("#d73a49"),
            "ls_active": QColor("#107c10"),
            "ls_idle": QColor("#0078d7"),
            "status_bg": QColor("#2d2d2d"),
            "status_fg": QColor("#ffffff"),
            "status_border": QColor("#cccccc"),
        }
    },
    
    "Ayu Dark": {
        "type": "dark",
        "colors": {
            "bg": QColor("#0a0e14"),
            "fg": QColor("#b3b1ad"),
            # Цвета заголовка окна
            "title_bg": QColor("#1a1f29"),           # Фон заголовка
            "title_bg_darker": QColor("#0f141e"),    # Чуть темнее title_bg
            "title_fg": QColor("#e6e1cf"),           # Текст заголовка
            "title_border": QColor("#273747"),       # Граница заголовка
            "title_inactive_bg": QColor("#131721"),  # Неактивный фон
            "title_inactive_fg": QColor("#8a8f9a"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#ff8f40"),
            "type": QColor("#39bae6"),
            "comment": QColor("#626a73"),
            "string": QColor("#c2d94c"),
            "number": QColor("#d2a6ff"),
            "operator": QColor("#f29668"),
            "function": QColor("#ffb454"),
            "modifier": QColor("#4086ff"),
            "directive": QColor("#e6b450"),
            "literal": QColor("#a37acc"),
            "namespace": QColor("#39bae6"),
            "special": QColor("#f07178"),
            "object": QColor("#95e6cb"),
            "caret": QColor("#e6e1cf"),
            "caret_line": QColor("#1a1f29"),
            "margin_bg": QColor("#0d1017"),
            "margin_fg": QColor("#3e4b59"),
            "selection_bg": QColor("#273747"),
            "selection_fg": QColor("#e6e1cf"),
            "brace_bg": QColor("#ff8f4055"),
            "brace_fg": QColor("#ff8f40"),
            "error": QColor("#f07178"),
            "warning": QColor("#ffb454"),
            "autosave_on": QColor("#c2d94c"),
            "autosave_off": QColor("#f07178"),
            "ls_active": QColor("#c2d94c"),
            "ls_idle": QColor("#39bae6"),
            "status_bg": QColor("#0d1017"),
            "status_fg": QColor("#b3b1ad"),
            "status_border": QColor("#273747"),
        }
    },
    
    "Ayu Light": {
        "type": "light",
        "colors": {
            "bg": QColor("#fafafa"),
            "fg": QColor("#5c6166"),
            # Цвета заголовка окна
            "title_bg": QColor("#e9e9e9"),           # Фон заголовка
            "title_bg_darker": QColor("#d8d8d8"),    # Чуть темнее title_bg
            "title_fg": QColor("#5c6166"),           # Текст заголовка
            "title_border": QColor("#abb0b6"),       # Граница заголовка
            "title_inactive_bg": QColor("#f0f0f0"),  # Неактивный фон
            "title_inactive_fg": QColor("#7c8187"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#fa8d3e"),
            "type": QColor("#399ee6"),
            "comment": QColor("#abb0b6"),
            "string": QColor("#86b300"),
            "number": QColor("#a37acc"),
            "operator": QColor("#ed9366"),
            "function": QColor("#f2ae49"),
            "modifier": QColor("#6a3efa"),
            "directive": QColor("#f2ae49"),
            "literal": QColor("#a37acc"),
            "namespace": QColor("#399ee6"),
            "special": QColor("#f07171"),
            "object": QColor("#4cbf99"),
            "caret": QColor("#5c6166"),
            "caret_line": QColor("#f0f0f0"),
            "margin_bg": QColor("#e9e9e9"),
            "margin_fg": QColor("#abb0b6"),
            "selection_bg": QColor("#d4d4d4"),
            "selection_fg": QColor("#5c6166"),
            "brace_bg": QColor("#fa8d3e55"),
            "brace_fg": QColor("#fa8d3e"),
            "error": QColor("#f07171"),
            "warning": QColor("#f2ae49"),
            "autosave_on": QColor("#86b300"),
            "autosave_off": QColor("#f07171"),
            "ls_active": QColor("#86b300"),
            "ls_idle": QColor("#399ee6"),
            "status_bg": QColor("#2d3c4d"),
            "status_fg": QColor("#fafafa"),
            "status_border": QColor("#abb0b6"),
        }
    },
    
    "Everforest Dark": {
        "type": "dark",
        "colors": {
            "bg": QColor("#2d3c4d"),
            "fg": QColor("#d3c6aa"),
            # Цвета заголовка окна
            "title_bg": QColor("#3a4a5a"),           # Фон заголовка
            "title_bg_darker": QColor("#2f3f4f"),    # Чуть темнее title_bg
            "title_fg": QColor("#d3c6aa"),           # Текст заголовка
            "title_border": QColor("#4a5a6a"),       # Граница заголовка
            "title_inactive_bg": QColor("#344453"),  # Неактивный фон
            "title_inactive_fg": QColor("#9faf94"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#e6986c"),
            "type": QColor("#7fbbb3"),
            "comment": QColor("#859289"),
            "string": QColor("#a7c080"),
            "number": QColor("#e67e80"),
            "operator": QColor("#e6986c"),
            "function": QColor("#dbbc7f"),
            "modifier": QColor("#e6986c"),
            "directive": QColor("#e67e80"),
            "literal": QColor("#e67e80"),
            "namespace": QColor("#7fbbb3"),
            "special": QColor("#e67e80"),
            "object": QColor("#a7c080"),
            "caret": QColor("#d3c6aa"),
            "caret_line": QColor("#3a4a5a"),
            "margin_bg": QColor("#26323f"),
            "margin_fg": QColor("#859289"),
            "selection_bg": QColor("#4a5a6a"),
            "selection_fg": QColor("#d3c6aa"),
            "brace_bg": QColor("#e6986c55"),
            "brace_fg": QColor("#e6986c"),
            "error": QColor("#e67e80"),
            "warning": QColor("#e6986c"),
            "autosave_on": QColor("#a7c080"),
            "autosave_off": QColor("#e67e80"),
            "ls_active": QColor("#a7c080"),
            "ls_idle": QColor("#7fbbb3"),
            "status_bg": QColor("#26323f"),
            "status_fg": QColor("#d3c6aa"),
            "status_border": QColor("#4a5a6a"),
        }
    },
    
    "Everforest Light": {
        "type": "light",
        "colors": {
            "bg": QColor("#fef6e4"),
            "fg": QColor("#5c6a72"),
            # Цвета заголовка окна
            "title_bg": QColor("#f0e8d8"),           # Фон заголовка
            "title_bg_darker": QColor("#e0d8c8"),    # Чуть темнее title_bg
            "title_fg": QColor("#5c6a72"),           # Текст заголовка
            "title_border": QColor("#d0d8d0"),       # Граница заголовка
            "title_inactive_bg": QColor("#f5eddd"),  # Неактивный фон
            "title_inactive_fg": QColor("#7c8a92"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#f57b5c"),
            "type": QColor("#4f9e9e"),
            "comment": QColor("#939f91"),
            "string": QColor("#8da101"),
            "number": QColor("#e65e5e"),
            "operator": QColor("#f57b5c"),
            "function": QColor("#dfb256"),
            "modifier": QColor("#f57b5c"),
            "directive": QColor("#e65e5e"),
            "literal": QColor("#e65e5e"),
            "namespace": QColor("#4f9e9e"),
            "special": QColor("#e65e5e"),
            "object": QColor("#8da101"),
            "caret": QColor("#5c6a72"),
            "caret_line": QColor("#f0e8d8"),
            "margin_bg": QColor("#e8e0d0"),
            "margin_fg": QColor("#939f91"),
            "selection_bg": QColor("#d0d8d0"),
            "selection_fg": QColor("#5c6a72"),
            "brace_bg": QColor("#f57b5c55"),
            "brace_fg": QColor("#f57b5c"),
            "error": QColor("#e65e5e"),
            "warning": QColor("#f57b5c"),
            "autosave_on": QColor("#8da101"),
            "autosave_off": QColor("#e65e5e"),
            "ls_active": QColor("#8da101"),
            "ls_idle": QColor("#4f9e9e"),
            "status_bg": QColor("#3a4a5a"),
            "status_fg": QColor("#fef6e4"),
            "status_border": QColor("#939f91"),
        }
    },
    
    "Night Owl": {
        "type": "dark",
        "colors": {
            "bg": QColor("#011627"),
            "fg": QColor("#d6deeb"),
            # Цвета заголовка окна
            "title_bg": QColor("#0b2942"),           # Фон заголовка
            "title_bg_darker": QColor("#051e33"),    # Чуть темнее title_bg
            "title_fg": QColor("#d6deeb"),           # Текст заголовка
            "title_border": QColor("#1d3b53"),       # Граница заголовка
            "title_inactive_bg": QColor("#082030"),  # Неактивный фон
            "title_inactive_fg": QColor("#98a8b5"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#c792ea"),
            "type": QColor("#82aaff"),
            "comment": QColor("#637777"),
            "string": QColor("#ecc48d"),
            "number": QColor("#f78c6c"),
            "operator": QColor("#c792ea"),
            "function": QColor("#82aaff"),
            "modifier": QColor("#c792ea"),
            "directive": QColor("#7fdbca"),
            "literal": QColor("#f78c6c"),
            "namespace": QColor("#82aaff"),
            "special": QColor("#ff5874"),
            "object": QColor("#addb67"),
            "caret": QColor("#d6deeb"),
            "caret_line": QColor("#0b2942"),
            "margin_bg": QColor("#01111d"),
            "margin_fg": QColor("#2e3d4f"),
            "selection_bg": QColor("#1d3b53"),
            "selection_fg": QColor("#d6deeb"),
            "brace_bg": QColor("#c792ea55"),
            "brace_fg": QColor("#c792ea"),
            "error": QColor("#ff5874"),
            "warning": QColor("#c792ea"),
            "autosave_on": QColor("#addb67"),
            "autosave_off": QColor("#ff5874"),
            "ls_active": QColor("#addb67"),
            "ls_idle": QColor("#82aaff"),
            "status_bg": QColor("#01111d"),
            "status_fg": QColor("#d6deeb"),
            "status_border": QColor("#1d3b53"),
        }
    },
    
    "Rosé Pine": {
        "type": "dark",
        "colors": {
            "bg": QColor("#191724"),
            "fg": QColor("#e0def4"),
            # Цвета заголовка окна
            "title_bg": QColor("#26233a"),           # Фон заголовка
            "title_bg_darker": QColor("#1c192d"),    # Чуть темнее title_bg
            "title_fg": QColor("#e0def4"),           # Текст заголовка
            "title_border": QColor("#403d52"),       # Граница заголовка
            "title_inactive_bg": QColor("#1f1d2e"),  # Неактивный фон
            "title_inactive_fg": QColor("#a5a2c4"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#ebbcba"),
            "type": QColor("#9ccfd8"),
            "comment": QColor("#6e6a86"),
            "string": QColor("#f6c177"),
            "number": QColor("#c4a7e7"),
            "operator": QColor("#e0def4"),
            "function": QColor("#ebbcba"),
            "modifier": QColor("#c4a7e7"),
            "directive": QColor("#eb6f92"),
            "literal": QColor("#f6c177"),
            "namespace": QColor("#9ccfd8"),
            "special": QColor("#eb6f92"),
            "object": QColor("#c4a7e7"),
            "caret": QColor("#e0def4"),
            "caret_line": QColor("#26233a"),
            "margin_bg": QColor("#1f1d2e"),
            "margin_fg": QColor("#6e6a86"),
            "selection_bg": QColor("#403d52"),
            "selection_fg": QColor("#e0def4"),
            "brace_bg": QColor("#ebbcba55"),
            "brace_fg": QColor("#ebbcba"),
            "error": QColor("#eb6f92"),
            "warning": QColor("#f6c177"),
            "autosave_on": QColor("#9ccfd8"),
            "autosave_off": QColor("#eb6f92"),
            "ls_active": QColor("#9ccfd8"),
            "ls_idle": QColor("#c4a7e7"),
            "status_bg": QColor("#1f1d2e"),
            "status_fg": QColor("#e0def4"),
            "status_border": QColor("#403d52"),
        }
    },
    
    "GitHub Dark": {
        "type": "dark",
        "colors": {
            "bg": QColor("#0d1117"),
            "fg": QColor("#e6edf3"),
            # Цвета заголовка окна
            "title_bg": QColor("#161b22"),           # Фон заголовка
            "title_bg_darker": QColor("#0c1118"),    # Чуть темнее title_bg
            "title_fg": QColor("#e6edf3"),           # Текст заголовка
            "title_border": QColor("#30363d"),       # Граница заголовка
            "title_inactive_bg": QColor("#12181f"),  # Неактивный фон
            "title_inactive_fg": QColor("#b1bac4"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#ff7b72"),
            "type": QColor("#79c0ff"),
            "comment": QColor("#8b949e"),
            "string": QColor("#a5d6ff"),
            "number": QColor("#79c0ff"),
            "operator": QColor("#ff7b72"),
            "function": QColor("#d2a8ff"),
            "modifier": QColor("#ff7b72"),
            "directive": QColor("#ff7b72"),
            "literal": QColor("#a5d6ff"),
            "namespace": QColor("#79c0ff"),
            "special": QColor("#ffa198"),
            "object": QColor("#a5d6ff"),
            "caret": QColor("#e6edf3"),
            "caret_line": QColor("#161b22"),
            "margin_bg": QColor("#0b1015"),
            "margin_fg": QColor("#6e7681"),
            "selection_bg": QColor("#264f78"),
            "selection_fg": QColor("#e6edf3"),
            "brace_bg": QColor("#ff7b7255"),
            "brace_fg": QColor("#ff7b72"),
            "error": QColor("#f85149"),
            "warning": QColor("#d29922"),
            "autosave_on": QColor("#a5d6ff"),
            "autosave_off": QColor("#f85149"),
            "ls_active": QColor("#a5d6ff"),
            "ls_idle": QColor("#79c0ff"),
            "status_bg": QColor("#161b22"),
            "status_fg": QColor("#e6edf3"),
            "status_border": QColor("#30363d"),
        }
    },
    
    "Material Ocean": {
        "type": "dark",
        "colors": {
            "bg": QColor("#0f111a"),
            "fg": QColor("#8f93a2"),
            # Цвета заголовка окна
            "title_bg": QColor("#1a1c28"),           # Фон заголовка
            "title_bg_darker": QColor("#11131e"),    # Чуть темнее title_bg
            "title_fg": QColor("#8f93a2"),           # Текст заголовка
            "title_border": QColor("#2c3e4a"),       # Граница заголовка
            "title_inactive_bg": QColor("#141622"),  # Неактивный фон
            "title_inactive_fg": QColor("#6a6e7d"),  # Неактивный текст
            # Остальные цвета
            "keyword": QColor("#c792ea"),
            "type": QColor("#82aaff"),
            "comment": QColor("#5c6773"),
            "string": QColor("#c3e88d"),
            "number": QColor("#f78c6c"),
            "operator": QColor("#89ddff"),
            "function": QColor("#82aaff"),
            "modifier": QColor("#c792ea"),
            "directive": QColor("#c792ea"),
            "literal": QColor("#f78c6c"),
            "namespace": QColor("#82aaff"),
            "special": QColor("#f07178"),
            "object": QColor("#c3e88d"),
            "caret": QColor("#8f93a2"),
            "caret_line": QColor("#1a1c28"),
            "margin_bg": QColor("#0b0d14"),
            "margin_fg": QColor("#3e4a5a"),
            "selection_bg": QColor("#2c3e4a"),
            "selection_fg": QColor("#8f93a2"),
            "brace_bg": QColor("#c792ea55"),
            "brace_fg": QColor("#c792ea"),
            "error": QColor("#f07178"),
            "warning": QColor("#f78c6c"),
            "autosave_on": QColor("#c3e88d"),
            "autosave_off": QColor("#f07178"),
            "ls_active": QColor("#c3e88d"),
            "ls_idle": QColor("#82aaff"),
            "status_bg": QColor("#0b0d14"),
            "status_fg": QColor("#8f93a2"),
            "status_border": QColor("#2c3e4a"),
        }
    },
    
}