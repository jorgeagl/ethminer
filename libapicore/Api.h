#ifndef _API_H_
#define _API_H_

#include <string>

#include "ApiServer.h"
#include <libethcore/Farm.h>
#include <libethcore/Miner.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace jsonrpc;

class Api
{
public:
	Api(const int &port, Farm &farm, const string &farmPool, const string &farmPort);
private:
	ApiServer *m_server;
	Farm &m_farm;
};

#endif //_API_H_