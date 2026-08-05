#pragma once

#include <cstdint>
#include <string>

namespace big::network
{
	enum class join_type : std::int32_t
	{
		join_public,
		new_public,
		closed_crew,
		crew,
		closed_friends = 6,
		find_friend = 9,
		solo,
		invite_only,
		join_crew,
		sc_tv,
		leave_online = -1
	};

	struct join_status final
	{
		bool success{};
		bool pending{};
		std::string message;
	};

	[[nodiscard]] bool join_service_ready() noexcept;
	bool queue_join_type(join_type type);
	[[nodiscard]] join_status current_join_status();
}
