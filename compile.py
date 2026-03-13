import os
from colorama import Fore
import time
try:
    start_t = time.time()
    os.system('clang twistc.cpp -O3 -std=c++17 -o bin/twistc.exe')
    print(Fore.GREEN + "twistc.exe success compiled." + Fore.BLACK, str(round(time.time() - start_t, 2)) + "ms" + Fore.RESET)
except:...

try:
    start_t = time.time()
    os.system('clang twistc.cpp -DSERVER -O3 -std=c++17 -o bin/twist-ls.exe')
    print(Fore.GREEN + "twist-ls.exe success compiled." + Fore.BLACK, str(round(time.time() - start_t, 2)) + "ms" + Fore.RESET)
except:...