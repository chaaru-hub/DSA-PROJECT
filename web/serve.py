"""
Local web server for the BST Visualizer (helper only - the project itself is C++).

Run:   python serve.py
It finds a free port (8000, 8001, ...), prints the direct localhost link
and opens it in your browser. Press Ctrl+C to stop.
"""
import http.server
import os
import socketserver
import threading
import webbrowser

os.chdir(os.path.dirname(os.path.abspath(__file__)))


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".js": "application/javascript", ".wasm": "application/wasm",
                      ".html": "text/html"}

    def log_message(self, *args):   # keep the window clean
        pass


server = None
for port in range(8000, 8020):
    try:
        server = socketserver.TCPServer(("127.0.0.1", port), Handler)
        break
    except OSError:
        continue                    # port busy, try the next one
if server is None:
    raise SystemExit("No free port between 8000 and 8019.")

url = f"http://localhost:{port}"
print()
print("  =========================================================")
print()
print("     BST Visualizer is running. Open this link:")
print()
print(f"         {url}")
print()
print("     (Ctrl+Click the link, or copy it into Chrome / Edge)")
print("     Keep this window open. Press Ctrl+C to stop.")
print()
print("  =========================================================")
print(flush=True)
threading.Timer(1.5, lambda: webbrowser.open(url)).start()
try:
    server.serve_forever()
except KeyboardInterrupt:
    print("Server stopped.")
