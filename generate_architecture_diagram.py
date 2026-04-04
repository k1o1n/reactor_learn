#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from html import escape


CANVAS_WIDTH = 1680
CANVAS_HEIGHT = 980


@dataclass(frozen=True)
class Box:
    key: str
    x: int
    y: int
    width: int
    height: int
    title: str
    subtitle: str
    fill: str
    stroke: str
    text: str = "#13212b"
    radius: int = 18
    title_size: int = 24
    subtitle_size: int = 14


@dataclass(frozen=True)
class Group:
    x: int
    y: int
    width: int
    height: int
    title: str
    subtitle: str
    fill: str
    stroke: str


@dataclass(frozen=True)
class Arrow:
    points: tuple[tuple[int, int], ...]
    label: str = ""
    label_x: int = 0
    label_y: int = 0
    color: str = "#4c667c"
    dashed: bool = False


def render_group(group: Group) -> str:
    return f"""
  <g>
    <rect x="{group.x}" y="{group.y}" width="{group.width}" height="{group.height}" rx="28" fill="{group.fill}" stroke="{group.stroke}" stroke-width="2.5" stroke-dasharray="10 10" />
    <text x="{group.x + 24}" y="{group.y + 38}" font-size="26" font-weight="700" fill="#0e2433">{escape(group.title)}</text>
    <text x="{group.x + 24}" y="{group.y + 62}" font-size="13" fill="#496272">{escape(group.subtitle)}</text>
  </g>"""


def render_box(box: Box) -> str:
    title_x = box.x + 20
    title_y = box.y + 38
    title_lines = box.title.split("\n")
    subtitle_lines = box.subtitle.split("\n") if box.subtitle else []

    lines = [
        f'    <text x="{title_x}" y="{title_y + idx * 24}" font-size="{box.title_size}" font-weight="700" fill="{box.text}">{escape(line)}</text>'
        for idx, line in enumerate(title_lines)
    ]

    subtitle_start_y = title_y + max(len(title_lines), 1) * 24 + 2
    lines.extend(
        f'    <text x="{title_x}" y="{subtitle_start_y + idx * 18}" font-size="{box.subtitle_size}" fill="#3f5563">{escape(line)}</text>'
        for idx, line in enumerate(subtitle_lines)
    )

    return "\n".join([
        "",
        "  <g>",
        f'    <rect x="{box.x}" y="{box.y}" width="{box.width}" height="{box.height}" rx="{box.radius}" fill="{box.fill}" stroke="{box.stroke}" stroke-width="2.4" />',
        *lines,
        "  </g>",
    ])


def render_arrow_path(arrow: Arrow) -> str:
    points = " ".join(f"{x},{y}" for x, y in arrow.points)
    dash = ' stroke-dasharray="8 8"' if arrow.dashed else ""
    return f"""
  <polyline points="{points}" fill="none" stroke="{arrow.color}" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round" marker-end="url(#arrowhead)"{dash} />"""


def render_arrow_label(arrow: Arrow) -> str:
    if not arrow.label:
        return ""
    return f"""
    <text x="{arrow.label_x}" y="{arrow.label_y}" font-size="12" font-weight="600" text-anchor="middle" fill="#445d71">{escape(arrow.label)}</text>"""


