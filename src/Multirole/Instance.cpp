#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <fstream>
#include <future>

#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole {

nlohmann::json LoadConfigJson(std::string_view path)
{
	fmt::print("Loading up config.json...\n");
	std::ifstream i(path.data());
	return nlohmann::json::parse(i);
}

// public

Instance::Instance() :
	lIoCtx(),
	whIoCtx(),
	logger(),
	cfg(LoadConfigJson("config.json")),
	lobby(),
	lobbyListing(lIoCtx, cfg.at("lobbyListingPort").get<unsigned short>(), lobby),
	roomHosting(lIoCtx, cfg.at("roomHostingPort").get<unsigned short>(), lobby),
	signalSet(lIoCtx)
{
	for(const auto& repoOpts : cfg.at("repos").get<std::vector<nlohmann::json>>())
	{
		std::string name = repoOpts.at("name").get<std::string>();
		fmt::print(FMT_STRING("Adding repository '{:s}'...\n"), name);
		repos.emplace(std::piecewise_construct, std::forward_as_tuple(name),
		              std::forward_as_tuple(whIoCtx, logger, repoOpts));
	}

	fmt::print("Setting up signal handling...\n");
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](const std::error_code&, int sigNum)
	{
		const char* sigName;
		switch(sigNum)
		{
			case SIGINT: sigName = "SIGINT"; break;
			case SIGTERM: sigName = "SIGTERM"; break;
			default: sigName = "Unknown signal"; break;
		}
		logger.Log(fmt::format(FMT_STRING("{:s} received."), sigName));
		Stop();
	});
}

int Instance::Run()
{
	std::future<std::size_t> wsHExec = std::async(std::launch::async,
	[this]()
	{
		return whIoCtx.run();
	});
	// Next run call will only return after all connections are properly closed
	std::size_t tHExec = lIoCtx.run();
	tHExec += wsHExec.get();
	fmt::print("All Contexts stopped. Total handlers executed: {}\n", tHExec);
	return EXIT_SUCCESS;
}

// private

constexpr const char* UNFINISHED_DUELS_STRING =
R"(All done, server will gracefully finish execution
after all duels finish. If you wish to forcefully end
you can terminate the process safely now (SIGKILL)
)";

void Instance::Stop()
{
	fmt::print("Closing all acceptors and finishing IO operations...\n");
	whIoCtx.stop(); // Terminates thread
	lobbyListing.Stop();
	roomHosting.Stop();
	const auto startedRoomsCount = lobby.GetStartedRoomsCount();
	lobby.CloseNonStartedRooms();
	if(startedRoomsCount > 0u)
	{
		fmt::print(UNFINISHED_DUELS_STRING);
		fmt::print(FMT_STRING("Remaining rooms: {:d}\n"), startedRoomsCount);
	}
}

} // namespace Multirole

} // namespace Ignis
