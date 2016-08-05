#ifndef _CACCOUNT_
#define _CACCOUNT_

#include <string>

#include <minwindef.h>

class CAccount
{
	SOCKET m_pSocket;
	std::string m_szLogin;
	std::string m_szPassword;

public:
	CAccount(SOCKET pSocket, std::string szLogin, std::string szPassword): 
		m_pSocket(pSocket), m_szLogin(szLogin), m_szPassword(szPassword) {}
	~CAccount() {}

	std::string GetLogin() const { return m_szLogin; }
	std::string GetPassword() const { return m_szPassword; }

	SOCKET 		GetSocket() const { return m_pSocket; }
};

#endif