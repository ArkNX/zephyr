/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_EVENT_QUEUE_COMPONENT_HPP__
#define __ZEPHYR_EVENT_QUEUE_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <future>
#include <queue>
#include <tuple>
#include <utility>
#include <string_view>

#include <zephyr/base/selector.hpp>
#include <zephyr/base/iopool.hpp>
#include <zephyr/base/error.hpp>

#include <zephyr/base/detail/util.hpp>
#include <zephyr/base/detail/function_traits.hpp>
#include <zephyr/base/detail/buffer_wrap.hpp>

namespace zephyr::detail
{
	template<class derived_t>
	class event_queue_cp
	{
	public:
		/**
		 * @constructor
		 */
		event_queue_cp() : derive(static_cast<derived_t&>(*this)) {}

		/**
		 * @destructor
		 */
		~event_queue_cp() = default;

	public:
		/**
		 * push a task to the tail of the event queue
		 * Callback signature : bool()
		 */
		template<class Callback>
		inline derived_t & push_event(Callback&& f)
		{
#if defined(ZEPHYR_SEND_CORE_ASYNC)
			// Make sure we run on the strand
			if (derive.io().strand().running_in_this_thread())
			{
				bool empty = this->events_.empty();
				this->events_.emplace(std::forward<Callback>(f));
				if (empty)
				{
					(this->events_.front())();
				}
				return (derive);
			}

			asio::post(derive.io().strand(), make_allocator(derive.wallocator(),
				[this, p = derive.selfptr(), f = std::forward<Callback>(f)]() mutable
			{
				bool empty = this->events_.empty();
				this->events_.emplace(std::move(f));
				if (empty)
				{
					(this->events_.front())();
				}
			}));

			return (derive);
#else
			// Make sure we run on the strand
			if (derive.io().strand().running_in_this_thread())
			{
				f();
				return (derive);
			}

			asio::post(derive.io().strand(), make_allocator(derive.wallocator(),
				[this, p = derive.selfptr(), f = std::forward<Callback>(f)]() mutable
			{
				f();
			}));

			return (derive);
#endif
		}

		/**
		 * Removes an element from the front of the event queue.
		 * and then execute the next element of the queue.
		 */
		template<typename = void>
		inline derived_t & next_event()
		{
#if defined(ZEPHYR_SEND_CORE_ASYNC)
			// Make sure we run on the strand
			if (derive.io().strand().running_in_this_thread())
			{
				if (!this->events_.empty())
				{
					this->events_.pop();

					if (!this->events_.empty())
					{
						(this->events_.front())();
					}
				}
				return (derive);
			}

			asio::post(derive.io().strand(), make_allocator(derive.wallocator(),
				[this, p = derive.selfptr()]() mutable
			{
				if (!this->events_.empty())
				{
					this->events_.pop();

					if (!this->events_.empty())
					{
						(this->events_.front())();
					}
				}
			}));

			return (derive);
#else
			ZEPHYR_ASSERT(false);
			return (derive);
#endif
		}

	protected:
		derived_t                         & derive;

		std::queue<std::function<bool()>>   events_;
	};
}

#endif // !__ZEPHYR_EVENT_QUEUE_COMPONENT_HPP__
