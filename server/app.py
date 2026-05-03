from flask import Flask, request, jsonify, send_from_directory
import json
import os
import sys
import random

app = Flask(__name__)

# When bundled with PyInstaller, resolve paths relative to the exe location
# In dev, resolve relative to this script's directory
if getattr(sys, 'frozen', False):
    _BASE_DIR = os.path.dirname(sys.executable)
else:
    _BASE_DIR = os.path.dirname(os.path.abspath(__file__))

CONFIG_PATH = os.path.join(_BASE_DIR, "config.json")


def load_config():
    with open(CONFIG_PATH, "r") as f:
        return json.load(f)


def save_config(data):
    with open(CONFIG_PATH, "w") as f:
        json.dump(data, f, indent=2)


# ─── Config editor ──────────────────────────────────────────────────

@app.route("/", methods=["GET"])
def editor_page():
    return send_from_directory(_BASE_DIR, "editor.html")


@app.route("/api/config", methods=["GET"])
def get_config():
    return jsonify(load_config())


@app.route("/api/config", methods=["POST"])
def save_config_endpoint():
    data = request.get_json(force=True)
    if not isinstance(data, dict):
        return jsonify({"error": "Invalid config"}), 400
    for key in ("monsters", "heroes", "xp_rewards"):
        if key not in data:
            return jsonify({"error": f"Missing key: {key}"}), 400
    save_config(data)
    return jsonify({"ok": True})


# ─── Game endpoints ──────────────────────────────────────────────────

@app.route("/run", methods=["GET"])
def get_run():
    cfg = load_config()
    hero_id = int(request.args.get("hero_id", 0))
    hero_id = max(0, min(hero_id, len(cfg["heroes"]) - 1))
    return jsonify({
        "monsters": cfg["monsters"],
        "heroes": cfg["heroes"],
        "hero": cfg["heroes"][hero_id],
        "xp_rewards": cfg["xp_rewards"],
    })


@app.route("/move", methods=["GET"])
def get_move():
    cfg = load_config()
    monster_id = int(request.args.get("monster_id", 0))
    monster_hp = int(request.args.get("monster_hp", 100))
    monster_max_hp = int(request.args.get("monster_max_hp", 100))
    hero_hp = int(request.args.get("hero_hp", 100))

    monster = cfg["monsters"][monster_id]
    moves = monster["moves"]
    num_moves = len(moves)

    weights = [1.0] * num_moves
    hp_ratio = monster_hp / max(1, monster_max_hp)

    for i, move in enumerate(moves):
        effect = move["effect"]

        if effect in ("heal", "damage_heal") and hp_ratio < 0.4:
            weights[i] += 3.0
        if effect == "buff" and hp_ratio > 0.6:
            weights[i] += 2.0
        if effect in ("debuff", "damage_debuff"):
            weights[i] += 1.0
        if effect == "damage" and move.get("base_value", 0) >= 20 and hero_hp < 40:
            weights[i] += 2.5
        if effect == "buff_self_damage" and hp_ratio < 0.3:
            weights[i] = 0.1

    total = sum(weights)
    r = random.uniform(0, total)
    cumulative = 0
    chosen = 0
    for i, w in enumerate(weights):
        cumulative += w
        if r <= cumulative:
            chosen = i
            break

    return jsonify({"move_index": chosen})


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=False)
