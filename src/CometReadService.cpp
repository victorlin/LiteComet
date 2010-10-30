#include <boost/bind.hpp>
#include <pion/net/HTTPResponseWriter.hpp>
#include <pion/net/PionUser.hpp>

#include "CometReadService.hpp"

using namespace boost;
using namespace pion;
using namespace pion::net;

namespace lite_comet {

// CometReadService member functions

void CometReadService::notifyChannel(
    HTTPResponseWriterWeakPtr weak_writer, 
    ChannelPtr channel
) {
    PION_LOG_DEBUG(m_logger, "Notify channel test");
    HTTPResponseWriterPtr writer(weak_writer);
    writer->write(channel->getData(0));
    writer->send(bind(&TCPConnection::finish, writer->getTCPConnection()));

    m_writers.erase(writer);
}

/// handles requests for EchoService
void CometReadService::operator()(
    HTTPRequestPtr& request, 
    TCPConnectionPtr& tcp_conn
) {
    HTTPResponseWriterPtr writer(
        HTTPResponseWriter::create(
            tcp_conn, 
            *request
        )
    );
    // We don't want Keep-Alive here
    tcp_conn->setLifecycle(TCPConnection::LIFECYCLE_CLOSE);
    // Set the content type as text
    writer->getResponse().setContentType(HTTPTypes::CONTENT_TYPE_TEXT);
    // Do not send content length, because it is unknown now
    writer->getResponse().setDoNotSendContentLength();
    // Send the header
    writer->send();

    // Keep the reference
    m_writers.insert(writer);

    PION_LOG_DEBUG(m_logger, "New request is waitting for channel test");
    ChannelPtr channel = m_channel_manager.getChannel("test");
    channel->addListener(
        bind(&CometReadService::notifyChannel, this, 
             HTTPResponseWriterWeakPtr(writer), channel));
}


}   // end namespace lite_comet

