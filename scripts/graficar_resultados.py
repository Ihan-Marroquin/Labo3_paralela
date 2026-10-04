from __future__ import annotations

import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "resultados"


def read_csv(name: str) -> list[dict[str, str]]:
    with (RESULTS / name).open(newline="", encoding="utf-8") as file:
        return list(csv.DictReader(file))


def summarize(
    rows: list[dict[str, str]],
    key: str,
    value: str,
    output_name: str,
) -> list[tuple[int, float, float]]:
    groups: dict[int, list[float]] = defaultdict(list)
    for row in rows:
        groups[int(row[key])].append(float(row[value]))

    summary = [
        (group, statistics.mean(values), statistics.stdev(values))
        for group, values in sorted(groups.items())
    ]
    with (RESULTS / output_name).open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow([key, f"promedio_{value}", f"desviacion_{value}"])
        writer.writerows(summary)
    return summary


def plot_series(
    summary: list[tuple[int, float, float]],
    title: str,
    x_label: str,
    y_label: str,
    output_name: str,
    color: str,
) -> None:
    x = [row[0] for row in summary]
    y = [row[1] for row in summary]
    error = [row[2] for row in summary]

    plt.figure(figsize=(9.2, 5.2), dpi=180)
    plt.errorbar(x, y, yerr=error, marker="o", linewidth=2.0, capsize=4, color=color)
    plt.xscale("log", base=2)
    plt.title(title)
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.grid(True, which="both", alpha=0.28)
    plt.tight_layout()
    plt.savefig(RESULTS / output_name, bbox_inches="tight")
    plt.close()


def main() -> None:
    RESULTS.mkdir(exist_ok=True)

    ping = summarize(
        read_csv("ping_pong_raw.csv"),
        "bytes",
        "tiempo_promedio_ms",
        "ping_pong_resumen.csv",
    )
    plot_series(
        ping,
        "Ping-Pong: tamaño del mensaje y latencia promedio",
        "Tamaño del mensaje (bytes, escala logarítmica)",
        "Tiempo promedio de ida y vuelta (ms)",
        "ping_pong_tiempo.png",
        "#1f77b4",
    )

    reception = summarize(
        read_csv("recepcion_anticipada_raw.csv"),
        "bytes",
        "tiempo_s",
        "recepcion_anticipada_resumen.csv",
    )
    plot_series(
        reception,
        "Recepción anticipada: tamaño del mensaje y tiempo",
        "Tamaño del mensaje (bytes, escala logarítmica)",
        "Tiempo hasta completar la recepción (s)",
        "recepcion_anticipada_tiempo.png",
        "#2ca02c",
    )

    pipeline = summarize(
        read_csv("pipeline_chunks_raw.csv"),
        "chunk_bytes",
        "tiempo_total_s",
        "pipeline_chunks_resumen.csv",
    )
    plot_series(
        pipeline,
        "Pipeline: tamaño del chunk y tiempo total",
        "Tamaño del chunk (bytes, escala logarítmica)",
        "Tiempo total (s)",
        "pipeline_chunks_tiempo.png",
        "#d97706",
    )


if __name__ == "__main__":
    main()
