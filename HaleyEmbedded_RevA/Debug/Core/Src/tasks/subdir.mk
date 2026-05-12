################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/tasks/task_audio.c \
../Core/Src/tasks/task_command.c \
../Core/Src/tasks/task_diagnostics.c \
../Core/Src/tasks/task_ethernet.c \
../Core/Src/tasks/task_led.c \
../Core/Src/tasks/task_sensor.c \
../Core/Src/tasks/task_system_state.c \
../Core/Src/tasks/task_watchdog.c 

OBJS += \
./Core/Src/tasks/task_audio.o \
./Core/Src/tasks/task_command.o \
./Core/Src/tasks/task_diagnostics.o \
./Core/Src/tasks/task_ethernet.o \
./Core/Src/tasks/task_led.o \
./Core/Src/tasks/task_sensor.o \
./Core/Src/tasks/task_system_state.o \
./Core/Src/tasks/task_watchdog.o 

C_DEPS += \
./Core/Src/tasks/task_audio.d \
./Core/Src/tasks/task_command.d \
./Core/Src/tasks/task_diagnostics.d \
./Core/Src/tasks/task_ethernet.d \
./Core/Src/tasks/task_led.d \
./Core/Src/tasks/task_sensor.d \
./Core/Src/tasks/task_system_state.d \
./Core/Src/tasks/task_watchdog.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tasks/%.o Core/Src/tasks/%.su Core/Src/tasks/%.cyclo: ../Core/Src/tasks/%.c Core/Src/tasks/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-tasks

clean-Core-2f-Src-2f-tasks:
	-$(RM) ./Core/Src/tasks/task_audio.cyclo ./Core/Src/tasks/task_audio.d ./Core/Src/tasks/task_audio.o ./Core/Src/tasks/task_audio.su ./Core/Src/tasks/task_command.cyclo ./Core/Src/tasks/task_command.d ./Core/Src/tasks/task_command.o ./Core/Src/tasks/task_command.su ./Core/Src/tasks/task_diagnostics.cyclo ./Core/Src/tasks/task_diagnostics.d ./Core/Src/tasks/task_diagnostics.o ./Core/Src/tasks/task_diagnostics.su ./Core/Src/tasks/task_ethernet.cyclo ./Core/Src/tasks/task_ethernet.d ./Core/Src/tasks/task_ethernet.o ./Core/Src/tasks/task_ethernet.su ./Core/Src/tasks/task_led.cyclo ./Core/Src/tasks/task_led.d ./Core/Src/tasks/task_led.o ./Core/Src/tasks/task_led.su ./Core/Src/tasks/task_sensor.cyclo ./Core/Src/tasks/task_sensor.d ./Core/Src/tasks/task_sensor.o ./Core/Src/tasks/task_sensor.su ./Core/Src/tasks/task_system_state.cyclo ./Core/Src/tasks/task_system_state.d ./Core/Src/tasks/task_system_state.o ./Core/Src/tasks/task_system_state.su ./Core/Src/tasks/task_watchdog.cyclo ./Core/Src/tasks/task_watchdog.d ./Core/Src/tasks/task_watchdog.o ./Core/Src/tasks/task_watchdog.su

.PHONY: clean-Core-2f-Src-2f-tasks

