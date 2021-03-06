/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_UDP_SEND_OP_HPP__
#define __ZEPHYR_UDP_SEND_OP_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <future>
#include <utility>
#include <string_view>

#include <zephyr/base/selector.hpp>
#include <zephyr/base/error.hpp>
#include <zephyr/base/detail/condition_wrap.hpp>

namespace zephyr::detail
{
	template<class derived_t, bool isSession>
	class udp_send_op
	{
	public:
		/**
		 * @constructor
		 */
		udp_send_op() : derive(static_cast<derived_t&>(*this)) {}

		/**
		 * @destructor
		 */
		~udp_send_op() = default;

	protected:
		template<class Data, class Callback>
		inline bool _udp_send(Data& data, Callback&& callback)
		{
#if defined(ZEPHYR_SEND_CORE_ASYNC)
			derive.stream().async_send(asio::buffer(data), asio::bind_executor(derive.io().strand(),
				make_allocator(derive.wallocator(),
					[this, p = derive.selfptr(), callback = std::forward<Callback>(callback)]
			(const error_code& ec, std::size_t bytes_sent) mutable
			{
				set_last_error(ec);

				callback(ec, bytes_sent);

				derive.next_event();
			})));
			return true;
#else
			error_code ec;
			std::size_t bytes_sent = derive.stream().send(asio::buffer(data), 0, ec);
			set_last_error(ec);
			callback(ec, bytes_sent);
			return (!bool(ec));
#endif
		}

		template<class Endpoint, class Data, class Callback>
		inline bool _udp_send_to(Endpoint& endpoint, Data& data, Callback&& callback)
		{
#if defined(ZEPHYR_SEND_CORE_ASYNC)
			derive.stream().async_send_to(asio::buffer(data), endpoint, asio::bind_executor(derive.io().strand(),
				make_allocator(derive.wallocator(),
					[this, p = derive.selfptr(), callback = std::forward<Callback>(callback)]
			(const error_code& ec, std::size_t bytes_sent) mutable
			{
				set_last_error(ec);

				callback(ec, bytes_sent);

				derive.next_event();
			})));
			return true;
#else
			error_code ec;
			std::size_t bytes_sent = derive.stream().send_to(asio::buffer(data), endpoint, 0, ec);
			set_last_error(ec);
			callback(ec, bytes_sent);
			return (!bool(ec));
#endif
		}

	protected:
		derived_t & derive;
	};
}

#endif // !__ZEPHYR_UDP_SEND_OP_HPP__
