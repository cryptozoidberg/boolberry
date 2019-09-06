#include <string>

#include "include_base_utils.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server.h"

int main () {
    log_space::get_set_log_detalisation_level(true, LOG_LEVEL_1);
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_1);
  
    std::string daemon_address = "http://localhost:10102";
    epee::net_utils::http::http_simple_client http_client;
    currency::COMMAND_RPC_GETBLOCK::request req = AUTO_VAL_INIT(req);
    for (uint64_t i=778000; i<std::numeric_limits<uint64_t>::max(); ++i) {
        if (i%1000 == 0) {
            std::cout << i << std::endl;
        }
        req.height = i;
        currency::COMMAND_RPC_GETBLOCK::response res = AUTO_VAL_INIT(res);
        bool r = epee::net_utils::invoke_http_json_rpc(daemon_address + "/json_rpc", "getblock", req, res, http_client, 10000);
        if (!r) {
            std::cout << "Error requesting block " << i << std::endl;
        }
        for (const auto& e: res.transfers) {
            if (e.payment_id.size()) {
                //std::cout << "Found payment id in block " << i << std::endl;
            }
        }
    }
}
