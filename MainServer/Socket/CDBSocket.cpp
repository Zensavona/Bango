#include "CDBSocket.h"

SOCKET CDBSocket::g_pDBSocket = INVALID_SOCKET;

bool CDBSocket::Connect(WORD wPort)
{
	CDBSocket::g_pDBSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (CDBSocket::g_pDBSocket <= INVALID_SOCKET) {
		printf("Error creating socket.\n");
		return false;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(wPort);

    if (connect(CDBSocket::g_pDBSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <= SOCKET_ERROR) {
    	printf("Failed to connect to DB Server.\n");
    	return false;
    }
    
	printf("Connected to DBServer.\n");

	pthread_t t;
	pthread_create(&t, NULL, &CDBSocket::ProcessDB, NULL);

	return true;
}

bool CDBSocket::Close()
{
	return true;
}

PVOID CDBSocket::ProcessDB(PVOID param)
{
	while (true)
	{
		Packet *packet = new Packet;
		memset(packet, 0, sizeof(Packet));
		//Sleep?

		int nLen = recv(CDBSocket::g_pDBSocket, packet, MAX_PACKET_LENGTH + (packet->data-(char*)packet), 0);
		if (nLen <= 0 || packet->wSize <=0) {
			printf("DBServer disconnected.\n");
			break;
		}

		if (nLen > MAX_PACKET_LENGTH || packet->wSize > MAX_PACKET_LENGTH) continue;

		DebugRawPacket(packet);

		pthread_t t;
		pthread_create(&t, NULL, &CDBSocket::Process, (PVOID)packet);
	}
}

PVOID CDBSocket::Process(PVOID param)
{
	Packet* packet = (Packet*)param;

	switch (packet->byType)
	{
		case D2S_LOGIN:
		{
			BYTE byAnswer=0;
			int nClientID=0;
			char *p = CSocket::ReadPacket(packet->data, "bd", &byAnswer, &nClientID);

			CClient *pClient = CServer::FindClient(nClientID);
			if (!pClient) break;

			if (byAnswer == LA_SAMEUSER) {
				int nClientExID=0;
				CSocket::ReadPacket(p, "d", &nClientExID);

				auto pClientEx = CServer::FindClient(nClientExID);
				if (pClientEx)
					pClientEx->Write(S2C_CLOSE, "b", CC_SAMEUSER);
			}

			pClient->Write(S2C_ANS_LOGIN, "b", byAnswer);
			printf("S2C_ANS_LOGIN sent.\n");

			break;
		}

		case D2S_SEC_LOGIN:
		{
			int nClientID=0;
			BYTE byAnswer=0;
			CSocket::ReadPacket(packet->data, "bd", &byAnswer, &nClientID);

			CClient *pClient = CServer::FindClient(nClientID);
			if (pClient)
				pClient->Write(S2C_SECOND_LOGIN, "bb", SL_RESULT_MSG, byAnswer);

			break;
		}

		case D2S_PLAYER_INFO:
		{
			int nClientID=0;
			char *p = CSocket::ReadPacket(packet->data, "d", &nClientID);

			CClient *pClient = CServer::FindClient(nClientID);
			if (!pClient) break;

			BYTE byAuth=0;
			int nExpTime=0;
			BYTE byUnknwon=0;

			pClient->Write(S2C_PLAYERINFO, "bbdm", byAuth, byUnknwon, nExpTime, 
				p, ((char*)packet + packet->wSize) - p);
			printf("S2C_PLAYERINFO sent.\n");

			break;
			/*

					BYTE byAuth=0;
					int nExpTime=0;
					BYTE byUnknwon=0;

					BYTE byCount=1;

					int nPID = 16;
					const char* pName = "lafreak";
					BYTE byJob=1;
					BYTE byClass=1;
					BYTE byLevel=100;
					int  nGID=0;
					WORD wStr=105;
					WORD wHth=106;
					WORD wInt=107;
					WORD wWis=108;
					WORD wDex=109;
					BYTE byFace=0;
					BYTE byHair=0;

					BYTE byWearItemCount=6;
					WORD wGloves=1134;
					WORD wBoots=1135;
					WORD wHelm=1136;
					WORD wShorts=1137;
					WORD wChest=1138;
					WORD wStick=668;


					//			p = ReadPacket( p, "bbd b dsbbdwwwwwbb b www...."


					pClient->Write(S2C_PLAYERINFO, "bbdbdsbbbdwwwwwbbbwwwwww", byAuth, byUnknwon, nExpTime, byCount,
						nPID, pName, byJob, byClass, byLevel, nGID, wStr, wHth, wInt, wWis, wDex, byFace, byHair, byWearItemCount,
						wGloves, wBoots, wHelm, wShorts, wChest, wStick);
					printf("S2C_PLAYERINFO sent.\n");
			*/
		}
	}

	delete packet;
	return NULL;
}

bool CDBSocket::Write(BYTE byType, ...)
{
	if (CDBSocket::g_pDBSocket == INVALID_SOCKET)
		return false;

	Packet packet;
	memset(&packet, 0, sizeof(Packet));

	packet.byType = byType;

	va_list va;
	va_start(va, byType);

	char* end = CSocket::WriteV(packet.data, va);

	va_end(va);

	packet.wSize = end - (char*)&packet;
	send(CDBSocket::g_pDBSocket, (char*)&packet, packet.wSize, 0);

	return true;
}

void CDBSocket::DebugRawPacket(Packet *packet)
{
	printf("Incoming D2S packet: [%u]\n", (BYTE)packet->byType);
	for (int i = 0; i < packet->wSize; i++)
		printf("%u ", (BYTE)((char*)packet)[i]);
	printf("\n");
}

