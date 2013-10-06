#include <cstdio>
#include <sstream>
#include <iostream>
#include <functional>
#include <string>
#include <cstdint>

#include <uv.h>

class UVException
{
	public:
		UVException(std::string msg) : m_msg(std::move(msg))
		{
		}

	private:
		std::string m_msg;
};

class IUVLoop
{
	public:
		virtual ~IUVLoop() { }
		virtual void Loop() = 0;
		virtual void Shutdown() = 0;
};

class UVLoop : public IUVLoop
{
	public:
		UVLoop()
		{
			m_loop = uv_loop_new();
			if (m_loop == nullptr)
				throw UVException("uv_loop_new failed");
		}

		~UVLoop()
		{
			uv_loop_delete(m_loop);
		}

		void Loop()
		{
			uv_run(m_loop, UV_RUN_DEFAULT);
		}

		void Shutdown()
		{
			uv_stop(m_loop);
		}

		uv_loop_t* GetInternalPtr() const { return m_loop; }

	protected:
		uv_loop_t *m_loop;
};

class UVTimer
{
	public:
		static void timer_cb(uv_timer_t* handle, int status)
		{
			UVTimer *pTimer = reinterpret_cast<UVTimer *>(handle->data);
			if (pTimer != nullptr)
				pTimer->HandleCallback(status);
		}

		UVTimer(IUVLoop& loop, uint64_t firstInterval, uint64_t loopInterval = 0, std::function<void ()> customcb = nullptr) : m_customcb(customcb)
		{
			UVLoop &concrete = (UVLoop &)loop;
			uv_timer_init(concrete.GetInternalPtr(), &m_timer);
			m_timer.data = this;
			uv_timer_start(&m_timer, timer_cb, firstInterval, loopInterval);
		}

		~UVTimer()
		{
			uv_timer_stop(&m_timer);
		}

	private:
		void HandleCallback(int status)
		{
			if (m_customcb)
				m_customcb();
			std::cout << "timer callback" << std::endl;
		}

		uv_timer_s m_timer;
		std::function<void ()> m_customcb;
};

namespace NetProtocol
{
	enum Enum
	{
		Tcp,
		Udp
	};
};

class UVAddress4Resolve
{
	public:
		UVAddress4Resolve(IUVLoop& loop, const std::string &hostname, uint16_t port = 0, NetProtocol::Enum protocol = NetProtocol::Tcp)
		{
			m_addrInfo.data = this;

			struct addrinfo hints;
			if (protocol == NetProtocol::Tcp)
			{
				hints.ai_family = PF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				hints.ai_flags = 0;
			}

			UVLoop &privateLoop = (UVLoop &)loop;
			std::ostringstream oss; oss << port;
			int r = uv_getaddrinfo(privateLoop.GetInternalPtr(), &m_addrInfo, dns_cb, hostname.c_str(), oss.str().c_str(), &hints);
			if (r != 0)
				throw UVException("uv_getaddrinfo: something went wrong");
		}

		~UVAddress4Resolve()
		{
			//m_addrInfo.data = nullptr;
		}

	private:
		static void dns_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
		{
			UVAddress4Resolve *pAddr = reinterpret_cast<UVAddress4Resolve *>(req->data);
			
			if (pAddr)
				pAddr->HandleCallback(status, res);

			uv_freeaddrinfo(res);
		}

		void HandleCallback(int status, struct addrinfo* res)
		{
			if (status == -1)
			{
				// error
				return;
			}

			char addr[17] = {'\0'};
			uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
			std::cout << "Address: " << addr << std::endl;
		}

		uv_getaddrinfo_t m_addrInfo;
};

int main(int argc, char *argv[])
{
	std::cout << "testing" << std::endl;

	UVLoop looper;
	UVTimer timer1(looper, 2000, 100);
	UVAddress4Resolve test(looper, "www.google.com");

	looper.Loop();

	return 0;
}
