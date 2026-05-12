################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/diagnostics/asserts.c \
../Core/Src/diagnostics/fault_manager.c \
../Core/Src/diagnostics/logger.c \
../Core/Src/diagnostics/runtime_stats.c \
../Core/Src/diagnostics/trace.c 

OBJS += \
./Core/Src/diagnostics/asserts.o \
./Core/Src/diagnostics/fault_manager.o \
./Core/Src/diagnostics/logger.o \
./Core/Src/diagnostics/runtime_stats.o \
./Core/Src/diagnostics/trace.o 

C_DEPS += \
./Core/Src/diagnostics/asserts.d \
./Core/Src/diagnostics/fault_manager.d \
./Core/Src/diagnostics/logger.d \
./Core/Src/diagnostics/runtime_stats.d \
./Core/Src/diagnostics/trace.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/diagnostics/%.o Core/Src/diagnostics/%.su Core/Src/diagnostics/%.cyclo: ../Core/Src/diagnostics/%.c Core/Src/diagnostics/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-diagnostics

clean-Core-2f-Src-2f-diagnostics:
	-$(RM) ./Core/Src/diagnostics/asserts.cyclo ./Core/Src/diagnostics/asserts.d ./Core/Src/diagnostics/asserts.o ./Core/Src/diagnostics/asserts.su ./Core/Src/diagnostics/fault_manager.cyclo ./Core/Src/diagnostics/fault_manager.d ./Core/Src/diagnostics/fault_manager.o ./Core/Src/diagnostics/fault_manager.su ./Core/Src/diagnostics/logger.cyclo ./Core/Src/diagnostics/logger.d ./Core/Src/diagnostics/logger.o ./Core/Src/diagnostics/logger.su ./Core/Src/diagnostics/runtime_stats.cyclo ./Core/Src/diagnostics/runtime_stats.d ./Core/Src/diagnostics/runtime_stats.o ./Core/Src/diagnostics/runtime_stats.su ./Core/Src/diagnostics/trace.cyclo ./Core/Src/diagnostics/trace.d ./Core/Src/diagnostics/trace.o ./Core/Src/diagnostics/trace.su

.PHONY: clean-Core-2f-Src-2f-diagnostics

