#!/usr/bin/env python3
"""Regenerate the per-spec PDFs exactly the way the yzane.markdown-pdf VSCode
extension does: markdown -> HTML (extension base CSS + docs/pdf-style.css) ->
bundled Chromium m80 -> Page.printToPDF (A4, 1.5/1/1/1 cm margins,
printBackground, no header/footer). Output PDFs land next to their .md sources.

Requires: the yzane.markdown-pdf 1.5.0 VSCode extension (for its Chromium and
CSS), python3-markdown, python3-websocket. The sandbox flags are mandatory on
recent kernels or Chromium crash-loops.
"""
import base64, json, os, subprocess, time, urllib.request
import markdown, websocket

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXT = os.path.expanduser("~/.vscode/extensions/yzane.markdown-pdf-1.5.0")
CHROME = EXT + "/node_modules/puppeteer-core/.local-chromium/linux-722234/chrome-linux/chrome"
PORT = 9223

DOCS = [
    "docs/IP-Specification/Cmod_A7_IP_Peripheral_Reference.md",
    "docs/Pin-Specification/Cmod_A7_Pin_Specification.md",
    "docs/Power-Specification/Cmod_A7_Power_Specification.md",
]

css = ""
for f in [EXT + "/styles/markdown.css", EXT + "/styles/markdown-pdf.css",
          REPO + "/docs/pdf-style.css"]:
    css += open(f).read() + "\n"


def md_to_html(md_path):
    body = markdown.markdown(open(md_path).read(),
                             extensions=["tables", "fenced_code"])
    return ("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            f"<style>{css}</style></head><body>{body}</body></html>")


def cdp(ws, method, params=None, _id=[0]):
    _id[0] += 1
    ws.send(json.dumps({"id": _id[0], "method": method, "params": params or {}}))
    while True:
        msg = json.loads(ws.recv())
        if msg.get("id") == _id[0]:
            if "error" in msg:
                raise RuntimeError(msg["error"])
            return msg.get("result", {})


chrome = subprocess.Popen(
    [CHROME, "--headless", "--no-sandbox", "--disable-gpu",
     "--disable-software-rasterizer", "--disable-dev-shm-usage",
     "--no-first-run", f"--remote-debugging-port={PORT}", "about:blank"],
    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
try:
    for _ in range(50):
        try:
            targets = json.load(urllib.request.urlopen(
                f"http://127.0.0.1:{PORT}/json"))
            break
        except Exception:
            time.sleep(0.2)
    page = next(t for t in targets if t["type"] == "page")
    ws = websocket.create_connection(page["webSocketDebuggerUrl"], timeout=60)
    cdp(ws, "Page.enable")

    for rel in DOCS:
        md = os.path.join(REPO, rel)
        html = md[:-3] + ".__pdf_tmp.html"
        pdf = md[:-3] + ".pdf"
        open(html, "w").write(md_to_html(md))
        cdp(ws, "Page.navigate", {"url": "file://" + html})
        while True:  # wait for load (images included)
            msg = json.loads(ws.recv())
            if msg.get("method") == "Page.loadEventFired":
                break
        time.sleep(0.5)
        r = cdp(ws, "Page.printToPDF", {
            "paperWidth": 8.27, "paperHeight": 11.69,       # A4 inches
            "marginTop": 0.59, "marginBottom": 0.39,        # 1.5 / 1 cm
            "marginLeft": 0.39, "marginRight": 0.39,        # 1 cm
            "printBackground": True, "displayHeaderFooter": False})
        open(pdf, "wb").write(base64.b64decode(r["data"]))
        os.remove(html)
        print(f"OK {pdf} ({os.path.getsize(pdf)} B)")
    ws.close()
finally:
    chrome.terminate()
print("ALL DONE")
