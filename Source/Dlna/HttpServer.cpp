#include "HttpServer.h"
#include "Utilities.h"

//////////////////////////////////////////////////////////////////////////
// CHttpHelper
bool CHttpHelper::IsConnectionKeepAlive(NPT_HttpMessage& message) 
{
	const NPT_String* connection = 
		message.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_CONNECTION);

	// the DLNA says that all HTTP 1.0 requests should be closed immediately by the server
	NPT_String protocol = message.GetProtocol();
	if (protocol.Compare(NPT_HTTP_PROTOCOL_1_0, true) == 0) return false;

	// all HTTP 1.1 requests without a Connection header 
	// or with a keep-alive Connection header should be kept alive if possible 
	return (!connection || connection->Compare("keep-alive", true) == 0);
}


//////////////////////////////////////////////////////////////////////////
// class CHttpServerSocketTask
CHttpServerSocketTask::CHttpServerSocketTask(NPT_Socket* socket, bool stay_alive_forver /* = false */)
	: m_Socket(socket)
	, m_StayAliveForever(stay_alive_forver)
{
	m_Socket->SetReadTimeout(60000);
	m_Socket->SetWriteTimeout(600000);
}

CHttpServerSocketTask::~CHttpServerSocketTask()
{
	if (m_Socket) delete m_Socket;
}

void CHttpServerSocketTask::DoRun()
{
	NPT_BufferedInputStreamReference buffered_input_stream;
	NPT_HttpRequestContext           context;
	NPT_Result                       res = NPT_SUCCESS;
	bool                             headers_only;
	bool                             keep_alive = false;

	// create a buffered input stream to parse HTTP request
	NPT_InputStreamReference input_stream;
	if (NPT_FAILED(GetInputStream(input_stream))) goto done;
	input_stream.AsPointer();
	buffered_input_stream = new NPT_BufferedInputStream(input_stream);

	while (!IsAborting(0)) {
		NPT_HttpRequest*  request = NULL;
		NPT_HttpResponse* response = NULL;

		// reset keep-alive to exit task on read failure
		keep_alive = false;

		// wait for a request
		res = Read(buffered_input_stream, request, &context);
		if (NPT_FAILED(res) || (request == NULL)) 
			goto cleanup;

		// process request and setup response
		res = RespondToClient(*request, context, response);
		if (NPT_FAILED(res) || (response == NULL)) 
			goto cleanup;

		// check if client requested keep-alive
		keep_alive = CHttpHelper::IsConnectionKeepAlive(*request);
		headers_only = request->GetMethod() == NPT_HTTP_METHOD_HEAD;

		// send response, pass keep-alive request from client
		// (it can be overridden if response handler did not allow it)
		res = Write(response, keep_alive, headers_only);

		// on write error, reset keep_alive so we can close this connection
		if (NPT_FAILED(res)) keep_alive = false;

cleanup:
		// cleanup
		delete request;
		delete response;

		if (!keep_alive && !m_StayAliveForever) {
			return;
		}
	}
done:
	return;
}

NPT_Result CHttpServerSocketTask::GetInputStream(NPT_InputStreamReference& stream)
{
	return m_Socket->GetInputStream(stream);
}

NPT_Result CHttpServerSocketTask::GetInfo(NPT_SocketInfo& info)
{
	return m_Socket->GetInfo(info);
}

