/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#if defined(ZEPHYR_USE_SSL)

#ifndef __ZEPHYR_SSL_STREAM_COMPONENT_HPP__
#define __ZEPHYR_SSL_STREAM_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <future>
#include <utility>
#include <string_view>

#include <zephyr/base/selector.hpp>
#include <zephyr/base/error.hpp>
#include <zephyr/base/iopool.hpp>

#include <zephyr/base/detail/allocator.hpp>
#include <zephyr/base/detail/condition_wrap.hpp>

namespace zephyr::detail
{
	template<class derived_t, class socket_t, bool isSession>
	class ssl_stream_cp
	{
		using handshake_type = typename asio::ssl::stream_base::handshake_type;

	public:
		using stream_type = asio::ssl::stream<socket_t&>;

		ssl_stream_cp(io_t& ssl_io, asio::ssl::context& ctx, handshake_type type)
			: derive(static_cast<derived_t&>(*this))
			, ssl_io_(ssl_io)
			, ssl_ctx_(ctx)
			, ssl_timer_(ssl_io.context())
			, ssl_type_(type)
		{
		}

		~ssl_stream_cp() = default;

		inline stream_type & ssl_stream()
		{
			ZEPHYR_ASSERT(bool(this->ssl_stream_));
			return (*(this->ssl_stream_));
		}

	protected:
		template<typename MatchCondition>
		inline void _ssl_init(
			const condition_wrap<MatchCondition>& condition,
			socket_t& socket, asio::ssl::context& ctx)
		{
			detail::ignore::unused(condition, socket, ctx);

			// Why put the initialization code of ssl stream here ?
			// Why not put it in the constructor ?
			// -----------------------------------------------------------------------
			// Beacuse SSL_CTX_use_certificate_chain_file,SSL_CTX_use_PrivateKey and
			// other SSL_CTX_... functions must be called before SSL_new, otherwise,
			// those SSL_CTX_... function calls have no effect.
			// When construct a tcps_client object, beacuse the tcps_client is derived
			// from ssl_stream_cp, so the ssl_stream_cp's constructor will be called,
			// but at this time, the SSL_CTX_... function has not been called.
			this->ssl_stream_ = std::make_unique<stream_type>(socket, ctx);
		}

		template<typename MatchCondition>
		inline void _ssl_start(
			const std::shared_ptr<derived_t>& this_ptr,
			const condition_wrap<MatchCondition>& condition,
			socket_t& socket, asio::ssl::context& ctx)
		{
			detail::ignore::unused(this_ptr, condition, socket, ctx);
		}

		template<typename Fn>
		inline void _ssl_stop(std::shared_ptr<derived_t> this_ptr, Fn&& fn)
		{
			if (!this->ssl_stream_)
				return;

			// if the client socket is not closed forever,this async_shutdown callback also can't be called forever,
			// so we use a timer to force close the socket,then the async_shutdown callback will be called.
			this->ssl_timer_.expires_after(std::chrono::milliseconds(ssl_shutdown_timeout));
			this->ssl_timer_.async_wait(asio::bind_executor(this->ssl_io_.strand(),
				[this, this_ptr, f = std::forward<Fn>(fn)](const error_code & ec) mutable
			{
				set_last_error(ec);

				(f)();
			}));

			// when server call ssl stream sync shutdown first,if the client socket is
			// not closed forever,then here shutdowm will blocking forever.
			this->ssl_stream_->async_shutdown(asio::bind_executor(this->ssl_io_.strand(),
				[this, self_ptr = std::move(this_ptr)](const error_code & ec) mutable
			{
				set_last_error(ec);

				// clost the ssl timer
				this->ssl_timer_.cancel();

				// SSL_clear : 
				// Reset ssl to allow another connection. All settings (method, ciphers, BIOs) are kept.

				// When the client auto reconnect, SSL_clear must be called, otherwise the SSL handshake will failed.
				SSL_clear(this->ssl_stream_->native_handle());
			}));
		}

		template<typename MatchCondition>
		inline void _post_handshake(std::shared_ptr<derived_t> this_ptr, condition_wrap<MatchCondition> condition)
		{
			ZEPHYR_ASSERT(bool(this->ssl_stream_));
			this->ssl_stream_->async_handshake(this->ssl_type_,
				asio::bind_executor(this->ssl_io_.strand(), make_allocator(derive.rallocator(),
					[this, self_ptr = std::move(this_ptr), condition](const error_code & ec)
			{
				derive._handle_handshake(ec, std::move(self_ptr), std::move(condition));
			})));
		}

		template<typename MatchCondition>
		inline void _handle_handshake(const error_code & ec, std::shared_ptr<derived_t> self_ptr, condition_wrap<MatchCondition> condition)
		{
			if constexpr (isSession)
			{
				derive.sessions().post([this, ec, this_ptr = std::move(self_ptr), condition]() mutable
				{
					try
					{
						set_last_error(ec);

						derive._fire_handshake(this_ptr, ec);

						asio::detail::throw_error(ec);

						derive._done_connect(ec, std::move(this_ptr), std::move(condition));
					}
					catch (system_error & e)
					{
						set_last_error(e);
						derive._do_disconnect(e.code());
					}
				});
			}
			else
			{
				set_last_error(ec);

				derive._fire_handshake(self_ptr, ec);

				derive._done_connect(ec, std::move(self_ptr), std::move(condition));
			}
		}

	protected:
		derived_t                    & derive;

		io_t                         & ssl_io_;

		asio::ssl::context           & ssl_ctx_;

		/// timer for close ssl timeout
		asio::steady_timer             ssl_timer_;

		handshake_type                 ssl_type_;

		std::unique_ptr<stream_type>   ssl_stream_;
	};
}

#endif // !__ZEPHYR_SSL_STREAM_COMPONENT_HPP__

#endif
