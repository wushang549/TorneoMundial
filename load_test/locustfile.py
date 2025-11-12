#!/usr/bin/env python3
import time
import random
import uuid
import requests
from typing import Optional, Dict, List, Tuple

# ------------------ Config ------------------
BASE_URL = "http://host.containers.internal:8081"

TOURNAMENTS_ENDPOINT   = "/tournaments"
GROUPS_ENDPOINT_FMT    = "/tournaments/{tid}/groups"
ADD_TEAM_TO_GROUP_FMT  = "/tournaments/{tid}/groups/{gid}/teams/{teamId}"
TEAMS_ENDPOINT         = "/teams"

MATCHES_ENDPOINT_FMT   = "/tournaments/{tid}/matches"
MATCH_PATCH_FMT        = "/tournaments/{tid}/matches/{mid}"  # PATCH with {"score":{...}}

# Make name unique to avoid 409 conflict
TOURNAMENT_NAME = f"World Cup 2025 - {uuid.uuid4()}"
TOURNAMENT_TYPE = "WORLD_CUP"  # must match your backend

TEAMS: List[Dict] = [
    {"name": "Brazil", "country": "Brazil", "shortName": "BRA", "foundedYear": 1914, "coach": "Coach Brazil"},
    {"name": "Argentina", "country": "Argentina", "shortName": "ARG", "foundedYear": 1893, "coach": "Coach Argentina"},
    {"name": "France", "country": "France", "shortName": "FRA", "foundedYear": 1919, "coach": "Coach France"},
    {"name": "Germany", "country": "Germany", "shortName": "GER", "foundedYear": 1900, "coach": "Coach Germany"},
    {"name": "Spain", "country": "Spain", "shortName": "ESP", "foundedYear": 1913, "coach": "Coach Spain"},
    {"name": "England", "country": "England", "shortName": "ENG", "foundedYear": 1863, "coach": "Coach England"},
    {"name": "Portugal", "country": "Portugal", "shortName": "POR", "foundedYear": 1914, "coach": "Coach Portugal"},
    {"name": "Netherlands", "country": "Netherlands", "shortName": "NED", "foundedYear": 1889, "coach": "Coach Netherlands"},
    {"name": "Italy", "country": "Italy", "shortName": "ITA", "foundedYear": 1898, "coach": "Coach Italy"},
    {"name": "Belgium", "country": "Belgium", "shortName": "BEL", "foundedYear": 1895, "coach": "Coach Belgium"},
    {"name": "Uruguay", "country": "Uruguay", "shortName": "URU", "foundedYear": 1900, "coach": "Coach Uruguay"},
    {"name": "Croatia", "country": "Croatia", "shortName": "CRO", "foundedYear": 1912, "coach": "Coach Croatia"},
    {"name": "Mexico", "country": "Mexico", "shortName": "MEX", "foundedYear": 1927, "coach": "Coach Mexico"},
    {"name": "United States", "country": "United States", "shortName": "USA", "foundedYear": 1913, "coach": "Coach USA"},
    {"name": "Japan", "country": "Japan", "shortName": "JPN", "foundedYear": 1921, "coach": "Coach Japan"},
    {"name": "South Korea", "country": "South Korea", "shortName": "KOR", "foundedYear": 1928, "coach": "Coach Korea"},
    {"name": "Denmark", "country": "Denmark", "shortName": "DEN", "foundedYear": 1889, "coach": "Coach Denmark"},
    {"name": "Switzerland", "country": "Switzerland", "shortName": "SUI", "foundedYear": 1895, "coach": "Coach Switzerland"},
    {"name": "Colombia", "country": "Colombia", "shortName": "COL", "foundedYear": 1924, "coach": "Coach Colombia"},
    {"name": "Chile", "country": "Chile", "shortName": "CHI", "foundedYear": 1895, "coach": "Coach Chile"},
    {"name": "Sweden", "country": "Sweden", "shortName": "SWE", "foundedYear": 1904, "coach": "Coach Sweden"},
    {"name": "Norway", "country": "Norway", "shortName": "NOR", "foundedYear": 1902, "coach": "Coach Norway"},
    {"name": "Poland", "country": "Poland", "shortName": "POL", "foundedYear": 1919, "coach": "Coach Poland"},
    {"name": "Austria", "country": "Austria", "shortName": "AUT", "foundedYear": 1904, "coach": "Coach Austria"},
    {"name": "Serbia", "country": "Serbia", "shortName": "SRB", "foundedYear": 1919, "coach": "Coach Serbia"},
    {"name": "Morocco", "country": "Morocco", "shortName": "MAR", "foundedYear": 1956, "coach": "Coach Morocco"},
    {"name": "Nigeria", "country": "Nigeria", "shortName": "NGA", "foundedYear": 1923, "coach": "Coach Nigeria"},
    {"name": "Senegal", "country": "Senegal", "shortName": "SEN", "foundedYear": 1960, "coach": "Coach Senegal"},
    {"name": "Australia", "country": "Australia", "shortName": "AUS", "foundedYear": 1961, "coach": "Coach Australia"},
    {"name": "Ecuador", "country": "Ecuador", "shortName": "ECU", "foundedYear": 1925, "coach": "Coach Ecuador"},
    {"name": "Peru", "country": "Peru", "shortName": "PER", "foundedYear": 1922, "coach": "Coach Peru"},
    {"name": "Canada", "country": "Canada", "shortName": "CAN", "foundedYear": 1912, "coach": "Coach Canada"},
]

