/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_USER_TIMER_COMPONENT_HPP__
#define __ZEPHYR_USER_TIMER_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>

#include <zephyr/base/selector.hpp>
#include <zephyr/base/iopool.hpp>
#include <zephyr/base/error.hpp>

#include <zephyr/base/detail/allocator.hpp>

namespace zephyr::detail
{
#ifndef ZEPHYR_USER_TIMER_ID_TYPE
#define ZEPHYR_USER_TIMER_ID_TYPE std::size_t
#endif

	struct user_timer_obj
	{
		ZEPHYR_USER_TIMER_ID_TYPE id;
		asio::steady_timer timer;
		std::function<void()> task;

		user_timer_obj(ZEPHYR_USER_TIMER_ID_TYPE Id, asio::io_context & context, std::function<void()> t)
			: id(Id), timer(context), task(std::move(t)) {}
	};

	template<class derived_t, bool isSession>
	class user_timer_cp
	{
	public:
		/**
		 * @constructor
		 */
		explicit user_timer_cp(io_t & timer_io) : derive(static_cast<derived_t&>(*this)), user_timer_io_(timer_io)
		{
		}

		/**
		 * @destructor
		 */
		~user_timer_cp()
		{
		}

	public:
		template<class Rep, class Period, class Fun, class... Args>
		inline void start_timer(ZEPHYR_USER_TIMER_ID_TYPE timer_id, std::chrono::duration<Rep, Period> duration, Fun&& fun, Args&&... args)
		{
			std::function<void()> t = std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...);

			auto fn = [this, this_ptr = derive.selfptr(), timer_id, duration, task = std::move(t)]()
			{
				std::shared_ptr<user_timer_obj> timer_obj_ptr;

				auto iter = this->user_timers_.find(timer_id);
				if (iter != this->user_timers_.end())
				{
					timer_obj_ptr = iter->second;
					timer_obj_ptr->task = std::move(task);
				}
				else
				{
					timer_obj_ptr = std::make_shared<user_timer_obj>(timer_id, this->user_timer_io_.context(), std::move(task));

					this->user_timers_[timer_id] = timer_obj_ptr;
				}

				derive._post_user_timers(std::move(timer_obj_ptr), duration, std::move(this_ptr));
			};

			// Make sure we run on the strand
			if (!this->user_timer_io_.strand().running_in_this_thread())
				asio::post(this->user_timer_io_.strand(), make_allocator(derive.wallocator(), std::move(fn)));
			else
				fn();
		}

		inline void stop_timer(ZEPHYR_USER_TIMER_ID_TYPE timer_id)
		{
			// Make sure we run on the strand
			if (!this->user_timer_io_.strand().running_in_this_thread())
				return asio::post(this->user_timer_io_.strand(), make_allocator(derive.wallocator(),
					[this, this_ptr = derive.selfptr(), timer_id]()
			{
				this->stop_timer(timer_id);
			}));

			auto iter = this->user_timers_.find(timer_id);
			if (iter != this->user_timers_.end())
			{
				iter->second->timer.cancel();
				this->user_timers_.erase(iter);
			}
		}

		/**
		 * @function : stop session
		 * note : this function must be noblocking,if it's blocking,will cause circle lock in session_mgr::stop function
		 */
		inline void stop_all_timers()
		{
			if (!this->user_timer_io_.strand().running_in_this_thread())
				return asio::post(this->user_timer_io_.strand(), make_allocator(derive.wallocator(),
					[this, this_ptr = derive.selfptr()]()
			{
				this->stop_all_timers();
			}));

			// close user custom timers
			for (auto &[id, timer_obj_ptr] : this->user_timers_)
			{
				std::ignore = id;
				timer_obj_ptr->timer.cancel();
			}
			this->user_timers_.clear();
		}

	protected:
		template<class Rep, class Period>
		inline void _post_user_timers(std::shared_ptr<user_timer_obj> timer_obj_ptr,
			std::chrono::duration<Rep, Period> duration, std::shared_ptr<derived_t> this_ptr)
		{
			// must detect whether the timer is still exists, in some cases, after erase and
			// cancel a timer, the steady_timer is still exist
			auto iter = this->user_timers_.find(timer_obj_ptr->id);
			if (iter == this->user_timers_.end())
				return;

			asio::steady_timer& timer = timer_obj_ptr->timer;

			timer.expires_after(duration);
			timer.async_wait(asio::bind_executor(this->user_timer_io_.strand(),
				make_allocator(derive.wallocator(), [this, timer_ptr = std::move(timer_obj_ptr), duration,
					self_ptr = std::move(this_ptr)](const error_code & ec)
			{
				derive._handle_user_timers(ec, std::move(timer_ptr), duration, std::move(self_ptr));
			})));
		}

		template<class Rep, class Period>
		inline void _handle_user_timers(const error_code & ec, std::shared_ptr<user_timer_obj> timer_obj_ptr,
			std::chrono::duration<Rep, Period> duration, std::shared_ptr<derived_t> this_ptr)
		{
			set_last_error(ec);

			if (!ec)
				(timer_obj_ptr->task)();

			if (ec == asio::error::operation_aborted)
				return;

			derive._post_user_timers(std::move(timer_obj_ptr), duration, std::move(this_ptr));
		}

	protected:
		derived_t                                     & derive;

		/// The io (include io_context and strand) 
		io_t                                          & user_timer_io_;

		/// user-defined timer
		std::unordered_map<ZEPHYR_USER_TIMER_ID_TYPE, std::shared_ptr<user_timer_obj>> user_timers_;
	};
}

#endif // !__ZEPHYR_USER_TIMER_COMPONENT_HPP__
