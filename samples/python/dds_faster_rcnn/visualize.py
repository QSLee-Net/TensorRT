#
# SPDX-FileCopyrightText: Copyright (c) 1993-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import numpy as np

import PIL.Image as Image
import PIL.ImageDraw as ImageDraw
import PIL.ImageFont as ImageFont

COLORS = [
    "GoldenRod",
    "MediumTurquoise",
    "GreenYellow",
    "SteelBlue",
    "DarkSeaGreen",
    "SeaShell",
    "LightGrey",
    "IndianRed",
    "DarkKhaki",
    "LawnGreen",
    "WhiteSmoke",
    "Peru",
    "LightCoral",
    "FireBrick",
    "OldLace",
    "LightBlue",
    "SlateGray",
    "OliveDrab",
    "NavajoWhite",
    "PaleVioletRed",
    "SpringGreen",
    "AliceBlue",
    "Violet",
    "DeepSkyBlue",
    "Red",
    "MediumVioletRed",
    "PaleTurquoise",
    "Tomato",
    "Azure",
    "Yellow",
    "Cornsilk",
    "Aquamarine",
    "CadetBlue",
    "CornflowerBlue",
    "DodgerBlue",
    "Olive",
    "Orchid",
    "LemonChiffon",
    "Sienna",
    "OrangeRed",
    "Orange",
    "DarkSalmon",
    "Magenta",
    "Wheat",
    "Lime",
    "GhostWhite",
    "SlateBlue",
    "Aqua",
    "MediumAquaMarine",
    "LightSlateGrey",
    "MediumSeaGreen",
    "SandyBrown",
    "YellowGreen",
    "Plum",
    "FloralWhite",
    "LightPink",
    "Thistle",
    "DarkViolet",
    "Pink",
    "Crimson",
    "Chocolate",
    "DarkGrey",
    "Ivory",
    "PaleGreen",
    "DarkGoldenRod",
    "LavenderBlush",
    "SlateGrey",
    "DeepPink",
    "Gold",
    "Cyan",
    "LightSteelBlue",
    "MediumPurple",
    "ForestGreen",
    "DarkOrange",
    "Tan",
    "Salmon",
    "PaleGoldenRod",
    "LightGreen",
    "LightSlateGray",
    "HoneyDew",
    "Fuchsia",
    "LightSeaGreen",
    "DarkOrchid",
    "Green",
    "Chartreuse",
    "LimeGreen",
    "AntiqueWhite",
    "Beige",
    "Gainsboro",
    "Bisque",
    "SaddleBrown",
    "Silver",
    "Lavender",
    "Teal",
    "LightCyan",
    "PapayaWhip",
    "Purple",
    "Coral",
    "BurlyWood",
    "LightGray",
    "Snow",
    "MistyRose",
    "PowderBlue",
    "DarkCyan",
    "White",
    "Turquoise",
    "MediumSlateBlue",
    "PeachPuff",
    "Moccasin",
    "LightSalmon",
    "SkyBlue",
    "Khaki",
    "MediumSpringGreen",
    "BlueViolet",
    "MintCream",
    "Linen",
    "SeaGreen",
    "HotPink",
    "LightYellow",
    "BlanchedAlmond",
    "RoyalBlue",
    "RosyBrown",
    "MediumOrchid",
    "DarkTurquoise",
    "LightGoldenRodYellow",
    "LightSkyBlue",
]


def visualize_detections(image_path, output_path, detections, labels=[]):
    image = Image.open(image_path).convert(mode="RGB")
    draw = ImageDraw.Draw(image)
    line_width = 2
    font = ImageFont.load_default()
    for d in detections:
        color = COLORS[d["class"] % len(COLORS)]
        draw.line(
            [
                (d["xmin"], d["ymin"]),
                (d["xmin"], d["ymax"]),
                (d["xmax"], d["ymax"]),
                (d["xmax"], d["ymin"]),
                (d["xmin"], d["ymin"]),
            ],
            width=line_width,
            fill=color,
        )
        label = f"Class {d['class']}"
        if d["class"] < len(labels):
            label = f"{labels[d['class']]}"
        score = d["score"]
        text = f"{label}: {int(100*score)}%"
        if score < 0:
            text = label
        left, top, right, bottom = font.getbbox(text)
        text_width, text_height = right - left, bottom - top
        text_bottom = max(text_height, d["ymin"])
        text_left = d["xmin"]
        margin = np.ceil(0.05 * text_height)
        draw.rectangle(
            [
                (text_left, text_bottom - text_height - 2 * margin),
                (text_left + text_width, text_bottom),
            ],
            fill=color,
        )
        draw.text(
            (text_left + margin, text_bottom - text_height - margin),
            text,
            fill="black",
            font=font,
        )
    if output_path is None:
        return image
    image.save(output_path)
