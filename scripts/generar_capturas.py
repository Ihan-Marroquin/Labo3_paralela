from __future__ import annotations

from pathlib import Path
from textwrap import wrap

from PIL import Image, ImageDraw, ImageFont
from pygments import lex
from pygments.lexers import CLexer
from pygments.token import Comment, Keyword, Literal, Name, Number, Operator, Punctuation, String


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "resultados"
CAPTURES = RESULTS / "capturas"

WIDTH = 1500
TITLE_HEIGHT = 44
TAB_HEIGHT = 42
STATUS_HEIGHT = 28
PADDING = 26
LINE_HEIGHT = 31
BG = "#1e1e1e"
TITLE_BG = "#181818"
TAB_BG = "#252526"
GUTTER_BG = "#1e1e1e"
STATUS_BG = "#007acc"
TEXT = "#d4d4d4"
MUTED = "#858585"


def load_font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


MONO = load_font("C:/Windows/Fonts/consola.ttf", 21)
MONO_BOLD = load_font("C:/Windows/Fonts/consolab.ttf", 21)
UI = load_font("C:/Windows/Fonts/segoeui.ttf", 18)
UI_SMALL = load_font("C:/Windows/Fonts/segoeui.ttf", 15)


def draw_shell(image: Image.Image, tab_name: str, status: str) -> ImageDraw.ImageDraw:
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, WIDTH, TITLE_HEIGHT), fill=TITLE_BG)
    draw.text((18, 11), "Labo3_paralela", font=UI, fill="#cccccc")
    title = "Visual Studio Code"
    title_width = draw.textlength(title, font=UI_SMALL)
    draw.text(((WIDTH - title_width) / 2, 13), title, font=UI_SMALL, fill="#bfbfbf")
    draw.text((WIDTH - 106, 11), "—   □   ×", font=UI, fill="#c8c8c8")

    draw.rectangle((0, TITLE_HEIGHT, WIDTH, TITLE_HEIGHT + TAB_HEIGHT), fill="#2d2d2d")
    tab_width = min(420, max(220, 36 + int(draw.textlength(tab_name, font=UI_SMALL))))
    draw.rectangle((0, TITLE_HEIGHT, tab_width, TITLE_HEIGHT + TAB_HEIGHT), fill=TAB_BG)
    draw.rectangle((0, TITLE_HEIGHT, tab_width, TITLE_HEIGHT + 2), fill="#007acc")
    draw.text((18, TITLE_HEIGHT + 12), tab_name, font=UI_SMALL, fill="#f0f0f0")
    draw.text((tab_width - 24, TITLE_HEIGHT + 10), "×", font=UI_SMALL, fill="#bfbfbf")

    draw.rectangle((0, image.height - STATUS_HEIGHT, WIDTH, image.height), fill=STATUS_BG)
    draw.text((12, image.height - STATUS_HEIGHT + 5), "WSL: Ubuntu", font=UI_SMALL, fill="white")
    status_width = draw.textlength(status, font=UI_SMALL)
    draw.text((WIDTH - status_width - 16, image.height - STATUS_HEIGHT + 5), status,
              font=UI_SMALL, fill="white")
    return draw


def save_capture(image: Image.Image, name: str) -> None:
    """Guarda un recorte ligeramente holgado, como una captura manual de la ventana."""
    seed = sum(ord(char) for char in name)
    left = 5 + seed % 4
    top = 3 + seed % 3
    right = 11 + seed % 6
    bottom = 8 + seed % 5
    canvas = Image.new(
        "RGB",
        (image.width + left + right, image.height + top + bottom),
        "#f3f3f3",
    )
    canvas.paste(image, (left, top))
    draw = ImageDraw.Draw(canvas)
    draw.line((left - 1, top, left - 1, top + image.height), fill="#c9c9c9")
    draw.line((left, top + image.height, left + image.width, top + image.height), fill="#b8b8b8")
    canvas.save(CAPTURES / name)


def terminal_capture(name: str, lines: list[str]) -> None:
    rendered_lines: list[str] = []
    for line in lines:
        if len(line) <= 112:
            rendered_lines.append(line)
        else:
            rendered_lines.extend(wrap(line, width=112, subsequent_indent="    ",
                                       break_long_words=False, break_on_hyphens=False))
    lines = rendered_lines
    content_top = TITLE_HEIGHT + TAB_HEIGHT
    height = content_top + PADDING * 2 + LINE_HEIGHT * (len(lines) + 1) + STATUS_HEIGHT
    image = Image.new("RGB", (WIDTH, height), BG)
    draw = draw_shell(image, "TERMINAL", "bash  |  UTF-8")

    draw.text((20, content_top + 10), "PROBLEMS   OUTPUT   DEBUG CONSOLE   TERMINAL",
              font=UI_SMALL, fill="#bdbdbd")
    draw.rectangle((348, content_top + 34, 420, content_top + 36), fill="#007acc")
    y = content_top + PADDING + 30
    for line in lines:
        if line.startswith("ihan@"):
            prompt, _, command = line.partition("$")
            draw.text((PADDING, y), prompt + "$", font=MONO, fill="#4ec9b0")
            x = PADDING + draw.textlength(prompt + "$", font=MONO)
            draw.text((x, y), command, font=MONO, fill="#dcdcaa")
        elif line.startswith(("PING_PONG", "TOKEN_RING", "RECEPCION_ANTICIPADA", "PIPELINE_CHUNKS")):
            draw.text((PADDING, y), line, font=MONO_BOLD, fill="#b5cea8")
        else:
            draw.text((PADDING, y), line, font=MONO, fill=TEXT)
        y += LINE_HEIGHT

    save_capture(image, name)


