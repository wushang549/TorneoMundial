from typing import Any
from locust import HttpUser, task
import uuid

NUMBER_OF_GROUPS = 8
TEAMS_PER_GROUP = 4
TOTAL_TEAMS = NUMBER_OF_GROUPS * TEAMS_PER_GROUP

class TournamentUser(HttpUser):

    def _extract_id_from_location(self, location: str | None) -> str | None:
        if not location:
            return None
        # If it's a URL like http://.../teams/<id>, return the last part
        return location.rstrip("/").split("/")[-1]

    def create_teams(self, total: int):
        team_ids: list[str] = []
        for _ in range(total):
            team_data = {"name": f"Team {uuid.uuid4()}"}
            with self.client.post(
                    "/teams",
                    json=team_data,
                    catch_response=True,
                    name="POST /teams"
            ) as response:
                if response.status_code in (200, 201):
                    location = response.headers.get("Location") or response.headers.get("location")
                    team_id = self._extract_id_from_location(location)
                    if team_id:
                        team_ids.append(team_id)
                    else:
                        response.failure("Missing team id in Location header")
                else:
                    response.failure(f"Team creation failed: {response.status_code}")
        return team_ids

    def create_tournament(self) -> str | None:
        tournament_data = {
            "name": f"Tournament - {uuid.uuid4()}",
            "format": {
                "numberOfGroups": NUMBER_OF_GROUPS,
                "maxTeamsPerGroup": TEAMS_PER_GROUP,
                "type": "ROUND_ROBIN"
            }
        }
        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code in (200, 201):
                location = response.headers.get("Location") or response.headers.get("location")
                return self._extract_id_from_location(location)
            else:
                response.failure(f"Tournament creation failed: {response.status_code}")
                return None

    def create_groups(self, tournament_id: str):
        group_ids: list[str] = []
        for i in range(NUMBER_OF_GROUPS):
            group_data = {
                "name": f"Group {i + 1}"
            }
            with self.client.post(
                    f"/tournaments/{tournament_id}/groups",
                    json=group_data,
                    catch_response=True,
                    name="POST /tournaments/{tournament_id}/groups"
            ) as response:
                if response.status_code in (200, 201):
                    location = response.headers.get("Location") or response.headers.get("location")
                    group_id = self._extract_id_from_location(location)
                    if group_id:
                        group_ids.append(group_id)
                    else:
                        response.failure("Missing group id in Location header")
                else:
                    response.failure(f"Group creation failed: {response.status_code}")
        return group_ids

    def add_teams_to_group(self, tournament_id: str, group_id: str, team_ids: list[str]):
        payload = [{"id": t_id} for t_id in team_ids]
        with self.client.post(
                f"/tournaments/{tournament_id}/groups/{group_id}",
                json=payload,
                catch_response=True,
                name="POST /tournaments/{tournament_id}/groups/{group_id}"
        ) as response:
            if response.status_code not in (200, 201, 204):
                response.failure(f"Add teams failed: {response.status_code}")

    @task
    def world_cup_flow(self):
        teams = self.create_teams(TOTAL_TEAMS)
        tournament_id = self.create_tournament()
        if not tournament_id:
            return

        groups = self.create_groups(tournament_id)
        if len(groups) != NUMBER_OF_GROUPS:
            return

        # Slice teams per group (4 per group)
        for i, group_id in enumerate(groups):
            start = i * TEAMS_PER_GROUP
            end = start + TEAMS_PER_GROUP
            self.add_teams_to_group(tournament_id, group_id, teams[start:end])
