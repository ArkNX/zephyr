/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#if !defined(ASIO_STANDALONE) && defined(ZEPHYR_USE_SSL)

#ifndef __ZEPHYR_HTTPS_SESSION_HPP__
#define __ZEPHYR_HTTPS_SESSION_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <zephyr/http/http_session.hpp>
#include <zephyr/tcp/component/ssl_stream_cp.hpp>

namespace zephyr::detail
{
	template <class>                             class session_mgr_t;
	template <class, class>                      class tcp_server_impl_t;
	template <class, class>                      class https_server_impl_t;

	template<class derived_t, class socket_t, class body_t, class buffer_t>
	class https_session_impl_t
		: public http_session_impl_t<derived_t, socket_t, body_t, buffer_t>
		, public ssl_stream_cp<derived_t, socket_t, true>
	{
		template <class, bool>                       friend class user_timer_cp;
		template <class>                             friend class post_cp;
		template <class>                             friend class data_persistence_cp;
		template <class>                             friend class event_queue_cp;
		template <class, bool>                       friend class send_cp;
		template <class, bool>                       friend class silence_timer_cp;
		template <class, bool>                       friend class connect_timeout_cp;
		template <class, bool>                       friend class tcp_send_op;
		template <class, bool>                       friend class tcp_recv_op;
		template <class, class, class, bool>         friend class http_send_cp;
		template <class, class, class, bool>         friend class http_send_op;
		template <class, class, class, bool>         friend class http_recv_op;
		template <class, class, bool>                friend class ssl_stream_cp;
		template <class>                             friend class session_mgr_t;
		template <class, class, class>               friend class session_impl_t;
		template <class, class, class>               friend class tcp_session_impl_t;
		template <class, class, class, class>        friend class http_session_impl_t;
		template <class, class>                      friend class tcp_server_impl_t;
		template <class, class>                      friend class https_server_impl_t;

	public:
		using self = https_session_impl_t<derived_t, socket_t, body_t, buffer_t>;
		using super = http_session_impl_t<derived_t, socket_t, body_t, buffer_t>;
		using key_type = std::size_t;
		using body_type = body_t;
		using buffer_type = buffer_t;
		using ssl_stream_comp = ssl_stream_cp<derived_t, socket_t, true>;
		using super::send;

		/**
		 * @constructor
		 */
		explicit https_session_impl_t(
			asio::ssl::context & ctx,
			session_mgr_t<derived_t> & sessions,
			listener_t & listener,
			io_t & rwio,
			std::size_t init_buffer_size,
			std::size_t max_buffer_size,
            std::size_t silence_timeout
		)
			: super(sessions, listener, rwio, init_buffer_size, max_buffer_size, silence_timeout)
			, ssl_stream_comp(this->io_, ctx, asio::ssl::stream_base::server)
			, ctx_(ctx)
		{
		}

		/**
		 * @destructor
		 */
		~https_session_impl_t()
		{
		}

		/**
		 * @function : get the stream object refrence
		 */
		inline typename ssl_stream_comp::stream_type & stream()
		{
			ZEPHYR_ASSERT(bool(this->ssl_stream_));
			return (*(this->ssl_stream_));
		}

	public:
		/**
		 * @function : get this object hash key,used for session map
		 */
		inline const key_type hash_key() const
		{
			return reinterpret_cast<key_type>(this);
		}

	protected:
		template<typename MatchCondition>
		inline void _do_init(std::shared_ptr<derived_t> this_ptr, condition_wrap<MatchCondition> condition)
		{
			super::_do_init(std::move(this_ptr), condition);

			this->derived()._ssl_init(condition, this->socket_, this->ctx_);
		}

		template<typename MatchCondition>
		inline void _handle_connect(const error_code& ec, std::shared_ptr<derived_t> this_ptr, condition_wrap<MatchCondition> condition)
		{
			asio::post(this->io_.strand(), make_allocator(this->wallocator_,
				[this, self_ptr = std::move(this_ptr), condition]()
			{
				this->derived()._ssl_start(self_ptr, condition, this->socket_, this->ctx_);

				this->derived()._post_handshake(std::move(self_ptr), std::move(condition));
			}));
		}

		inline void _handle_disconnect(const error_code& ec, std::shared_ptr<derived_t> this_ptr)
		{
			this->derived()._ssl_stop(this_ptr, [this, ec, this_ptr]()
			{
				super::_handle_disconnect(ec, std::move(this_ptr));
			});
		}

		inline void _fire_handshake(std::shared_ptr<derived_t>& this_ptr, error_code ec)
		{
			this->listener_.notify(event::handshake, this_ptr, ec);
		}

	protected:
		asio::ssl::context & ctx_;
	};
}

namespace zephyr
{
	class https_session : public detail::https_session_impl_t<https_session, asio::ip::tcp::socket, http::string_body, beast::flat_buffer>
	{
	public:
		using https_session_impl_t<https_session, asio::ip::tcp::socket, http::string_body, beast::flat_buffer>::https_session_impl_t;
	};
}

#endif // !__ZEPHYR_HTTPS_SESSION_HPP__

#endif
