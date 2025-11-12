#!/usr/bin/env python3
import requests
from typing import Optional, Dict, List, Tuple

BASE_URL = "http://host.containers.internal:8081"
TOURNAMENTS_ENDPOINT = "/tournaments"
GROUPS_ENDPOINT_FMT = "/tournaments/{tid}/groups"
ADD_TEAM_TO_GROUP_FMT = "/tournaments/{tid}/groups/{gid}/teams/{teamId}"
TEAMS_ENDPOINT = "/teams"

TOURNAMENT_NAME = "World Cup 2025"
TOURNAMENT_TYPE = "WORLD_CUP"  # matches your backend format.type

TEAMS: List[Dict] = [
    {"name": "Brazil",         "country": "Brazil",        "shortName": "BRA", "foundedYear": 1914, "coach": "Coach Brazil"},
    {"name": "Argentina",      "country": "Argentina",     "shortName": "ARG", "foundedYear": 1893, "coach": "Coach Argentina"},
    {"name": "France",         "country": "France",        "shortName": "FRA", "foundedYear": 1919, "coach": "Coach France"},
    {"name": "Germany",        "country": "Germany",       "shortName": "GER", "foundedYear": 1900, "coach": "Coach Germany"},
    {"name": "Spain",          "country": "Spain",         "shortName": "ESP", "foundedYear": 1913, "coach": "Coach Spain"},
    {"name": "England",        "country": "England",       "shortName": "ENG", "foundedYear": 1863, "coach": "Coach England"},
    {"name": "Portugal",       "country": "Portugal",      "shortName": "POR", "foundedYear": 1914, "coach": "Coach Portugal"},
    {"name": "Netherlands",    "country": "Netherlands",   "shortName": "NED", "foundedYear": 1889, "coach": "Coach Netherlands"},
    {"name": "Italy",          "country": "Italy",         "shortName": "ITA", "foundedYear": 1898, "coach": "Coach Italy"},
    {"name": "Belgium",        "country": "Belgium",       "shortName": "BEL", "foundedYear": 1895, "coach": "Coach Belgium"},
    {"name": "Uruguay",        "country": "Uruguay",       "shortName": "URU", "foundedYear": 1900, "coach": "Coach Uruguay"},
    {"name": "Croatia",        "country": "Croatia",       "shortName": "CRO", "foundedYear": 1912, "coach": "Coach Croatia"},
    {"name": "Mexico",         "country": "Mexico",        "shortName": "MEX", "foundedYear": 1927, "coach": "Coach Mexico"},
    {"name": "United States",  "country": "United States", "shortName": "USA", "foundedYear": 1913, "coach": "Coach USA"},
    {"name": "Japan",          "country": "Japan",         "shortName": "JPN", "foundedYear": 1921, "coach": "Coach Japan"},
    {"name": "South Korea",    "country": "South Korea",   "shortName": "KOR", "foundedYear": 1928, "coach": "Coach Korea"},
    {"name": "Denmark",        "country": "Denmark",       "shortName": "DEN", "foundedYear": 1889, "coach": "Coach Denmark"},
    {"name": "Switzerland",    "country": "Switzerland",   "shortName": "SUI", "foundedYear": 1895, "coach": "Coach Switzerland"},
    {"name": "Colombia",       "country": "Colombia",      "shortName": "COL", "foundedYear": 1924, "coach": "Coach Colombia"},
    {"name": "Chile",          "country": "Chile",         "shortName": "CHI", "foundedYear": 1895, "coach": "Coach Chile"},
    {"name": "Sweden",         "country": "Sweden",        "shortName": "SWE", "foundedYear": 1904, "coach": "Coach Sweden"},
    {"name": "Norway",         "country": "Norway",        "shortName": "NOR", "foundedYear": 1902, "coach": "Coach Norway"},
    {"name": "Poland",         "country": "Poland",        "shortName": "POL", "foundedYear": 1919, "coach": "Coach Poland"},
    {"name": "Austria",        "country": "Austria",       "shortName": "AUT", "foundedYear": 1904, "coach": "Coach Austria"},
    {"name": "Serbia",         "country": "Serbia",        "shortName": "SRB", "foundedYear": 1919, "coach": "Coach Serbia"},
    {"name": "Morocco",        "country": "Morocco",       "shortName": "MAR", "foundedYear": 1956, "coach": "Coach Morocco"},
    {"name": "Nigeria",        "country": "Nigeria",       "shortName": "NGA", "foundedYear": 1923, "coach": "Coach Nigeria"},
    {"name": "Senegal",        "country": "Senegal",       "shortName": "SEN", "foundedYear": 1960, "coach": "Coach Senegal"},
    {"name": "Australia",      "country": "Australia",     "shortName": "AUS", "foundedYear": 1961, "coach": "Coach Australia"},
    {"name": "Ecuador",        "country": "Ecuador",       "shortName": "ECU", "foundedYear": 1925, "coach": "Coach Ecuador"},
    {"name": "Peru",           "country": "Peru",          "shortName": "PER", "foundedYear": 1922, "coach": "Coach Peru"},
    {"name": "Canada",         "country": "Canada",        "shortName": "CAN", "foundedYear": 1912, "coach": "Coach Canada"},
]

