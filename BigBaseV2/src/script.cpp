#include "common.hpp"
#include "logger.hpp"
#include "script.hpp"

namespace big
{
	namespace
	{
		void log_script_exception(PEXCEPTION_POINTERS exception)
		{
			LOG_ERROR("Script raised a structured exception.");
			if (exception && exception->ContextRecord)
				g_stackwalker.ShowCallstack(GetCurrentThread(), exception->ContextRecord);
		}
	}

	script::script(func_t func, std::optional<std::size_t> stack_size) :
		m_func(func)
	{
		if (!m_func)
			throw std::invalid_argument("Script function cannot be null.");

		m_script_fiber = CreateFiber(
			stack_size.value_or(0),
			&script::fiber_entry,
			this);

		if (!m_script_fiber)
			throw std::runtime_error("CreateFiber failed for script.");
	}

	script::~script()
	{
		if (m_script_fiber)
			DeleteFiber(m_script_fiber);
	}

	VOID CALLBACK script::fiber_entry(void* parameter)
	{
		auto* instance = static_cast<script*>(parameter);
		if (instance)
			instance->fiber_func();
	}

	void script::tick()
	{
		if (m_finished || !m_script_fiber)
			return;

		m_main_fiber = GetCurrentFiber();
		if (!m_main_fiber)
			throw std::runtime_error("Script tick requires a converted fiber thread.");

		if (!m_wake_time || *m_wake_time <= clock::now())
			SwitchToFiber(m_script_fiber);
	}

	void script::yield(std::optional<clock::duration> time)
	{
		if (!m_main_fiber)
			throw std::runtime_error("Script attempted to yield without a main fiber.");

		m_wake_time = time ? std::optional<clock::time_point>{clock::now() + *time} : std::nullopt;
		SwitchToFiber(m_main_fiber);
	}

	script* script::get_current() noexcept
	{
		if (!IsThreadAFiber())
			return nullptr;

		return static_cast<script*>(GetFiberData());
	}

	void script::fiber_func() noexcept
	{
		__try
		{
			try
			{
				m_func();
			}
			catch (const std::exception& exception)
			{
				m_faulted = true;
				LOG_ERROR("Script threw a C++ exception: {}", exception.what());
			}
			catch (...)
			{
				m_faulted = true;
				LOG_ERROR("Script threw an unknown C++ exception.");
			}
		}
		__except (log_script_exception(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
		{
			m_faulted = true;
		}

		m_finished = true;
		LOG_INFO("Script finished{}.", m_faulted ? " with errors" : "");

		if (m_main_fiber)
			SwitchToFiber(m_main_fiber);

		for (;;)
			Sleep(INFINITE);
	}
}
