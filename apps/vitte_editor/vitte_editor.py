#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

KEYWORDS = [
    "use", "space", "pick", "form", "proc", "entry", "if", "else", "loop", "break",
    "continue", "return", "let", "set", "match", "when", "case", "true", "false",
    "unsafe", "macro", "trait", "emit", "pull", "share", "as", "in", "and", "or", "not"
]

TOKEN_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_/]*")

class EditorTab:
    def __init__(self, notebook, path=None):
        self.path = path
        self.frame = ttk.Frame(notebook)
        self.text = tk.Text(self.frame, wrap="none", undo=True)
        self.vsb = ttk.Scrollbar(self.frame, orient="vertical", command=self.text.yview)
        self.hsb = ttk.Scrollbar(self.frame, orient="horizontal", command=self.text.xview)
        self.text.configure(yscrollcommand=self.vsb.set, xscrollcommand=self.hsb.set)

        self.text.grid(row=0, column=0, sticky="nsew")
        self.vsb.grid(row=0, column=1, sticky="ns")
        self.hsb.grid(row=1, column=0, sticky="ew")

        self.frame.rowconfigure(0, weight=1)
        self.frame.columnconfigure(0, weight=1)

        self.configure_tags()
        self.text.bind("<KeyRelease>", self.on_edit)
        self.text.bind("<Control-space>", self.show_completion)

    def configure_tags(self):
        self.text.tag_configure("kw", foreground="#0057D8")
        self.text.tag_configure("num", foreground="#8F3F00")
        self.text.tag_configure("str", foreground="#0A7C2F")
        self.text.tag_configure("comment", foreground="#7A7A7A")

    def load_file(self, path):
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", content)
        self.path = path
        self.highlight_all()

    def save(self):
        if not self.path:
            return False
        data = self.text.get("1.0", tk.END)
        with open(self.path, "w", encoding="utf-8") as f:
            f.write(data.rstrip("\n") + "\n")
        return True

    def on_edit(self, _event=None):
        self.highlight_all()

    def highlight_all(self):
        txt = self.text.get("1.0", tk.END)
        for tag in ("kw", "num", "str", "comment"):
            self.text.tag_remove(tag, "1.0", tk.END)

        for m in re.finditer(r"//.*$", txt, flags=re.MULTILINE):
            self.add_span("comment", m.start(), m.end())

        for m in re.finditer(r'"([^"\\]|\\.)*"', txt):
            self.add_span("str", m.start(), m.end())

        for m in re.finditer(r"\b\d+\b", txt):
            self.add_span("num", m.start(), m.end())

        for kw in KEYWORDS:
            for m in re.finditer(r"\b" + re.escape(kw) + r"\b", txt):
                self.add_span("kw", m.start(), m.end())

    def add_span(self, tag, start, end):
        start_idx = self.offset_to_index(start)
        end_idx = self.offset_to_index(end)
        self.text.tag_add(tag, start_idx, end_idx)

    def offset_to_index(self, offset):
        return f"1.0+{offset}c"

    def completion_prefix(self):
        pos = self.text.index(tk.INSERT)
        line, col = map(int, pos.split("."))
        start = f"{line}.0"
        current = self.text.get(start, pos)
        m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)$", current)
        return m.group(1) if m else ""

    def show_completion(self, _event=None):
        prefix = self.completion_prefix()
        if not prefix:
            return "break"

        body = self.text.get("1.0", tk.END)
        symbols = sorted(set(TOKEN_RE.findall(body)))
        items = [k for k in KEYWORDS if k.startswith(prefix)]
        items += [s for s in symbols if s.startswith(prefix) and s not in items]
        items = items[:80]
        if not items:
            return "break"

        popup = tk.Toplevel(self.text)
        popup.wm_overrideredirect(True)
        popup.attributes("-topmost", True)
        lb = tk.Listbox(popup, height=min(10, len(items)), width=40)
        lb.pack(fill="both", expand=True)
        for it in items:
            lb.insert(tk.END, it)
        lb.selection_set(0)

        bbox = self.text.bbox(tk.INSERT)
        if bbox:
            x, y, w, h = bbox
            abs_x = self.text.winfo_rootx() + x
            abs_y = self.text.winfo_rooty() + y + h
            popup.geometry(f"+{abs_x}+{abs_y}")

        def apply_and_close(_event=None):
            sel = lb.curselection()
            if not sel:
                popup.destroy()
                return "break"
            picked = lb.get(sel[0])
            if picked != prefix:
                self.text.insert(tk.INSERT, picked[len(prefix):])
            popup.destroy()
            return "break"

        def close(_event=None):
            popup.destroy()
            return "break"

        lb.bind("<Return>", apply_and_close)
        lb.bind("<Double-Button-1>", apply_and_close)
        lb.bind("<Escape>", close)
        lb.focus_set()
        return "break"


class VitteEditorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("VitteEditor")
        self.project_dir = None
        self.tabs = {}

        self.build_ui()

    def build_ui(self):
        self.root.geometry("1320x860")

        self.main = ttk.PanedWindow(self.root, orient="horizontal")
        self.main.pack(fill="both", expand=True)

        left = ttk.Frame(self.main)
        center = ttk.Frame(self.main)
        right = ttk.Frame(self.main)

        self.main.add(left, weight=2)
        self.main.add(center, weight=8)
        self.main.add(right, weight=3)

        ttk.Label(left, text="Projet").pack(anchor="w", padx=6, pady=6)
        self.file_list = tk.Listbox(left)
        self.file_list.pack(fill="both", expand=True, padx=6, pady=6)
        self.file_list.bind("<Double-Button-1>", self.open_selected_project_file)

        self.notebook = ttk.Notebook(center)
        self.notebook.pack(fill="both", expand=True)

        ttk.Label(right, text="Diagnostics / Suggestions").pack(anchor="w", padx=6, pady=6)
        self.diag_list = tk.Listbox(right)
        self.diag_list.pack(fill="both", expand=True, padx=6, pady=6)
        self.diag_list.bind("<Double-Button-1>", self.jump_to_diag)

        btns = ttk.Frame(right)
        btns.pack(fill="x", padx=6, pady=6)
        ttk.Button(btns, text="Check actif", command=self.check_active).pack(fill="x", pady=2)
        ttk.Button(btns, text="Check projet", command=self.check_project).pack(fill="x", pady=2)
        ttk.Button(btns, text="Sauver actif", command=self.save_active).pack(fill="x", pady=2)
        ttk.Button(btns, text="Sauver tout", command=self.save_all).pack(fill="x", pady=2)

        self.status = tk.StringVar(value="Pret")
        ttk.Label(self.root, textvariable=self.status, anchor="w").pack(fill="x")

        self.diag_payload = []

        self.build_menu()

    def build_menu(self):
        m = tk.Menu(self.root)
        f = tk.Menu(m, tearoff=0)
        f.add_command(label="Nouveau", command=self.new_tab)
        f.add_command(label="Ouvrir fichier", command=self.open_file_dialog)
        f.add_command(label="Ouvrir dossier", command=self.open_project_dialog)
        f.add_separator()
        f.add_command(label="Sauver", command=self.save_active)
        f.add_command(label="Sauver tout", command=self.save_all)
        f.add_separator()
        f.add_command(label="Quitter", command=self.root.quit)
        m.add_cascade(label="Fichier", menu=f)

        b = tk.Menu(m, tearoff=0)
        b.add_command(label="Check actif", command=self.check_active)
        b.add_command(label="Check projet", command=self.check_project)
        m.add_cascade(label="Build", menu=b)

        self.root.config(menu=m)

    def new_tab(self):
        tab = EditorTab(self.notebook)
        key = str(id(tab))
        self.tabs[key] = tab
        self.notebook.add(tab.frame, text="untitled.vit")
        self.notebook.select(tab.frame)
        self.status.set("Nouveau fichier")

    def open_file_dialog(self):
        path = filedialog.askopenfilename(filetypes=[("Vitte", "*.vit"), ("All", "*")])
        if path:
            self.open_file(path)

    def open_project_dialog(self):
        d = filedialog.askdirectory()
        if d:
            self.project_dir = d
            self.refresh_project_files()
            self.status.set(f"Projet: {d}")

    def refresh_project_files(self):
        self.file_list.delete(0, tk.END)
        if not self.project_dir:
            return
        files = []
        for root, _dirs, names in os.walk(self.project_dir):
            for n in names:
                if n.endswith(".vit"):
                    files.append(os.path.relpath(os.path.join(root, n), self.project_dir))
        files.sort()
        for p in files:
            self.file_list.insert(tk.END, p)

    def open_selected_project_file(self, _event=None):
        sel = self.file_list.curselection()
        if not sel or not self.project_dir:
            return
        rel = self.file_list.get(sel[0])
        self.open_file(os.path.join(self.project_dir, rel))

    def open_file(self, path):
        abs_path = os.path.abspath(path)
        for tab in self.tabs.values():
            if tab.path and os.path.abspath(tab.path) == abs_path:
                self.notebook.select(tab.frame)
                return

        tab = EditorTab(self.notebook, abs_path)
        tab.load_file(abs_path)
        key = str(id(tab))
        self.tabs[key] = tab
        self.notebook.add(tab.frame, text=os.path.basename(abs_path))
        self.notebook.select(tab.frame)
        self.status.set(f"Ouvert: {abs_path}")

    def active_tab(self):
        current = self.notebook.select()
        if not current:
            return None
        for tab in self.tabs.values():
            if str(tab.frame) == current:
                return tab
        return None

    def save_active(self):
        tab = self.active_tab()
        if not tab:
            return
        if not tab.path:
            path = filedialog.asksaveasfilename(defaultextension=".vit", filetypes=[("Vitte", "*.vit")])
            if not path:
                return
            tab.path = path
            self.notebook.tab(tab.frame, text=os.path.basename(path))
        ok = tab.save()
        self.status.set("Sauve" if ok else "Echec sauvegarde")

    def save_all(self):
        for tab in list(self.tabs.values()):
            if tab.path:
                tab.save()
        self.status.set("Tous les fichiers ont ete sauvegardes")

    def parse_diag_json(self, payload):
        payload = payload.strip()
        if not payload:
            return []
        start = payload.find("{")
        if start == -1:
            return []
        try:
            obj = json.loads(payload[start:])
        except Exception:
            return []
        return obj.get("diagnostics", [])

    def run_check_file(self, path):
        cmd = ["vitte", "check", "--diag-json", path]
        proc = subprocess.run(cmd, text=True, capture_output=True)
        diags = self.parse_diag_json(proc.stdout + "\n" + proc.stderr)
        return proc.returncode, diags, proc.stdout, proc.stderr

    def show_diags(self, diags):
        self.diag_list.delete(0, tk.END)
        self.diag_payload = diags
        if not diags:
            self.diag_list.insert(tk.END, "Aucun diagnostic")
            return
        for d in diags:
            sev = d.get("severity", "?")
            code = d.get("code", "")
            msg = d.get("message", "")
            fil = d.get("file", "")
            line = d.get("start", 0)
            self.diag_list.insert(tk.END, f"[{sev}] {code} {fil}:{line} {msg}")

    def check_active(self):
        tab = self.active_tab()
        if not tab or not tab.path:
            messagebox.showinfo("VitteEditor", "Aucun fichier actif sauvegarde")
            return
        self.save_active()
        rc, diags, out, err = self.run_check_file(tab.path)
        self.show_diags(diags)
        self.status.set(f"Check actif rc={rc}")
        if rc != 0 and not diags:
            messagebox.showerror("Check", (out + "\n" + err).strip()[:3000])

    def check_project(self):
        if not self.project_dir:
            messagebox.showinfo("VitteEditor", "Ouvre un dossier projet")
            return
        all_diags = []
        for i in range(self.file_list.size()):
            rel = self.file_list.get(i)
            p = os.path.join(self.project_dir, rel)
            rc, diags, _out, _err = self.run_check_file(p)
            if diags:
                all_diags.extend(diags)
            if rc != 0 and not diags:
                all_diags.append({
                    "severity": "error",
                    "code": "E-CHECK",
                    "message": "check failed without JSON diagnostics",
                    "file": p,
                    "start": 1,
                })
        self.show_diags(all_diags)
        self.status.set(f"Check projet: {len(all_diags)} diagnostics")

    def jump_to_diag(self, _event=None):
        sel = self.diag_list.curselection()
        if not sel:
            return
        idx = sel[0]
        if idx >= len(self.diag_payload):
            return
        d = self.diag_payload[idx]
        path = d.get("file")
        line = int(d.get("start", 1) or 1)
        if not path:
            return
        if not os.path.isabs(path) and self.project_dir:
            path = os.path.join(self.project_dir, path)
        self.open_file(path)
        tab = self.active_tab()
        if tab:
            tab.text.mark_set(tk.INSERT, f"{line}.0")
            tab.text.see(f"{line}.0")
            tab.text.focus_set()


def main():
    root = tk.Tk()
    app = VitteEditorApp(root)
    if len(sys.argv) > 1:
        first = sys.argv[1]
        if os.path.isdir(first):
            app.project_dir = os.path.abspath(first)
            app.refresh_project_files()
        elif os.path.isfile(first):
            app.open_file(first)
    root.mainloop()


if __name__ == "__main__":
    main()