def http_post(path: str, json_body: dict | None = None) -> Tuple[int, dict | str]:
    url = f"{BASE_URL}{path}"
    try:
        resp = requests.post(url, json=json_body)
    except requests.RequestException as exc:
        return 0, f"Request error: {exc}"
    try:
        data = resp.json()
    except ValueError:
        data = resp.text
    return resp.status_code, data

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
    if status == 409:
        print(f"[WARN] Tournament already exists. Body={data}")
        return None
    print(f"[FAIL] Tournament create failed. Status={status} Body={data}")
    return None

def create_groups(tid: str) -> List[dict]:
    group_ids: List[dict] = []
    for i in range(1, 9):
        name = f"grupo{i}"
        status, data = http_post(GROUPS_ENDPOINT_FMT.format(tid=tid), {"name": name})
        if status == 201 and isinstance(data, dict):
            gid = data.get("id")
            print(f"[OK] Group created: {name} id={gid}")
            group_ids.append({"name": name, "id": gid})
        elif status == 409:
            print(f"[WARN] Group {name} already exists. Body={data}")
        else:
            print(f"[FAIL] Group {name} create failed. Status={status} Body={data}")
    return group_ids

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
        tid = data.get("id")
        print(f"[OK] Team created: {team['name']} id={tid}")
        return tid
    if status == 409:
        print(f"[WARN] Team exists: {team['name']}. Body={data}")
        return None
    print(f"[FAIL] Team create failed: {team['name']}. Status={status} Body={data}")
    return None

def add_team_to_group(tournament_id: str, group_id: str, team_id: str) -> bool:
    path = ADD_TEAM_TO_GROUP_FMT.format(tid=tournament_id, gid=group_id, teamId=team_id)
    status, data = http_post(path, None)
    if status in (200, 204):
        print(f"[OK] Added team {team_id} to group {group_id}")
        return True
    print(f"[FAIL] Add team to group failed. Status={status} Body={data}")
    return False

def chunk(lst: List[str], size: int) -> List[List[str]]:
    return [lst[i:i+size] for i in range(0, len(lst), size)]

def main() -> None:
    # 1) Create tournament
    tournament_id = create_world_cup()
    if not tournament_id:
        print("[STOP] No tournament id. Exiting.")
        return

    # 2) Create 8 groups
    groups = create_groups(tournament_id)
    # Keep only groups that returned an id
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
        group = groups[idx]
        gid = group["id"]
        for team_id in teams_chunk:
            add_team_to_group(tournament_id, gid, team_id)

    print("[DONE] Seeding complete.")

if __name__ == "__main__":
    main()
