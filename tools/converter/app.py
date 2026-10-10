"""Windows folder-picker front end for the TH06 asset converter."""
from pathlib import Path
import sys
import threading
import queue
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pipeline import convert
import optional_content


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

    def add_content(kind):
        path = filedialog.askdirectory(title='Select the '+('Touhou 20' if kind == 'th20' else 'Retexture Pack')+' folder', mustexist=True)
        if not path:
            return
        for button in buttons.winfo_children():
            button.config(state='disabled')
        progress.start()
        window.protocol('WM_DELETE_WINDOW', lambda: None)
        def add():
            try:
                getattr(optional_content, kind)(Path(path).resolve(), output, resources.resolve(), lambda text: events.put(('status', text)))
                events.put(('extra', 'Optional content added. Copy assets to sd:/3ds/th06/assets/.'))
            except Exception as error:
                events.put(('extra-error', str(error)))
        threading.Thread(target=add, daemon=True).start()

    def ready():
        progress.stop()
        window.protocol('WM_DELETE_WINDOW', window.destroy)
        if not buttons.winfo_children():
            ttk.Button(buttons, text='Add retexture pack…', command=lambda: add_content('retexture')).pack(side='left', padx=4)
            ttk.Button(buttons, text='Add TH20 graphics…', command=lambda: add_content('th20')).pack(side='left', padx=4)
            ttk.Button(buttons, text='Close', command=window.destroy).pack(side='left', padx=4)
            buttons.pack(padx=16, pady=(0, 16))
        for button in buttons.winfo_children():
            button.config(state='normal')

    def worker():
        try:
            convert(source, output, resources, lambda text: events.put(('status', text)))
            events.put(('done', str(output)))
        except Exception as error:
            events.put(('error', str(error)))

    def poll():
        while not events.empty():
            kind, text = events.get()
            if kind == 'status':
                status.set(text)
            else:
                progress.stop()
                if kind in ('done', 'extra'):
                    status.set(f'Assets ready: {output}\n\nCopy assets to sd:/3ds/th06/assets/.\nOptional: add your retexture pack or TH20 graphics below.')
                    ready()
                elif kind == 'extra-error':
                    messagebox.showerror('Optional import failed', text, parent=window)
                    ready()
                else:
                    messagebox.showerror('Conversion failed', text, parent=window)
                    window.destroy(); return
        window.after(100, poll)

    threading.Thread(target=worker, daemon=True).start()
    window.after(100, poll); window.mainloop()


if __name__ == '__main__':
    main()
