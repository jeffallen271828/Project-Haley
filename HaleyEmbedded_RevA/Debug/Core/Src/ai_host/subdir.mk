################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ai_host/ai_host_commands.c \
../Core/Src/ai_host/ai_host_events.c \
../Core/Src/ai_host/ai_host_interface.c \
../Core/Src/ai_host/ai_host_state.c 

OBJS += \
./Core/Src/ai_host/ai_host_commands.o \
./Core/Src/ai_host/ai_host_events.o \
./Core/Src/ai_host/ai_host_interface.o \
./Core/Src/ai_host/ai_host_state.o 

C_DEPS += \
./Core/Src/ai_host/ai_host_commands.d \
./Core/Src/ai_host/ai_host_events.d \
./Core/Src/ai_host/ai_host_interface.d \
./Core/Src/ai_host/ai_host_state.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/ai_host/%.o Core/Src/ai_host/%.su Core/Src/ai_host/%.cyclo: ../Core/Src/ai_host/%.c Core/Src/ai_host/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-ai_host

clean-Core-2f-Src-2f-ai_host:
	-$(RM) ./Core/Src/ai_host/ai_host_commands.cyclo ./Core/Src/ai_host/ai_host_commands.d ./Core/Src/ai_host/ai_host_commands.o ./Core/Src/ai_host/ai_host_commands.su ./Core/Src/ai_host/ai_host_events.cyclo ./Core/Src/ai_host/ai_host_events.d ./Core/Src/ai_host/ai_host_events.o ./Core/Src/ai_host/ai_host_events.su ./Core/Src/ai_host/ai_host_interface.cyclo ./Core/Src/ai_host/ai_host_interface.d ./Core/Src/ai_host/ai_host_interface.o ./Core/Src/ai_host/ai_host_interface.su ./Core/Src/ai_host/ai_host_state.cyclo ./Core/Src/ai_host/ai_host_state.d ./Core/Src/ai_host/ai_host_state.o ./Core/Src/ai_host/ai_host_state.su

.PHONY: clean-Core-2f-Src-2f-ai_host

