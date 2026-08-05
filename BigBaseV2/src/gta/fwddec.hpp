#pragma once

namespace rage
{
	template <typename T>
	class atArray;

	class datBase;
	class pgBase;
	class datBitBuffer;
	class sysMemAllocator;

	class scriptIdBase;
	class scriptId;
	class scriptResource;
	class scriptHandler;
	class scriptHandlerNetComponent;
	class scriptHandlerObject;
	class scriptHandlerMgr;

	struct scrProgram;
	struct scrProgramTableEntry;
	class scrProgramTable;

	struct scrThreadContext;
	class scrThread;
	class tlsContext;

	class netLoggingInterface;
	class netLogStub;
	class rlGamerInfo;
	class netConnectionManager;

	class netPlayerData;
	class nonPhysicalPlayerDataBase;
	class netPlayer;
	class netPlayerMgrBase;

	class netGameEvent;
	class netEventMgr;
	class netObject;
	class netObjectMgrBase;

	class rlGamerHandle;
	class rlSessionInfo;
	class rlSessionByGamerTaskResult;
	struct rlTaskStatus;
	class rlGetAvatarsContext;
	struct rlGetAvatarsPlayerList;

	class scrNativeCallContext;
	class scrNativeRegistration;
	class scrNativeRegistrationTable;

	class fwExtension;
	struct fwExtensionContainer;
	class fwRefAwareBase;
	class fwExtensibleBase;
	class fwEntity;
	class fwArchetype;
}

class GtaThread;

class CGameScriptId;
class CGameScriptHandler;
class CGameScriptHandlerNetwork;
class CGameScriptHandlerNetComponent;
class CGameScriptHandlerMgr;
class CNetGamePlayerDataMsg;
class CNonPhysicalPlayerData;

class CEntity;
class CDynamicEntity;
class CPhysical;

class CPed;
class CVehicle;
class CObject;
class CPickup;

class CPedFactory;
class CVehicleFactory;

class CNetGamePlayer;
class CNetworkPlayerMgr;
class CNetworkSession;
class CPlayerInfo;