NPT_Result CHttpServerSocketTask::Read(NPT_BufferedInputStreamReference& buffered_input_stream, NPT_HttpRequest*& request, NPT_HttpRequestContext* context /* = false */)
{
	NPT_SocketInfo info;
	GetInfo(info);

	if (context)
	{
		context->SetLocalAddress(info.local_address);
		context->SetRemoteAddress(info.remote_address);
	}

	buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);
	NPT_Result res = NPT_HttpRequest::Parse(*buffered_input_stream, &info.local_address, request);
	if (NPT_FAILED(res) || !request)
	{
		res = NPT_FAILED(res) ? res : NPT_FAILURE;
		if (res != NPT_ERROR_TIMEOUT && res != NPT_ERROR_EOS)
			return res;
	}

	// update context with socket info again 
	// to refresh the remote address in case it was a non connected udp socket
	GetInfo(info);
	if (context) {
		context->SetLocalAddress(info.local_address);
		context->SetRemoteAddress(info.remote_address);
	}

	// return right away if no body is expected
	if (request->GetMethod() == NPT_HTTP_METHOD_GET || 
		request->GetMethod() == NPT_HTTP_METHOD_HEAD) {
			return NPT_SUCCESS;
	}

	// create an entity
	NPT_HttpEntity* request_entity = new NPT_HttpEntity(request->GetHeaders());
	request->SetEntity(request_entity);

	NPT_MemoryStream* body_stream = new NPT_MemoryStream();
	request_entity->SetInputStream((NPT_InputStreamReference)body_stream);

	// unbuffer the stream to read body fast
	buffered_input_stream->SetBufferSize(0);

	// check for chunked Transfer-Encoding
	if (request_entity->GetTransferEncoding() == "chunked")
	{
		NPT_StreamToStreamCopy(*NPT_InputStreamReference(new NPT_HttpChunkedInputStream(buffered_input_stream)).AsPointer(), *body_stream);
		request_entity->SetTransferEncoding(NULL);
	}
	else if (request_entity->GetContentLength())
	{
		// a request with a body must always have a content length if not chunked
		NPT_StreamToStreamCopy(
			*buffered_input_stream.AsPointer(), 
			*body_stream, 
			0, 
			request_entity->GetContentLength());
	}
	else
	{
		request->SetEntity(NULL);
	}

	// rebuffer the stream
	buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);

	return NPT_SUCCESS;
}

NPT_Result CHttpServerSocketTask::Write(NPT_HttpResponse* response, bool& keep_alive, bool headers_only /* = false */)
{
	return NPT_SUCCESS;
}