def build_diagram() -> str:
    groups = [
        Group(60, 120, 430, 760, "Main Reactor Plane", "acceptor thread + base EventLoop", "#f8fbff", "#85a8c6"),
        Group(540, 120, 760, 760, "Sub Reactors Plane", "one loop per thread for connection I/O", "#fbfcf7", "#92b07f"),
        Group(1330, 120, 290, 760, "Shared Services", "cross-cutting runtime components", "#fffaf4", "#d8ac67"),
    ]

    boxes = [
        Box("client", 90, 190, 170, 86, "Clients", "requests / keepalive traffic", "#dff3ff", "#5fa8d3"),
        Box("acceptor", 285, 190, 175, 86, "Acceptor", "listen + accept new fds", "#ffe8c9", "#d39a4f"),
        Box("main_reactor", 155, 350, 270, 116, "MainReactor", "base EventLoop with epoll + eventfd", "#d8ecff", "#4e8ec6"),
        Box("tcp_server", 155, 535, 270, 116, "TcpServer", "owns acceptor thread and thread pool", "#e7f0ff", "#6c90c8"),
        Box("thread_pool", 610, 190, 300, 86, "EventLoopThreadPool", "round-robin dispatch to workers", "#e6f7df", "#77a95d"),
        Box("sub_reactors", 980, 190, 240, 86, "SubReactors", "multiple EventLoop threads", "#e3f2d9", "#7eab63"),
        Box("event_loop", 630, 370, 280, 116, "EventLoop", "epoll, wakeup, task queue", "#d8f0d0", "#6d9960"),
        Box("channel", 970, 370, 220, 116, "Channel", "fd event wrapper + callbacks", "#eef7dd", "#91ad5a"),
        Box("tcp_connection", 630, 570, 300, 116, "TcpConnection", "read/write buffer, close flow, OnMessage", "#dff3d7", "#6fa05e"),
        Box("timer_wheel", 970, 570, 220, 116, "TimerWheel", "timerfd-driven heartbeat buckets", "#f1f8d8", "#9eb34e"),
        Box("app_logic", 720, 742, 400, 108, "User Callback /\nProtocol Layer", "length-prefixed parsing\nand business logic", "#edf0ff", "#7e86cc", title_size=20, subtitle_size=13),
        Box("logger", 1370, 240, 220, 92, "Logger", "front-end logging macros", "#fff1db", "#d39a4f"),
        Box("async_logging", 1370, 430, 220, 112, "AsyncLogging", "double buffer + flush thread", "#ffe6c4", "#cc8b3f"),
        Box("log_file", 1370, 630, 220, 86, "Log File", "pid*.log / benchmark output", "#fff7e8", "#d4ab67"),
    ]

    arrows = [
        Arrow(((260, 233), (285, 233)), "connect", 272, 220),
        Arrow(((372, 276), (372, 320), (290, 320), (290, 350)), "accept", 340, 309),
        Arrow(((290, 466), (290, 535)), "own base loop", 348, 505),
        Arrow(((425, 593), (505, 593), (505, 233), (610, 233)), "round-robin", 533, 580),
        Arrow(((910, 233), (980, 233)), "worker loops", 944, 220),
        Arrow(((1100, 276), (1100, 332), (770, 332), (770, 370)), "one loop per thread", 1000, 324),
        Arrow(((910, 428), (970, 428)), "epoll events", 940, 415),
        Arrow(((1080, 486), (1080, 528), (780, 528), (780, 570)), "callbacks", 985, 520),
        Arrow(((930, 628), (970, 628)), "heartbeat refresh", 952, 616),
        Arrow(((1080, 570), (1080, 540), (840, 540), (840, 570)), "idle timeout", 952, 533, dashed=True),
        Arrow(((780, 686), (780, 728), (920, 728), (920, 742)), "protocol dispatch", 860, 720),
        Arrow(((770, 370), (770, 315), (1285, 315), (1285, 286), (1370, 286)), "runtime logs", 1280, 304),
        Arrow(((1480, 332), (1480, 430)), "append", 1518, 388),
        Arrow(((1480, 542), (1480, 630)), "flush", 1516, 592),
    ]

    parts = [
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        f"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"{CANVAS_WIDTH}\" height=\"{CANVAS_HEIGHT}\" viewBox=\"0 0 {CANVAS_WIDTH} {CANVAS_HEIGHT}\">",
        "  <defs>",
        "    <linearGradient id=\"bg\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">",
        "      <stop offset=\"0%\" stop-color=\"#f5f9fc\" />",
        "      <stop offset=\"100%\" stop-color=\"#fdf7ee\" />",
        "    </linearGradient>",
        "    <filter id=\"shadow\" x=\"-10%\" y=\"-10%\" width=\"130%\" height=\"130%\">",
        "      <feDropShadow dx=\"0\" dy=\"10\" stdDeviation=\"12\" flood-color=\"#708090\" flood-opacity=\"0.12\" />",
        "    </filter>",
        "    <marker id=\"arrowhead\" markerWidth=\"12\" markerHeight=\"12\" refX=\"10\" refY=\"6\" orient=\"auto\">",
        "      <path d=\"M 0 0 L 12 6 L 0 12 z\" fill=\"#4c667c\" />",
        "    </marker>",
        "  </defs>",
        "  <rect width=\"100%\" height=\"100%\" fill=\"url(#bg)\" />",
        "  <text x=\"60\" y=\"64\" font-size=\"34\" font-weight=\"800\" fill=\"#10202d\">reactor Architecture</text>",
        "  <text x=\"60\" y=\"95\" font-size=\"16\" fill=\"#4b6271\">C++17, epoll, eventfd, one loop per thread, timer wheel heartbeat, async logging</text>",
    ]

    parts.extend(render_group(group) for group in groups)
    parts.append("  <g>")
    parts.extend(render_arrow_path(arrow) for arrow in arrows)
    parts.append("  </g>")
    parts.append("  <g filter=\"url(#shadow)\">")
    parts.extend(render_box(box) for box in boxes)
    parts.append("  </g>")
    parts.append("  <g>")
    parts.extend(render_arrow_label(arrow) for arrow in arrows)
    parts.extend([
        "    <rect x=\"60\" y=\"902\" width=\"1560\" height=\"42\" rx=\"14\" fill=\"#ffffff\" opacity=\"0.92\" stroke=\"#d7e2e9\" />",
        "    <text x=\"84\" y=\"929\" font-size=\"13\" fill=\"#304958\">MainReactor accepts new connections and TcpServer dispatches sockets to SubReactors. Each worker EventLoop manages Channel and TcpConnection I/O. TimerWheel handles heartbeat expiry. Logger writes through AsyncLogging to disk.</text>",
        "  </g>",
        "</svg>",
    ])
    return "\n".join(parts)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate the reactor architecture SVG diagram.")
    parser.add_argument(
        "--output",
        default=os.path.join("docs", "architecture.svg"),
        help="Output SVG path. Default: docs/architecture.svg",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    svg = build_diagram()
    output_path = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as file:
        file.write(svg)
    print(f"generated {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())