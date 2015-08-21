#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <Neptune.h>
#include "ThreadTask.h"


class CHttpHelper {
public:
	static bool         IsConnectionKeepAlive(NPT_HttpMessage& message);
	static bool         IsBodyStreamSeekable(NPT_HttpMessage& message);

	static NPT_Result   ToLog(NPT_LoggerReference logger, int level, const char* prefix, NPT_HttpRequest* request);
	static NPT_Result   ToLog(NPT_LoggerReference logger, int level, const char* prefix, const NPT_HttpRequest& request);
	static NPT_Result   ToLog(NPT_LoggerReference logger, int level, const char* prefix, NPT_HttpResponse* response);
	static NPT_Result   ToLog(NPT_LoggerReference logger, int level, const char* prefix, const NPT_HttpResponse& response);

	static NPT_Result   GetContentType(const NPT_HttpMessage& message, NPT_String& type);
	static NPT_Result   GetContentLength(const NPT_HttpMessage& message, NPT_LargeSize& len);

	static NPT_Result   GetHost(const NPT_HttpRequest& request, NPT_String& value);
	static void         SetHost(NPT_HttpRequest& request, const char* host);
	//static PLT_DeviceSignature GetDeviceSignature(const NPT_HttpRequest& request);

	static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_String& text, NPT_HttpEntity** entity = NULL);
	static NPT_Result   SetBody(NPT_HttpMessage& message, const char* text, NPT_HttpEntity** entity = NULL);
	static NPT_Result   SetBody(NPT_HttpMessage& message, const void* body, NPT_LargeSize len, NPT_HttpEntity** entity = NULL);
	static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_InputStreamReference stream, NPT_HttpEntity** entity = NULL);
	static NPT_Result   GetBody(const NPT_HttpMessage& message, NPT_String& body);
	static NPT_Result   ParseBody(const NPT_HttpMessage& message, NPT_XmlElementNode*& xml);

	static void			SetBasicAuthorization(NPT_HttpRequest& request, const char* username, const char* password);
};


class CHttpServerSocketTask : public CThreadTask
{
	friend class CThreadTask;
public:
	CHttpServerSocketTask(NPT_Socket* socket, bool stay_alive_forver = false);

protected:
	// Request callback handler
	virtual NPT_Result SetupResponse(NPT_HttpRequest& request, 
		const NPT_HttpRequestContext& context, 
		NPT_HttpResponse& response) = 0;
	

	virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream);
	virtual NPT_Result GetInfo(NPT_SocketInfo& info);

	virtual void DoAbort() { if (m_Socket) m_Socket->Cancel(); }
	virtual void DoRun();

	virtual ~CHttpServerSocketTask();

private:
	virtual NPT_Result Read(NPT_BufferedInputStreamReference& buffered_input_stream, 
		NPT_HttpRequest*& request, 
		NPT_HttpRequestContext* context = false);
	virtual NPT_Result Write(NPT_HttpResponse* response, bool& keep_alive, bool headers_only = false);
	virtual NPT_Result RespondToClient(NPT_HttpRequest& request, 
		const NPT_HttpRequestContext& connext, 
		NPT_HttpResponse*& response);

protected:
	NPT_Socket* m_Socket;
	bool m_StayAliveForever;
};


class CHttpServer 
	: public NPT_HttpRequestHandler
	, public NPT_HttpServer
{
public:
	CHttpServer(NPT_IpAddress address = NPT_IpAddress::Any, 
		NPT_IpPort port = 0, 
		bool allow_random_port_on_bingd_failure = false, 
		NPT_Cardinal max_clients = 50, 
		bool reuse_address = false);
	virtual ~CHttpServer();

	// class methods
	static NPT_Result ServeFile(const NPT_HttpRequest&        request, 
		const NPT_HttpRequestContext& context,
		NPT_HttpResponse&             response, 
		NPT_String                    file_path);
	static NPT_Result ServeStream(const NPT_HttpRequest&        request, 
		const NPT_HttpRequestContext& context,
		NPT_HttpResponse&             response,
		NPT_InputStreamReference&     stream, 
		const char*                   content_type);

	// NPT_HttpRequestHandler methods
	virtual NPT_Result SetupResponse(NPT_HttpRequest&              request,
		const NPT_HttpRequestContext& context,
		NPT_HttpResponse&             response);

	// methods
	virtual NPT_Result   Start();
	virtual NPT_Result   Stop();
	virtual unsigned int GetPort() { return m_Port; }

private:
	CThreadManager* m_TaskManager;
	NPT_IpAddress m_Address;
	NPT_IpPort m_Port;
	bool m_AllowRandomPortOnBindFailure;
	bool m_ReuseAddress;
	bool m_Aborted;
};


class CHttpListenTask : public CThreadTask
{
public:
	CHttpListenTask(NPT_HttpRequestHandler* handler, 
		NPT_TcpServerSocket* socket, 
		bool owns_socket = true)
		: m_Handler(handler)
		, m_Socket(socket)
		, m_OwnsSocket(owns_socket)
	{}

protected:
	virtual ~CHttpListenTask() { if (m_OwnsSocket && m_Socket) delete m_Socket; }

protected:
	virtual void DoAbort() { if (m_Socket) m_Socket->Cancel(); }
	virtual void DoRun();

protected:
	NPT_HttpRequestHandler* m_Handler;
	NPT_TcpServerSocket* m_Socket;
	bool m_OwnsSocket;
};

#endif	// __HTTP_SERVER_H__