# ------------------ HTTP helpers ------------------
def http_post(path: str, json_body: dict | None = None) -> Tuple[int, dict | str]:
    url = f"{BASE_URL}{path}"
    try:
        r = requests.post(url, json=json_body, timeout=10)
    except requests.RequestException as exc:
        return 0, f"Request error: {exc}"
    try:
        data = r.json()
    except ValueError:
        data = r.text
    return r.status_code, data

def http_get(path: str, params: dict | None = None) -> Tuple[int, dict | str]:
    url = f"{BASE_URL}{path}"
    try:
        r = requests.get(url, params=params, timeout=10)
    except requests.RequestException as exc:
        return 0, f"Request error: {exc}"
    try:
        data = r.json()
    except ValueError:
        data = r.text
    return r.status_code, data

def http_patch(path: str, json_body: dict) -> Tuple[int, dict | str]:
    url = f"{BASE_URL}{path}"
    try:
        r = requests.patch(url, json=json_body, timeout=10)
    except requests.RequestException as exc:
        return 0, f"Request error: {exc}"
    try:
        data = r.json()
    except ValueError:
        data = r.text
    return r.status_code, data

# ------------------ Domain helpers ------------------
def create_world_cup() -> Optional[str]:
    payload = {
        "name": TOURNAMENT_NAME,
        "format": {
            "numberOfGroups": 8,
            "maxTeamsPerGroup": 4,
            "type": TOURNAMENT_TYPE,
        },
    }
    status, data = http_post(TOURNAMENTS_ENDPOINT, payload)
    if status == 201 and isinstance(data, dict):
        tid = data.get("id")
        print(f"[OK] Tournament created: {TOURNAMENT_NAME} id={tid}")
        return tid
    print(f"[FAIL] Tournament create failed. status={status} body={data}")
    return None

def create_groups(tid: str) -> List[dict]:
    created: List[dict] = []
    for i in range(1, 9):
        name = f"grupo{i}"
        status, data = http_post(GROUPS_ENDPOINT_FMT.format(tid=tid), {"name": name})
        if status == 201 and isinstance(data, dict):
            gid = data.get("id")
            print(f"[OK] Group created: {name} id={gid}")
            created.append({"name": name, "id": gid})
        else:
            print(f"[FAIL] Group {name} create failed. status={status} body={data}")
    return created

