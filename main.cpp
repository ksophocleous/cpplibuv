#include <cstdio>
#include <sstream>
#include <iostream>
#include <functional>
#include <string>
#include <cstdint>

#include <vector>
#include <uv.h>

class UVException
{
	public:
		UVException(std::string msg) : m_msg(std::move(msg))
		{
		}

		friend std::ostream& operator<< (std::ostream& out, const UVException &e)
		{
			out << e.m_msg;
			return out;
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

class IUVHandle
{
	public:
		virtual ~IUVHandle();
};

class UVTimer
{
	public:
		typedef std::function<void (UVTimer&)> TimerCallback;

		static void timer_cb(uv_timer_t* handle, int status)
		{
			if (status < 0)
				throw UVException("UVTimer callback received status (below zero)");

			UVTimer *pTimer = reinterpret_cast<UVTimer *>(handle->data);
			if (pTimer != nullptr)
				pTimer->m_customcb(*pTimer);
		}

		UVTimer(uv_loop_t *owningLooper, uint64_t firstInterval, uint64_t loopInterval, TimerCallback customcb) : 
			m_customcb(customcb)
		{
			m_timer.data = this;

			if (m_customcb == nullptr)
				throw UVException("Timer without a callback is not allowed");

			if (uv_timer_init(owningLooper, &m_timer) == -1)
				throw UVException("uv_timer_init failed");

			if (uv_timer_start(&m_timer, timer_cb, firstInterval, loopInterval) == -1)
				throw UVException("uv_timer_start failed");
		}

		UVTimer(UVTimer&& moved) : m_customcb(std::move(moved.m_customcb))
		{
			memcpy(&m_timer, &moved.m_timer, sizeof(m_timer));
			m_timer.data = this;
			moved.m_timer.data = nullptr;
			std::cout << "moving from object " << &moved << " to this " << this;
		}

		~UVTimer()
		{
			uv_timer_stop(&m_timer);
		}

	private:
		UVTimer(const UVTimer&);
		UVTimer& operator=(const UVTimer&);

		uv_timer_t m_timer;
		TimerCallback m_customcb;
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
		typedef std::function<void (UVAddress4Resolve &resolver, const std::string &ip, int status)> AddressResolvedCb;

		UVAddress4Resolve(uv_loop_t* loop, std::string hostname, uint16_t port, AddressResolvedCb cb, NetProtocol::Enum protocol = NetProtocol::Tcp) : 
			m_cb(cb), m_port(port), m_hostname(std::move(hostname))
		{
			std::cout << "Resolver :: IN" << std::endl;

			m_addrInfo.data = this;

			struct addrinfo hints;
			if (protocol == NetProtocol::Tcp)
			{
				hints.ai_family = PF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				hints.ai_flags = 0;
			}

			std::ostringstream oss; oss << port;
			int r = uv_getaddrinfo(loop, &m_addrInfo, dns_cb, m_hostname.c_str(), oss.str().c_str(), &hints);
			if (r != 0)
				throw UVException("uv_getaddrinfo: something went wrong");
		}

		uint16_t GetPort() const { return m_port; }
		std::string GetHostname() const { return m_hostname; }

		~UVAddress4Resolve()
		{
			std::cout << "Resolver :: OUT" << std::endl;
		}

	private:
		static void dns_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
		{
			UVAddress4Resolve *pAddr = reinterpret_cast<UVAddress4Resolve *>(req->data);
			
			if (pAddr)
			{
				AddressResolvedCb cb = pAddr->m_cb;

				if (status < 0)
					throw UVException("Resolve address X failed with status negative");

				char addr[17] = {'\0'};
				uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);

				if (cb)
					cb(*pAddr, addr, status);
			}

			uv_freeaddrinfo(res);
		}

		std::string m_hostname;
		uint16_t m_port;
		uv_getaddrinfo_t m_addrInfo;
		AddressResolvedCb m_cb;
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

		UVAddress4Resolve &ResolveAddress(std::string hostname, uint16_t port, std::function<void (UVAddress4Resolve &resolver, const std::string &ip, int status)> funcCb, NetProtocol::Enum protocol = NetProtocol::Tcp)
		{
			if (funcCb == nullptr)
				throw UVException("Resolving address with no callback - waste of time: consider removing it");

			auto intermediate = [this, funcCb] (UVAddress4Resolve &resolver, const std::string &ip, int status)
			{
				try
				{
					if (status < 0)
						funcCb(resolver, "", status);
					else
						funcCb(resolver, ip, status);
				}
				catch (UVException e)
				{
					std::cerr << "UVLoop::ResolveAddress(): " << e;
				}

				UVAddress4Resolve *pResolver = &resolver;
				delete pResolver;
			};

			return *(new UVAddress4Resolve(m_loop, std::move(hostname), port, intermediate, protocol));
		}

		UVTimer& NewTimer(uint64_t firstInterval, uint64_t loopInterval, std::function<bool ()> customcb)
		{
			if (customcb == nullptr)
				throw UVException("NewTimer must have a custom callback");

			auto intermediate = [this, customcb, loopInterval] (UVTimer &timer)
			{
				try
				{
					if (loopInterval != 0 && customcb() == true)
						return;
				}
				catch (UVException e)
				{
					std::cerr << e;
				}

				StopTimer(timer);
			};

			return *(new UVTimer(m_loop, firstInterval, loopInterval, intermediate));
		}

		void StopTimer(UVTimer& ptr)
		{
			UVTimer *pTimer = &ptr;
			delete pTimer;
		}

	protected:
		uv_loop_t *m_loop;
};





int main(int argc, char *argv[])
{
	std::cout << "testing" << std::endl;

	UVLoop looper;

	/////////////////////////////////////////////

	uint32_t timerCount = 0;
	looper.NewTimer(2000, 100, 
		[&timerCount] () -> bool
		{
			std::cout << "timer exec" << std::endl;
			timerCount++;
			return (timerCount < 10);
		});

	/////////////////////////////////////////////

	looper.ResolveAddress("www.google.com", 80, 
		[] (UVAddress4Resolve &resolver, const std::string &ip, int status)
		{
			std::cout << "Host " << resolver.GetHostname() << " resolved at ip " << ip << std::endl;
		});

	/////////////////////////////////////////////

	looper.Loop();

	return 0;
}
