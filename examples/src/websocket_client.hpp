#pragma once

#include <zephyr/zephyr.hpp>

void run_ws_client(std::string_view host, std::string_view port)
{
#ifndef ASIO_STANDALONE

	zephyr::ws_client client;
	
	client.connect_timeout(std::chrono::seconds(10));
	client.bind_init([&]()
	{
		client.ws_stream().set_option(websocket::stream_base::decorator(
			[](websocket::request_type& req)
		{
			req.set(http::field::authorization," websocket-client-authorization");
		}));
	}).bind_connect([&](asio::error_code ec)
	{
		if (zephyr::get_last_error())
			printf("connect failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
		else
			printf("connect success : %s %u\n", client.local_address().c_str(), client.local_port());
		std::string s;
		s += '<';
		int len = 128 + std::rand() % 512;
		for (int i = 0; i < len; i++)
		{
			s += (char)((std::rand() % 26) + 'a');
		}
		s += '>';
		client.send(std::move(s), [](std::size_t bytes_sent) {});
	}).bind_upgrade([&](asio::error_code ec)
	{
		std::cout << "upgrade " << (ec ? "failure : " : "success : ") << ec.value() << " "
			<< ec.message() << std::endl << client.upgrade_response() << std::endl;
		client.send("abc"); // this send will failed, because connection is not fully completed
	}).bind_disconnect([](asio::error_code ec)
	{
		printf("disconnect : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
	}).bind_recv([&](std::string_view sv)
	{
		printf("recv : %u %.*s\n", (unsigned)sv.size(), (int)sv.size(), sv.data());

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		client.send(sv, []() {});
	});

	if (!client.start(host, port))
	{
		printf("start failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
	}
	//client.async_start(host, port);
	client.post([]() {});
	while (std::getchar() != '\n');
	//std::this_thread::sleep_for(std::chrono::seconds(20));

	client.stop();
#endif
}


void run_wss_client(std::string_view host, std::string_view port)
{
#if !defined(ASIO_STANDALONE) && defined(ZEPHYR_USE_SSL)
	//while (1)
	{
		zephyr::wss_client client;
		client.connect_timeout(std::chrono::seconds(300));
		client.set_verify_mode(asio::ssl::verify_peer);
		//client.set_cert_buffer(ca_crt, client_crt, client_key, "client");
		client.set_cert_file("ca.crt", "client.crt", "client.key", "client");
		client.bind_init([&]()
		{
			client.ws_stream().set_option(websocket::stream_base::decorator(
				[](websocket::request_type& req)
			{
				req.set(http::field::authorization, " websocket-client-coro");
			}));
		}).bind_recv([&](std::string_view s)
		{
			printf("recv : %u %.*s\n", (unsigned)s.size(), (int)s.size(), s.data());
			client.send(s);
		}).bind_connect([&](asio::error_code ec)
		{
			if (zephyr::get_last_error())
				printf("connect failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
			else
				printf("connect success : %s %u\n", client.local_address().c_str(), client.local_port());

			std::string s;
			s += '<';
			int len = 128 + std::rand() % (300);
			for (int i = 0; i < len; i++)
			{
				s += (char)((std::rand() % 26) + 'a');
			}
			s += '>';

			client.send(std::move(s), [](std::size_t bytes_sent) {});
		}).bind_disconnect([](asio::error_code ec)
		{
			printf("disconnect : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
		}).bind_handshake([&](asio::error_code ec)
		{
			printf("handshake : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
		}).bind_upgrade([&](asio::error_code ec)
		{
			std::cout << "upgrade " << (ec ? "failure : " : "success : ") << ec.value() << " "
				<< ec.message() << std::endl << client.upgrade_response() << std::endl;
		});
		client.async_start(host, port);
		while (std::getchar() != '\n');
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		//client.stop();
	}
#endif // ZEPHYR_USE_SSL
}

