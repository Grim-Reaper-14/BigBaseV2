#pragma once

#include "fwddec.hpp"
#include "script_id.hpp"

#include <cstddef>

namespace rage
{
	class scriptResource
	{
	public:
		virtual ~scriptResource() = default;
	};

	class scriptHandlerNetComponent
	{
	public:
		virtual ~scriptHandlerNetComponent() = default;

		scriptHandler* m_script_handler{}; // 0x08
	};

	// Verified GTA V Enhanced vtable prefix. Do not append guessed virtual
	// methods: doing so makes calls land in unrelated game slots.
	class scriptHandler
	{
	public:
		virtual ~scriptHandler() = default;   // 0 (0x00)
		virtual bool unknown_0x08() = 0;      // 1 (0x08)
		virtual void unknown_0x10() = 0;      // 2 (0x10)
		virtual void cleanup_objects() = 0;   // 3 (0x18)
		virtual scriptId* unknown_0x20() = 0; // 4 (0x20)
		virtual scriptId* get_id() = 0;       // 5 (0x28)
	};

	class scriptHandlerMgr
	{
	public:
		virtual ~scriptHandlerMgr() = default;
	};

	static_assert(sizeof(scriptHandlerNetComponent) == 0x10);
	static_assert(sizeof(scriptHandler) == sizeof(void*));
	static_assert(sizeof(scriptHandlerMgr) == sizeof(void*));
}

class CGameScriptHandler : public rage::scriptHandler
{
};

class CGameScriptHandlerNetwork : public CGameScriptHandler
{
};

class CGameScriptHandlerMgr : public rage::scriptHandlerMgr
{
};