def create_team(team: dict) -> Optional[str]:
    payload = {
        "name": team["name"],
        "country": team["country"],
        "shortName": team["shortName"],
        "foundedYear": team["foundedYear"],
        "coach": team["coach"],
    }
    status, data = http_post(TEAMS_ENDPOINT, payload)
    if status == 201 and isinstance(data, dict):
        team_id = data.get("id")
        print(f"[OK] Team created: {team['name']} id={team_id}")
        return team_id
    print(f"[FAIL] Team create failed: {team['name']} status={status} body={data}")
    return None

def add_team_to_group(tournament_id: str, group_id: str, team_id: str) -> bool:
    path = ADD_TEAM_TO_GROUP_FMT.format(tid=tournament_id, gid=group_id, teamId=team_id)
    status, data = http_post(path, None)
    if status in (200, 204):
        print(f"[OK] Added team {team_id} to group {group_id}")
        return True
    print(f"[FAIL] Add team to group failed. status={status} body={data}")
    return False

def chunk(lst: List[str], size: int) -> List[List[str]]:
    return [lst[i:i + size] for i in range(0, len(lst), size)]

# ------------------ Matches / Scores ------------------
def list_matches(tid: str, show: str | None = None) -> List[dict]:
    params = {"showMatches": show} if show else None
    status, data = http_get(MATCHES_ENDPOINT_FMT.format(tid=tid), params)
    if status == 200 and isinstance(data, list):
        if show:
            # Extra client-side filter to be strict
            if show == "pending":
                data = [m for m in data if m.get("status") == "pending"]
            elif show == "played":
                data = [m for m in data if m.get("status") == "played"]
        return data
    print(f"[FAIL] List matches failed. status={status} body={data}")
    return []

def wait_for_group_matches(tid: str, timeout_sec: int = 15) -> int:
    # Poll until consumer creates group matches
    start = time.time()
    last_seen = 0
    while time.time() - start < timeout_sec:
        matches = list_matches(tid, show="pending")
        group_pending = [m for m in matches if m.get("round") == "group" and m.get("status") == "pending"]
        last_seen = len(group_pending)
        if last_seen > 0:
            return last_seen
        time.sleep(1)
    return last_seen

def update_score(tid: str, mid: str, home: int, visitor: int) -> bool:
    payload = {"score": {"home": home, "visitor": visitor}}
    url = MATCH_PATCH_FMT.format(tid=tid, mid=mid)
    print(f"[DEBUG] PATCH {url} body={payload}")
    s, d = http_patch(url, payload)
    print(f"[DEBUG] -> status={s} body={d}")
    return s in (200, 204)

def play_group_stage(tid: str) -> None:
    matches = list_matches(tid, show="pending")
    matches = [m for m in matches if m.get("round") == "group" and m.get("status") == "pending"]
    total = len(matches)
    print(f"[INFO] Pending group matches: {total}")
    if total == 0:
        return

    played = 0
    for m in matches:
        mid = m["id"]
        home = random.randint(0, 4)
        visitor = random.randint(0, 4)
        if home == visitor:
            visitor = (visitor + 1) % 5
        ok = update_score(tid, mid, home, visitor)
        if ok:
            played += 1
        else:
            print(f"[WARN] Could not play match {mid}")
    print(f"[INFO] Group-stage played: {played}/{total}")

# ------------------ Knockout helpers ------------------
def wait_for_round(tid: str, round_name: str, timeout_sec: int = 60) -> List[dict]:
    start = time.time()
    last: List[dict] = []
    while time.time() - start < timeout_sec:
        ms = list_matches(tid)  # all matches
        round_ms = [m for m in ms if m.get("round") == round_name]
        if round_ms:
            return round_ms
        last = round_ms
        time.sleep(1)
    return last

