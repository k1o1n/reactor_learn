#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from html import escape


CANVAS_WIDTH = 1760
CANVAS_HEIGHT = 1040


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
        Group(60, 130, 460, 800, "Main Reactor Plane", "acceptor thread + base EventLoop", "#f8fbff", "#85a8c6"),
        Group(560, 130, 820, 800, "Sub Reactors Plane", "owner-loop initialization and connection I/O", "#fbfcf7", "#92b07f"),
        Group(1410, 130, 290, 800, "Shared Services", "cross-cutting runtime components", "#fffaf4", "#d8ac67"),
    ]

    boxes = [
        Box("client", 90, 210, 180, 88, "Clients", "requests / keepalive traffic", "#dff3ff", "#5fa8d3"),
        Box("acceptor", 300, 210, 190, 88, "Acceptor", "accept loop drains backlog", "#ffe8c9", "#d39a4f"),
        Box("main_reactor", 160, 385, 300, 120, "MainReactor", "base EventLoop with epoll + eventfd", "#d8ecff", "#4e8ec6"),
        Box("tcp_server", 160, 585, 300, 124, "TcpServer", "baseloop registry + dispatch policy", "#e7f0ff", "#6c90c8"),
        Box("thread_pool", 610, 210, 320, 88, "EventLoopThreadPool", "round-robin dispatch to workers", "#e6f7df", "#77a95d"),
        Box("sub_reactors", 1020, 210, 260, 88, "SubReactors", "multiple EventLoop threads", "#e3f2d9", "#7eab63"),
        Box("event_loop", 640, 390, 310, 124, "EventLoop", "epoll, wakeup, owner-thread tasks", "#d8f0d0", "#6d9960"),
        Box("channel", 1030, 390, 230, 124, "Channel", "fd event wrapper + callbacks", "#eef7dd", "#91ad5a"),
        Box("tcp_connection", 640, 605, 330, 124, "TcpConnection", "owner-loop init, read/write, close flow", "#dff3d7", "#6fa05e"),
        Box("timer_wheel", 1030, 605, 230, 124, "TimerWheel", "timerfd + heartbeat buckets", "#f1f8d8", "#9eb34e"),
        Box("app_logic", 720, 800, 460, 96, "User Callback /\nProtocol Layer", "length-prefixed parsing and business logic", "#edf0ff", "#7e86cc", title_size=20, subtitle_size=13),
        Box("logger", 1440, 250, 220, 96, "Logger", "front-end logging macros", "#fff1db", "#d39a4f"),
        Box("async_logging", 1440, 455, 220, 122, "AsyncLogging", "buffer queue + dedicated flush thread", "#ffe6c4", "#cc8b3f"),
        Box("log_file", 1440, 675, 220, 88, "Log File", "pid*.log / benchmark output", "#fff7e8", "#d4ab67"),
    ]

    arrows = [
        Arrow(((270, 254), (300, 254)), "connect", 284, 240),
        Arrow(((395, 298), (395, 346), (310, 346), (310, 385)), "drain accept", 370, 334),
        Arrow(((310, 505), (310, 585)), "base loop", 366, 552),
        Arrow(((460, 646), (535, 646), (535, 254), (610, 254)), "choose io loop", 546, 630),
        Arrow(((930, 254), (1020, 254)), "worker loops", 975, 240),
        Arrow(((1150, 298), (1150, 352), (795, 352), (795, 390)), "owner-loop init", 1052, 344),
        Arrow(((950, 452), (1030, 452)), "epoll events", 990, 438),
        Arrow(((1145, 514), (1145, 566), (810, 566), (810, 605)), "callbacks", 980, 548),
        Arrow(((970, 665), (1030, 665)), "heartbeat refresh", 1000, 646),
        Arrow(((1145, 605), (1145, 570), (875, 570), (875, 605)), "idle timeout", 1080, 556, dashed=True),
        Arrow(((820, 729), (820, 776), (950, 776), (950, 800)), "protocol dispatch", 888, 768),
        Arrow(((795, 390), (795, 330), (1375, 330), (1375, 298), (1440, 298)), "runtime logs", 1355, 318),
        Arrow(((1550, 346), (1550, 455)), "append", 1588, 408),
        Arrow(((1550, 577), (1550, 675)), "flush", 1582, 632),
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
        "  <text x=\"60\" y=\"66\" font-size=\"34\" font-weight=\"800\" fill=\"#10202d\">reactor Architecture</text>",
        "  <text x=\"60\" y=\"98\" font-size=\"16\" fill=\"#4b6271\">C++17, epoll, eventfd, one loop per thread, timer wheel heartbeat, async logging</text>",
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
        "    <text x=\"60\" y=\"972\" font-size=\"13\" fill=\"#304958\">MainReactor drains the accept backlog, TcpServer dispatches sockets to SubReactors, and each owner EventLoop initializes TcpConnection, heartbeat state, and protocol callbacks before I/O is activated.</text>",
        "    <text x=\"60\" y=\"994\" font-size=\"13\" fill=\"#304958\">Logger appends through AsyncLogging, which keeps buffer ownership separate from backend wakeups and flushes batched output to disk.</text>",
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