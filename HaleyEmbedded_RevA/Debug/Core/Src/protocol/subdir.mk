################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/protocol/packet_parser.c \
../Core/Src/protocol/packet_protocol.c \
../Core/Src/protocol/packet_serializer.c 

OBJS += \
./Core/Src/protocol/packet_parser.o \
./Core/Src/protocol/packet_protocol.o \
./Core/Src/protocol/packet_serializer.o 

C_DEPS += \
./Core/Src/protocol/packet_parser.d \
./Core/Src/protocol/packet_protocol.d \
./Core/Src/protocol/packet_serializer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/protocol/%.o Core/Src/protocol/%.su Core/Src/protocol/%.cyclo: ../Core/Src/protocol/%.c Core/Src/protocol/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-protocol

clean-Core-2f-Src-2f-protocol:
	-$(RM) ./Core/Src/protocol/packet_parser.cyclo ./Core/Src/protocol/packet_parser.d ./Core/Src/protocol/packet_parser.o ./Core/Src/protocol/packet_parser.su ./Core/Src/protocol/packet_protocol.cyclo ./Core/Src/protocol/packet_protocol.d ./Core/Src/protocol/packet_protocol.o ./Core/Src/protocol/packet_protocol.su ./Core/Src/protocol/packet_serializer.cyclo ./Core/Src/protocol/packet_serializer.d ./Core/Src/protocol/packet_serializer.o ./Core/Src/protocol/packet_serializer.su

.PHONY: clean-Core-2f-Src-2f-protocol