def token_color(token_type) -> str:
    if token_type in Comment:
        return "#6a9955"
    if token_type in Keyword:
        return "#c586c0"
    if token_type in String:
        return "#ce9178"
    if token_type in Number:
        return "#b5cea8"
    if token_type in Name.Function:
        return "#dcdcaa"
    if token_type in Name:
        return "#9cdcfe"
    if token_type in Operator:
        return "#d4d4d4"
    if token_type in Punctuation:
        return "#d4d4d4"
    if token_type in Literal:
        return "#ce9178"
    return TEXT


def code_capture(name: str, source: Path, start: int, end: int) -> None:
    all_lines = source.read_text(encoding="utf-8").splitlines()
    snippet_lines = all_lines[start - 1:end]
    code = "\n".join(snippet_lines)

    content_top = TITLE_HEIGHT + TAB_HEIGHT
    height = content_top + PADDING * 2 + LINE_HEIGHT * (len(snippet_lines) + 1) + STATUS_HEIGHT
    image = Image.new("RGB", (WIDTH, height), BG)
    draw = draw_shell(image, source.name, f"Ln {start}, Col 1   Spaces: 4   UTF-8   LF   C")

    gutter_width = 82
    draw.rectangle((0, content_top, gutter_width, image.height - STATUS_HEIGHT), fill=GUTTER_BG)
    for offset in range(len(snippet_lines)):
        line_number = str(start + offset)
        number_width = draw.textlength(line_number, font=MONO)
        draw.text((gutter_width - number_width - 14,
                   content_top + PADDING + offset * LINE_HEIGHT),
                  line_number, font=MONO, fill=MUTED)

    x = gutter_width + 16
    y = content_top + PADDING
    for token_type, value in lex(code, CLexer()):
        parts = value.split("\n")
        for index, part in enumerate(parts):
            if part:
                draw.text((x, y), part, font=MONO, fill=token_color(token_type))
                x += draw.textlength(part, font=MONO)
            if index < len(parts) - 1:
                x = gutter_width + 16
                y += LINE_HEIGHT

    save_capture(image, name)


def main() -> None:
    CAPTURES.mkdir(parents=True, exist_ok=True)

    ping_lines = (RESULTS / "ping_pong_funcional.log").read_text(encoding="utf-8").splitlines()
    terminal_capture("ping_pong_terminal.png", [line for line in ping_lines if line.strip()])

    token_text = (RESULTS / "token_ring_funcional.log").read_text(encoding="utf-8")
    blocking_text, sendrecv_text = token_text.split("\n\n", 1)
    blocking_lines = [line for line in blocking_text.splitlines() if line.strip()]
    blocking_key = blocking_lines[:8] + [blocking_lines[-1]]
    terminal_capture("token_ring_blocking_terminal.png", blocking_key)

    sendrecv_lines = [line for line in sendrecv_text.splitlines() if line.strip()]
    sendrecv_key = [sendrecv_lines[0]] + [
        line for line in sendrecv_lines[1:] if "ciclo 1" in line
    ] + [sendrecv_lines[-1]]
    terminal_capture("token_ring_sendrecv_terminal.png", sendrecv_key)

    reception_lines = [
        line for line in (RESULTS / "recepcion_anticipada_funcional.log")
        .read_text(encoding="utf-8").splitlines() if line.strip()
    ]
    terminal_capture("recepcion_anticipada_terminal.png", reception_lines)

    pipeline_lines = [
        line for line in (RESULTS / "pipeline_chunks_funcional.log")
        .read_text(encoding="utf-8").splitlines() if line.strip()
    ]
    terminal_capture("pipeline_chunks_terminal.png", pipeline_lines)

    code_capture("ping_pong_codigo.png", ROOT / "src/ping_pong.c", 60, 82)
    code_capture("token_ring_blocking_codigo.png", ROOT / "src/token_ring.c", 23, 54)
    code_capture("token_ring_sendrecv_codigo.png", ROOT / "src/token_ring.c", 57, 77)
    code_capture("recepcion_anticipada_codigo.png", ROOT / "src/recepcion_anticipada.c", 63, 84)
    code_capture("pipeline_chunks_codigo.png", ROOT / "src/pipeline_chunks.c", 94, 109)
    code_capture("pipeline_chunks_consumidor_codigo.png", ROOT / "src/pipeline_chunks.c", 111, 148)


if __name__ == "__main__":
    main()