NPT_Result CHttpServerSocketTask::RespondToClient(NPT_HttpRequest& request, const NPT_HttpRequestContext& connext, NPT_HttpResponse*& response)
{
	NPT_Result result = NPT_ERROR_NO_SUCH_ITEM;

	return NPT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// CHttpServer
CHttpServer::CHttpServer(NPT_IpAddress address /* = NPT_IpAddress::Any */, NPT_IpPort port /* = 0 */, bool allow_random_port_on_bingd_failure /* = false */, NPT_Cardinal max_clients /* = 50 */, bool reuse_address /* = false */)
{
}

CHttpServer::~CHttpServer()
{
	Stop();
	delete m_TaskManager;
}


NPT_Result CHttpServer::Start()
{
    NPT_Result res = NPT_FAILURE;
    
    // we can't restart an aborted server
    if (m_Aborted) return NPT_ERROR_INVALID_STATE;
    
    // if we're given a port for our http server, try it
    if (m_Port) {
        res = SetListenPort(m_Port, m_ReuseAddress);
        // return right away if failed and not allowed to try again randomly
        //if (NPT_FAILED(res) && !m_AllowRandomPortOnBindFailure) {
        //    NPT_CHECK_SEVERE(res);
        //}
    }
    
    // try random port now
    if (!m_Port || NPT_FAILED(res)) {
        int retries = 100;
        do {    
            int random = NPT_System::GetRandomInteger();
            int port = (unsigned short)(1024 + (random % 1024));
            if (NPT_SUCCEEDED(SetListenPort(port, m_ReuseAddress))) {
                break;
            }
        } while (--retries > 0);

        //if (retries == 0) NPT_CHECK_SEVERE(NPT_FAILURE);
    }

    // keep track of port server has successfully bound
    m_Port = m_BoundPort;

    // Tell server to try to listen to more incoming sockets
    // (this could fail silently)
    if (m_TaskManager->GetMaxTasks() > 20) {
        m_Socket.Listen(m_TaskManager->GetMaxTasks());
    }
    
    // start a task to listen for incoming connections
    CHttpListenTask *task = new CHttpListenTask(this, &m_Socket, false);
    //NPT_CHECK_SEVERE(m_TaskManager->StartTask(task));

    NPT_SocketInfo info;
    m_Socket.GetInfo(info);
    //NPT_LOG_INFO_2("HttpServer listening on %s:%d", 
    //    (const char*)info.local_address.GetIpAddress().ToString(), 
    //    m_Port);
    return NPT_SUCCESS;
}


NPT_Result CHttpServer::Stop()
{
    m_Aborted = true;

    // stop all other pending tasks 
    m_TaskManager->StopAllTasks();
    return NPT_SUCCESS;
}


NPT_Result CHttpServer::SetupResponse(NPT_HttpRequest&              request, 
                              const NPT_HttpRequestContext& context,
                              NPT_HttpResponse&             response) 
{
    NPT_String prefix = NPT_String::Format("PLT_HttpServer::SetupResponse %s request from %s for \"%s\"", 
        (const char*) request.GetMethod(),
        (const char*) context.GetRemoteAddress().ToString(),
        (const char*) request.GetUrl().ToString());

    NPT_List<NPT_HttpRequestHandler*> handlers = FindRequestHandlers(request);
    if (handlers.GetItemCount() == 0) return NPT_ERROR_NO_SUCH_ITEM;

    // ask the handler to setup the response
    NPT_Result result = (*handlers.GetFirstItem())->SetupResponse(request, context, response);
    
    // DLNA compliance
    UPNPMessageHelper::SetDate(response);
    if (request.GetHeaders().GetHeader("Accept-Language")) {
        response.GetHeaders().SetHeader("Content-Language", "en");
    }
    return result;
}


NPT_Result CHttpServer::ServeFile(const NPT_HttpRequest&        request, 
                          const NPT_HttpRequestContext& context,
                          NPT_HttpResponse&             response,
                          NPT_String                    file_path) 
{
    NPT_InputStreamReference stream;
    NPT_File                 file(file_path);
    NPT_FileInfo             file_info;
    
    // prevent hackers from accessing files outside of our root
    if ((file_path.Find("/..") >= 0) || (file_path.Find("\\..") >= 0) ||
        NPT_FAILED(NPT_File::GetInfo(file_path, &file_info))) {
        return NPT_ERROR_NO_SUCH_ITEM;
    }
    
    // check for range requests
    const NPT_String* range_spec = request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_RANGE);
    
    // handle potential 304 only if range header not set
    NPT_DateTime  date;
    NPT_TimeStamp timestamp;
    if (NPT_SUCCEEDED(UPNPMessageHelper::GetIfModifiedSince((NPT_HttpMessage&)request, date)) &&
        !range_spec) {
        date.ToTimeStamp(timestamp);
        
        //NPT_LOG_INFO_5("File %s timestamps: request=%d (%s) vs file=%d (%s)", 
        //               (const char*)request.GetUrl().GetPath(),
        //               (NPT_UInt32)timestamp.ToSeconds(),
        //               (const char*)date.ToString(),
        //               (NPT_UInt32)file_info.m_ModificationTime,
        //               (const char*)NPT_DateTime(file_info.m_ModificationTime).ToString());
        
        if (timestamp >= file_info.m_ModificationTime) {
            // it's a match
            //NPT_LOG_FINE_1("Returning 304 for %s", request.GetUrl().GetPath().GetChars());
            //response.SetStatus(304, "Not Modified", NPT_HTTP_PROTOCOL_1_1);
            return NPT_SUCCESS;
        }
    }
    
    // open file
    if (NPT_FAILED(file.Open(NPT_FILE_OPEN_MODE_READ)) || 
        NPT_FAILED(file.GetInputStream(stream))        ||
        stream.IsNull()) {
        return NPT_ERROR_NO_SUCH_ITEM;
    }
    
    // set Last-Modified and Cache-Control headers
    if (file_info.m_ModificationTime) {
        NPT_DateTime last_modified = NPT_DateTime(file_info.m_ModificationTime);
        response.GetHeaders().SetHeader("Last-Modified", last_modified.ToString(NPT_DateTime::FORMAT_RFC_1123), true);
        response.GetHeaders().SetHeader("Cache-Control", "max-age=0,must-revalidate", true);
        //response.GetHeaders().SetHeader("Cache-Control", "max-age=1800", true);
    }
    
    //zhujw PLT_HttpRequestContext tmp_context(request, context);
    //zhujw return ServeStream(request, context, response, stream, PLT_MimeType::GetMimeType(file_path, &tmp_context));
	return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpServer::ServeStream
+---------------------------------------------------------------------*/
NPT_Result 
CHttpServer::ServeStream(const NPT_HttpRequest&        request, 
                            const NPT_HttpRequestContext& context,
                            NPT_HttpResponse&             response,
                            NPT_InputStreamReference&     body, 
                            const char*                   content_type) 
{    
    if (body.IsNull()) return NPT_FAILURE;
    
    // set date
    NPT_TimeStamp now;
    NPT_System::GetCurrentTimeStamp(now);
    response.GetHeaders().SetHeader("Date", NPT_DateTime(now).ToString(NPT_DateTime::FORMAT_RFC_1123), true);
    
    // get entity
    NPT_HttpEntity* entity = response.GetEntity();
    //NPT_CHECK_POINTER_FATAL(entity);
    
    // set the content type
    entity->SetContentType(content_type);
    
    // check for range requests
    const NPT_String* range_spec = request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_RANGE);
    
    // setup entity body
    NPT_CHECK(NPT_HttpFileRequestHandler::SetupResponseBody(response, body, range_spec));
              
    // set some default headers
    if (response.GetEntity()->GetTransferEncoding() != NPT_HTTP_TRANSFER_ENCODING_CHUNKED) {
        // set but don't replace Accept-Range header only if body is seekable
        NPT_Position offset;
        if (NPT_SUCCEEDED(body->Tell(offset)) && NPT_SUCCEEDED(body->Seek(offset))) {
            response.GetHeaders().SetHeader(NPT_HTTP_HEADER_ACCEPT_RANGES, "bytes", false); 
        }
    }
    
    // set getcontentFeatures.dlna.org
    const NPT_String* value = request.GetHeaders().GetHeaderValue("getcontentFeatures.dlna.org");
    if (value) {
        //zhujw PLT_HttpRequestContext tmp_context(request, context);
        //const char* dlna = PLT_ProtocolInfo::GetDlnaExtension(entity->GetContentType(),
        //                                                      &tmp_context);
        //if (dlna) response.GetHeaders().SetHeader("ContentFeatures.DLNA.ORG", dlna, false);
    }
    
    // transferMode.dlna.org
    value = request.GetHeaders().GetHeaderValue("transferMode.dlna.org");
    if (value) {
        // Interactive mode not supported?
        /*if (value->Compare("Interactive", true) == 0) {
		response.SetStatus(406, "Not Acceptable");
		return NPT_SUCCESS;
		}*/

		response.GetHeaders().SetHeader("TransferMode.DLNA.ORG", value->GetChars(), false);
	} else {
		response.GetHeaders().SetHeader("TransferMode.DLNA.ORG", "Streaming", false);
	}

	if (request.GetHeaders().GetHeaderValue("TimeSeekRange.dlna.org")) {
		response.SetStatus(406, "Not Acceptable");
		return NPT_SUCCESS;
	}

	return NPT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// CHttpListenTask::DoRun
void CHttpListenTask::DoRun()
{

}