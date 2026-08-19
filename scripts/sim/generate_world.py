#!/usr/bin/env python3
"""Create a simulation world with one randomly heated Mission-3 box.

The IMAV rulebook defines the hot spot as 70-100 degC. The simulator uses
372.15 K (99 degC), matching the previous project while remaining in range.
"""
from __future__ import annotations

import argparse
import random
import xml.etree.ElementTree as ET
from pathlib import Path

BOXES = ("M3Box1", "M3Box2", "M3Box3")
HOT_TEMPERATURE_K = "372.15"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=None)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    chosen = rng.choice(BOXES)

    tree = ET.parse(args.input)
    root = tree.getroot()
    found = False

    for model in root.iter("model"):
        if model.get("name") != chosen:
            continue
        visual = model.find(".//visual")
        if visual is None:
            raise RuntimeError(f"{chosen} has no visual element")
        plugin = ET.Element(
            "plugin",
            {
                "filename": "gz-sim-thermal-system",
                "name": "gz::sim::systems::Thermal",
            },
        )
        temperature = ET.SubElement(plugin, "temperature")
        temperature.text = HOT_TEMPERATURE_K
        visual.append(plugin)
        found = True
        break

    if not found:
        raise RuntimeError(f"Could not find any Mission-3 box named {BOXES}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(tree, space="  ")
    tree.write(args.output, encoding="utf-8", xml_declaration=True)
    print(f"Mission-3 hot box: {chosen}; temperature: {HOT_TEMPERATURE_K} K")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
