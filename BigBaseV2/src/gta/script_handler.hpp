#pragma once

#include "fwddec.hpp"
#include "script_id.hpp"

#include <cstddef>
#include <cstdint>

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

	// Verified GTA V Enhanced vtable prefix. The remaining object body and
	// methods are intentionally opaque: the current Enhanced reference does not
	// publish stable offsets for them, and this project only requires slots 0
	// and 6 for the main_persistent VMT hooks.
	class scriptHandler
	{
	public:
		virtual ~scriptHandler() = default;          // 0 (0x00)
		virtual bool unknown_0x08() = 0;             // 1 (0x08)
		virtual void unknown_0x10() = 0;             // 2 (0x10)
		virtual void cleanup_objects() = 0;          // 3 (0x18)
		virtual scriptId* unknown_0x20() = 0;        // 4 (0x20)
		virtual scriptId* get_id() = 0;              // 5 (0x28)
		virtual bool is_networked() = 0;             // 6 (0x30)
	};

	// Kept as an opaque interface because the Enhanced manager layout is not
	// currently verified and BigBaseV2 stores this type only as a pointer.
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
