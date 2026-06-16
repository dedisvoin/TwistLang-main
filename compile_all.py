import os
from colorama import Fore
import time
try:
    start_t = time.time()
    os.system('D:/clang/clang+llvm-21.1.1-x86_64-pc-windows-msvc/bin/clang.exe lumenc.cpp -O3 -std=c++17 -o bin/lumenc.exe')
    print(Fore.GREEN + "lumenc.exe success compiled." + Fore.BLACK, str(round(time.time() - start_t, 2)) + "ms" + Fore.RESET)
except:...

try:
    start_t = time.time()
    os.system('D:/clang/clang+llvm-21.1.1-x86_64-pc-windows-msvc/bin/clang.exe lumenc.cpp -O3 -DSERVER -std=c++17 -o bin/lumen-ls.exe')
    print(Fore.GREEN + "lumen-ls.exe success compiled." + Fore.BLACK, str(round(time.time() - start_t, 2)) + "ms" + Fore.RESET)
except:...
