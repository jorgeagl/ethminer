#ifndef _APISERVER_H_
#define _APISERVER_H_

#include <string>

#include <libethcore/Farm.h>
#include <libethcore/Miner.h>
#include <jsonrpccpp/server.h>

using namespace std;
using namespace std::chrono;
using namespace dev;
using namespace dev::eth;
using namespace jsonrpc;

class ApiServer : public AbstractServer<ApiServer>
{
public:
	ApiServer(AbstractServerConnector *conn, serverVersion_t type, Farm &farm, bool &readonly);

	void SetPool(const string pool);
private:
	Farm &m_farm;
	string pool;

	void getMinerStat1(const Json::Value& request, Json::Value& response);
	void doMinerRestart(const Json::Value& request, Json::Value& response);
	void doMinerReboot(const Json::Value& request, Json::Value& response);
};

#endif //_APISERVER_H_