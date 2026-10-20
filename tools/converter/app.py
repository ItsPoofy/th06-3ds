"""Windows folder-picker front end for the TH06 asset converter."""
from pathlib import Path
import sys
import threading
import queue
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pipeline import convert
from diagnostics import error_message, write_error_log


def main():
    window = tk.Tk(); window.withdraw()
    source = filedialog.askdirectory(title='Select your Touhou 6 game folder', mustexist=True)
    if not source:
        window.destroy(); return
    resources = Path(getattr(sys, '_MEIPASS', Path(__file__).parent))/'resources'
    executable = Path(sys.executable if getattr(sys, 'frozen', False) else __file__).resolve()
    output = executable.parent/'assets'
    window.title('TH06 Asset Converter'); window.resizable(False, False)
    status = tk.StringVar(value='Preparing…')
    ttk.Label(window, textvariable=status, wraplength=440, padding=20).pack()
    progress = ttk.Progressbar(window, mode='indeterminate', length=420)
    progress.pack(padx=20, pady=(0, 20)); progress.start()
    window.deiconify(); window.protocol('WM_DELETE_WINDOW', lambda: None)
    events = queue.Queue()
    buttons = ttk.Frame(window)

    def ready():
        progress.stop()
        window.protocol('WM_DELETE_WINDOW', window.destroy)
        if not buttons.winfo_children():
            ttk.Button(buttons, text='Close', command=window.destroy).pack(side='left', padx=4)
            buttons.pack(padx=16, pady=(0, 16))

    def worker():
        stage = 'Checking the selected game folder'

        def report(text):
            nonlocal stage
            stage = text
            events.put(('status', text))

        try:
            convert(source, output, resources, report)
            events.put(('done', str(output)))
        except Exception as error:
            log_path = write_error_log(error, stage, source, output, executable.parent)
            events.put(('error', error_message(error, stage, log_path)))

    def poll():
        while not events.empty():
            kind, text = events.get()
            if kind == 'status':
                status.set(text)
            else:
                progress.stop()
                if kind == 'done':
                    status.set(f'Assets ready: {output}\n\nRetexture graphics, sound sets, and TH20 sprites are included.\nCopy assets to sd:/3ds/th06/assets/.')
                    ready()
                else:
                    status.set(text)
                    ready()
                    messagebox.showerror('Conversion failed', text, parent=window)
        window.after(100, poll)

    threading.Thread(target=worker, daemon=True).start()
    window.after(100, poll); window.mainloop()


if __name__ == '__main__':
    main()
