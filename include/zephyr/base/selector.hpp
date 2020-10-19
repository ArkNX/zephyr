/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_SELECTOR_HPP__
#define __ZEPHYR_SELECTOR_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <zephyr/config.hpp>

#ifdef ASIO_STANDALONE
	#include <asio.hpp>
	#if defined(ZEPHYR_USE_SSL)
		#include <asio/ssl.hpp>
	#endif
#else
	#include <boost/asio.hpp>
	#include <boost/beast.hpp>
	#if defined(ZEPHYR_USE_SSL)
		#include <boost/asio/ssl.hpp>
		// boost 1.72(107200) BOOST_BEAST_VERSION 277
		#if defined(BOOST_BEAST_VERSION) && (BOOST_BEAST_VERSION >= 277)
			#include <boost/beast/ssl.hpp>
		#endif
		#include <boost/beast/websocket/ssl.hpp>
	#endif
	#ifndef ASIO_VERSION
		#define ASIO_VERSION BOOST_ASIO_VERSION
	#endif
#endif // ASIO_STANDALONE


#ifdef ASIO_STANDALONE
	//namespace asio = ::asio;
#else
	namespace boost::asio
	{
		using error_code = ::boost::system::error_code;
	}
	namespace asio = ::boost::asio;
	namespace beast = ::boost::beast;
	namespace http = ::boost::beast::http;
	namespace websocket = ::boost::beast::websocket;
#endif // ASIO_STANDALONE

namespace zephyr
{
#ifdef ASIO_STANDALONE
	using error_code = ::asio::error_code;
	using system_error = ::asio::system_error;
#else
	using error_code = ::boost::system::error_code;
	using system_error = ::boost::system::system_error;

	namespace http = ::boost::beast::http;
	namespace websocket = ::boost::beast::websocket;
#endif // ASIO_STANDALONE
}

#define ZEPHYR_SEND_CORE_ASYNC

#endif // !__ZEPHYR_SELECTOR_HPP__
