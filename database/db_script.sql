-- podman run -d --replace --name=tournament_db --network development -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
-- podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql

CREATE USER tournament_svc WITH PASSWORD 'password';
CREATE USER tournament_admin WITH PASSWORD 'password';

CREATE DATABASE tournament_db;

\connect tournament_db

grant all privileges on database tournament_db to tournament_admin;
grant all privileges on database tournament_db to tournament_svc;
grant usage on schema public to tournament_admin;
grant usage on schema public to tournament_svc;

GRANT SELECT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT DELETE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT UPDATE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT INSERT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT CREATE ON SCHEMA public TO tournament_admin;

\connect tournament_db tournament_admin

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE TEAMS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX team_unique_name_idx ON teams ((document->>'name'));

CREATE TABLE TOURNAMENTS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_unique_name_idx ON TOURNAMENTS ((document->>'name'));

CREATE TABLE GROUPS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    TOURNAMENT_ID UUID not null references TOURNAMENTS(ID),
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_group_unique_name_idx ON GROUPS (tournament_id,(document->>'name'));

-- Matches table: stores per-match JSON document and links to its tournament
CREATE TABLE MATCHES (
                         id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
                         tournament_id UUID NOT NULL REFERENCES TOURNAMENTS(id) ON DELETE CASCADE,
                         document JSONB NOT NULL,
                         last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                         created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Fast listing/filtering by tournament
CREATE INDEX idx_matches_tournament ON MATCHES (tournament_id);

-- Prevent exact duplicates for the same tournament/round/home/visitor
-- (Note: this forbids A(home)-B(visitor) duplicates; if you want to also
-- forbid B(home)-A(visitor) as the "same" game, we can add a trigger later.)
CREATE UNIQUE INDEX match_unique_per_round_idx
    ON MATCHES (
                tournament_id,
        (document->>'round'),
        (document->'home'->>'id'),
        (document->'visitor'->>'id')
        );

-- Keep JSON document consistent with the relational FK (helps catch bugs early)
ALTER TABLE MATCHES
    ADD CONSTRAINT matches_document_tournament_consistent
        CHECK ( (document->>'tournamentId')::uuid = tournament_id );


GRANT SELECT ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT DELETE ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT UPDATE ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT INSERT ON ALL TABLES IN SCHEMA public TO tournament_svc;