def play_knockout_round(tid: str, round_name: str) -> None:
    ms = list_matches(tid, show="pending")
    ms = [m for m in ms if m.get("round") == round_name and m.get("status") == "pending"]
    print(f"[INFO] Pending {round_name} matches: {len(ms)}")
    for m in ms:
        mid = m["id"]
        # Avoid draws
        home = random.randint(0, 4)
        visitor = random.randint(0, 4)
        if home == visitor:
            visitor = (visitor + 1) % 5
        ok = update_score(tid, mid, home, visitor)
        if not ok:
            print(f"[WARN] Could not play KO match {mid}")

    # Wait until all matches in this round are not pending
    start = time.time()
    while time.time() - start < 60:
        left = [m for m in list_matches(tid, show="pending") if m.get("round") == round_name]
        if not left:
            break
        time.sleep(1)

def get_champion(tid: str) -> Optional[dict]:
    finals = [m for m in list_matches(tid) if m.get("round") == "final"]
    if not finals:
        return None
    final = finals[0]
    if final.get("status") != "played":
        return None

    wtid = final.get("winnerTeamId")
    home = final.get("home", {})
    visitor = final.get("visitor", {})
    if wtid:
        if home.get("id") == wtid:
            return {"teamId": wtid, "name": home.get("name"), "side": "home"}
        if visitor.get("id") == wtid:
            return {"teamId": wtid, "name": visitor.get("name"), "side": "visitor"}

    score = final.get("score", {})
    hs, vs = score.get("home"), score.get("visitor")
    if isinstance(hs, int) and isinstance(vs, int):
        if hs > vs:
            return {"teamId": home.get("id"), "name": home.get("name"), "side": "home"}
        elif vs > hs:
            return {"teamId": visitor.get("id"), "name": visitor.get("name"), "side": "visitor"}
    return None

# ------------------ Orchestration ------------------
def main() -> None:
    # 1) Create tournament
    tournament_id = create_world_cup()
    if not tournament_id:
        print("[STOP] No tournament id. Exiting.")
        return

    # 2) Create 8 groups
    groups = create_groups(tournament_id)
    groups = [g for g in groups if g.get("id")]
    if len(groups) < 8:
        print(f"[WARN] Only {len(groups)} groups created. Proceeding with those.")

    # 3) Create 32 teams
    team_ids: List[str] = []
    for t in TEAMS:
        tid = create_team(t)
        if tid:
            team_ids.append(tid)
    if len(team_ids) < 32:
        print(f"[WARN] Only {len(team_ids)} teams created. Proceeding with those.")

    # 4) Assign teams to groups in chunks of 4
    team_chunks = chunk(team_ids, 4)
    for idx, teams_chunk in enumerate(team_chunks):
        if idx >= len(groups):
            print("[INFO] No more groups available for remaining teams.")
            break
        gid = groups[idx]["id"]
        for team_id in teams_chunk:
            add_team_to_group(tournament_id, gid, team_id)

    print("[INFO] Seeding complete. Waiting for group matches to appear...")
    pending = wait_for_group_matches(tournament_id, timeout_sec=15)
    print(f"[INFO] Pending group matches detected: {pending}")

    print("[INFO] Now playing group stage...")
    play_group_stage(tournament_id)
    time.sleep(1)
# 5) Knockouts (use lowercase round names)
    print("[INFO] Waiting for Round of 16 (r16)...")
    wait_for_round(tournament_id, "r16", timeout_sec=60)
    play_knockout_round(tournament_id, "r16")

    print("[INFO] Waiting for Quarter-finals (qf)...")
    wait_for_round(tournament_id, "qf", timeout_sec=60)
    play_knockout_round(tournament_id, "qf")

    print("[INFO] Waiting for Semi-finals (sf)...")
    wait_for_round(tournament_id, "sf", timeout_sec=60)
    play_knockout_round(tournament_id, "sf")

    print("[INFO] Waiting for Final (final)...")
    wait_for_round(tournament_id, "final", timeout_sec=60)
    play_knockout_round(tournament_id, "final")

    champ = get_champion(tournament_id)
    if champ:
        print(f"[RESULT] Champion: {champ['name']} (teamId={champ['teamId']}) \U0001F3C6")
    else:
        print("[RESULT] Champion not found yet. Check consumers/logs.")

if __name__ == "__main__":
    main()
