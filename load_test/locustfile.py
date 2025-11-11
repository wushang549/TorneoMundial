#!/usr/bin/env python3
import requests

BASE_URL = "http://host.containers.internal:8081"

TOURNAMENTS_ENDPOINT = "/tournaments"
TEAMS_ENDPOINT = "/teams"

TOURNAMENT_NAME = "World Cup 2025"
TOURNAMENT_TYPE = "WORLD_CUP"

TEAMS = [
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


def create_world_cup() -> str | None:
    url = f"{BASE_URL}{TOURNAMENTS_ENDPOINT}"
    payload = {
        "name": TOURNAMENT_NAME,
        "format": {
            "numberOfGroups": 8,
            "maxTeamsPerGroup": 4,
            "type": TOURNAMENT_TYPE,
        },
    }

    try:
        response = requests.post(url, json=payload)
    except requests.RequestException as exc:
        print(f"[ERROR] Request failed when creating tournament: {exc}")
        return None

    if response.status_code == 201:
        try:
            data = response.json()
        except ValueError:
            data = {}

        tournament_id = data.get("id")
        print(f"[OK] Created tournament '{TOURNAMENT_NAME}' with id={tournament_id}")

        if tournament_id:
            with open("world_cup_tournament_id.txt", "w", encoding="utf-8") as f:
                f.write(tournament_id)
            print("[INFO] Saved tournament id to world_cup_tournament_id.txt")

        return tournament_id

    if response.status_code == 409:
        print(f"[WARN] Tournament '{TOURNAMENT_NAME}' already exists (409 Conflict)")
        print(f"Body: {response.text}")
        return None

    print(
        f"[FAIL] Could not create tournament. "
        f"Status={response.status_code}, Body={response.text}"
    )
    return None


def create_team(team: dict) -> None:
    url = f"{BASE_URL}{TEAMS_ENDPOINT}"
    payload = {
        "name": team["name"],
        "country": team["country"],
        "shortName": team["shortName"],
        "foundedYear": team["foundedYear"],
        "coach": team["coach"],
    }

    try:
        response = requests.post(url, json=payload)
    except requests.RequestException as exc:
        print(f"[ERROR] Request failed for team '{team['name']}': {exc}")
        return

    if response.status_code == 201:
        try:
            data = response.json()
        except ValueError:
            data = {}
        team_id = data.get("id", "<no-id-returned>")
        print(f"[OK] Created team '{team['name']}' with id={team_id}")
    elif response.status_code == 409:
        print(f"[WARN] Team '{team['name']}' already exists (409 Conflict)")
    else:
        print(
            f"[FAIL] Could not create team '{team['name']}'. "
            f"Status={response.status_code}, Body={response.text}"
        )


def main() -> None:
    tournament_id = create_world_cup()
    if not tournament_id:
        print("[INFO] Skipping team creation because tournament was not created.")
    else:
        print(f"[INFO] Using tournament id: {tournament_id} (for next steps later)")
    print("[INFO] Creating 32 teams...")
    for team in TEAMS:
        create_team(team)


if __name__ == "__main__":
    main()
