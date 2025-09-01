-- podman run -d --name=tournament_db --network development -e POSTGRES_USER=admin -e POSTGRES_PASSWORD=admin -p 5432:5432 tournament_db

CREATE USER tournament_svc WITH PASSWORD 'password';
CREATE USER tournament_admin WITH PASSWORD 'password';

CREATE DATABASE tournament_db;


------tournament_admin user

grant all privileges on database tournament_db to tournament_admin;
grant all privileges on database tournament_db to tournament_svc;
grant usage on schema public to tournament_admin;
grant usage on schema public to tournament_svc;

GRANT SELECT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT DELETE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT UPDATE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT INSERT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT CREATE ON SCHEMA public TO tournament_admin;

GRANT SELECT ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT DELETE ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT UPDATE ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT INSERT ON ALL TABLES IN SCHEMA public TO tournament_svc;