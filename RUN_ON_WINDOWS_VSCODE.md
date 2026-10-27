# Run the BST Visualizer on localhost (Windows + VS Code)

There are two ways to run the project:

* **A. In the browser on localhost**: easiest, only needs Python.
* **B. As the desktop C++ program**: needs the MSYS2 compiler and SDL2.

---

## 0. One-time setup

1. Install **VS Code**: <https://code.visualstudio.com>
2. Install **Git**: <https://git-scm.com/download/win>
3. Install **Python**: <https://www.python.org/downloads/>
   During the install, tick **"Add python.exe to PATH"**.

## 1. Get the project and open it in VS Code

Open VS Code, then press **Ctrl + `** to open the terminal and run:

```powershell
git clone https://github.com/chaaru-hub/DSA-PROJECT.git
cd DSA-PROJECT
code .
```

`code .` opens the project folder in VS Code. If VS Code asks
*"Do you trust the authors of the files in this folder?"*, click **Yes, I trust the authors**.

Already have the folder? Get the latest version instead:

```powershell
cd DSA-PROJECT
git pull
```

---

## A. Run in the browser on localhost (recommended)

**Option 1: one shortcut.** Press **Ctrl + Shift + B**. VS Code runs the task
*"Run on localhost (browser)"* and the browser opens the visualizer.

**Option 2: type the commands** in the VS Code terminal (**Ctrl + `**):

```powershell
cd web
python serve.py
```

The terminal prints the link, for example:

```
     BST Visualizer is running. Open this link:

         http://localhost:8000
```

**Ctrl + click** the link if the browser did not open by itself.
Keep the terminal open while you use the page. Press **Ctrl + C** in the terminal to stop the server.

> If `python` is not found, try `py serve.py`. If neither works, reinstall Python and tick
> "Add python.exe to PATH".

---

## B. Build and run the desktop program (C++ / SDL2)

### One-time setup: MSYS2 compiler and SDL2

1. Install **MSYS2** from <https://www.msys2.org> (keep the default folder `C:\msys64`).
2. Open **"MSYS2 UCRT64"** from the Start menu and run:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 make
   ```

### Build and run from VS Code

**Option 1: run the task.** Click **Terminal → Run Task… → "Build and run desktop app (MSYS2)"**.
It compiles the code and opens `TreeViz.exe`.

**Option 2: type the commands** in the **MSYS2 UCRT64** terminal:

```bash
cd /c/Users/<your-name>/DSA-PROJECT    # the folder where you cloned the project
make
./TreeViz.exe
```

To start `TreeViz.exe` by double-clicking it outside MSYS2, copy `SDL2.dll` from
`C:\msys64\ucrt64\bin\` into the project folder.

### Useful extras

| Command | What it does |
|---|---|
| `./TreeViz.exe --light` | start with the light theme (better on projectors) |
| `./TreeViz.exe --selftest` | 5000 random operations, checks the BST stays correct |
| `make clean` then `make` | rebuild everything from scratch |

---

## Using the visualizer

| Key / button | Action |
|---|---|
| type numbers, e.g. `50 30 70`, then **Enter** | insert them |
| **Delete** key or button | delete the typed number |
| **Search** | search for the typed number |
| **Inorder / Preorder / Postorder** | animate the traversal |
| **Clear** | remove all nodes |
| **Space** | pause / resume |
| **← / →** | step back / forward |
| **↑ / ↓** | faster / slower |
| **F2** | light / dark theme |
| **F11** | fullscreen |
