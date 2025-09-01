CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE TEAMS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX team_unique_name_idx ON teams ((document->>'name'));

CREATE TABLE GROUPS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    TOURNAMENT_ID UUID references TEAMS(ID),
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_gruop_unique_name_idx ON GROUPS (tournament_id,(document->>'name'));

CREATE TABLE TOURNAMENTS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_unique_name_idx ON TOURNAMENTS ((document->>'name'));

CREATE TABLE MATCHES (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);