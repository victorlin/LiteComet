// ------------------------------------------------------------------
// pion-net: a C++ framework for building lightweight HTTP interfaces
// ------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_COOKIESERVICE_HEADER__
#define __PION_COOKIESERVICE_HEADER__

#include <pion/net/WebService.hpp>


namespace pion {		// begin namespace pion
namespace plugins {		// begin namespace plugins

///
/// CookieService: web service that displays and updates cookies
/// 
class CookieService :
	public pion::net::WebService
{
public:
	CookieService(void) {}
	virtual ~CookieService() {}
	virtual void operator()(pion::net::HTTPRequestPtr& request,
							pion::net::TCPConnectionPtr& tcp_conn);
};

}	// end namespace plugins
}	// end namespace pion
	
#endif
