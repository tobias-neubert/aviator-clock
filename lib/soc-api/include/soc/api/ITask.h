#pragma once

#include <functional>

namespace soc
{
  namespace api
  {
    /**
     * @brief Defines the signature for a function that can be executed by a task.
     * It's a simple void function with no parameters, intended to be used with
     * lambdas that capture their own context.
     */
    using TaskFunction_t = std::function<void()>;

    /**
     * @brief Interface for an RTOS task.
     *
     * This is currently a marker interface. Its purpose is to provide a common type
     * for tasks created by the ISoc factory. The destruction of an object
     * implementing this interface should signal the underlying task to terminate
     * and join it.
     */
    class ITask
    {
    public:
      virtual ~ITask() = default;
    };
  } // namespace api
} // namespace soc
