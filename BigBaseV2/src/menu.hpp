#pragma once

#include "common.hpp"

namespace big
{
	class menu
	{
	public:
		enum class page
		{
			self,
			weapons,
			vehicle,
			teleport,
			world,
			players,
			settings
		};

		void draw();
		void draw_sidebar();
		void draw_page();

		page m_current_page{page::self};
	};

	inline menu g_menu;
}
