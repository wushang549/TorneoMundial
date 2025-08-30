#ifndef POSTGRES_CONFIG_HPP
#define POSTGRES_CONFIG_HPP

#include <pqxx/pqxx>

namespace persistence{
    namespace config {
        void GetConnection() {
            pqxx::connection connection("postgresql://tournament_db:password@localhost:5432/tournamen_db");
            pqxx::work tx(connection);
            pqxx::row row = tx.exec1("SELECT 1");
            // pqxx::result r = tx.exec("SELECT $1", pqxx::params{argv[1]});
            //r[0][0].c_str()
            tx.commit();

        }
    }
}
#endif