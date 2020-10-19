/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_LOCAL_ENDPOINT_COMPONENT_HPP__
#define __ZEPHYR_LOCAL_ENDPOINT_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <zephyr/base/selector.hpp>

namespace zephyr::detail
{
	template<class derived_t, class endpoint_type>
	class local_endpoint_cp
	{
	public:
		/**
		 * @constructor
		 */
		local_endpoint_cp() : derive(static_cast<derived_t&>(*this)), local_endpoint_() {}

		/**
		 * @destructor
		 */
		~local_endpoint_cp() = default;

	public:
		/**
		 * Construct an endpoint using a port number, specified in the host's byte
		 * order. The IP address will be the any address (i.e. INADDR_ANY or
		 * in6addr_any).
		 * To initialise an IPv4 TCP endpoint for port 1234, use:
		 * asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), 1234);
		 * To specify an IPv6 UDP endpoint for port 9876, use:
		 * asio::ip::udp::endpoint ep(asio::ip::udp::v6(), 9876);
		 */
		template<typename InternetProtocol>
		inline derived_t & local_endpoint(const InternetProtocol& internet_protocol, unsigned short port_num)
		{
			this->local_endpoint_ = endpoint_type(internet_protocol, port_num);
			return (derive);
		}

		/**
		 * Construct an endpoint using a port number and an IP address. This
		 * function may be used for accepting connections on a specific interface
		 * or for making a connection to a remote endpoint.
		 * To initialise an IPv4 TCP endpoint for port 1234, use:
		 * asio::ip::tcp::endpoint ep(asio::ip::address_v4::from_string("..."), 1234);
		 * To specify an IPv6 UDP endpoint for port 9876, use:
		 * asio::ip::udp::endpoint ep(asio::ip::address_v6::from_string("..."), 9876);
		 */
		inline derived_t & local_endpoint(const asio::ip::address& addr, unsigned short port_num)
		{
			this->local_endpoint_ = endpoint_type(addr, port_num);
			return (derive);
		}

		/**
		 * get the binded local endpoint refrence
		 */
		inline endpoint_type& local_endpoint() { return this->local_endpoint_; }

	protected:
		derived_t   & derive;

		/// local bind endpoint
		endpoint_type local_endpoint_;
	};
}

#endif // !__ZEPHYR_LOCAL_ENDPOINT_COMPONENT_HPP__
