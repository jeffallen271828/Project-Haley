################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ethernet/ethernet_commands.c \
../Core/Src/ethernet/ethernet_crc.c \
../Core/Src/ethernet/ethernet_driver.c \
../Core/Src/ethernet/ethernet_packets.c \
../Core/Src/ethernet/ethernet_protocol.c 

OBJS += \
./Core/Src/ethernet/ethernet_commands.o \
./Core/Src/ethernet/ethernet_crc.o \
./Core/Src/ethernet/ethernet_driver.o \
./Core/Src/ethernet/ethernet_packets.o \
./Core/Src/ethernet/ethernet_protocol.o 

C_DEPS += \
./Core/Src/ethernet/ethernet_commands.d \
./Core/Src/ethernet/ethernet_crc.d \
./Core/Src/ethernet/ethernet_driver.d \
./Core/Src/ethernet/ethernet_packets.d \
./Core/Src/ethernet/ethernet_protocol.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/ethernet/%.o Core/Src/ethernet/%.su Core/Src/ethernet/%.cyclo: ../Core/Src/ethernet/%.c Core/Src/ethernet/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-ethernet

clean-Core-2f-Src-2f-ethernet:
	-$(RM) ./Core/Src/ethernet/ethernet_commands.cyclo ./Core/Src/ethernet/ethernet_commands.d ./Core/Src/ethernet/ethernet_commands.o ./Core/Src/ethernet/ethernet_commands.su ./Core/Src/ethernet/ethernet_crc.cyclo ./Core/Src/ethernet/ethernet_crc.d ./Core/Src/ethernet/ethernet_crc.o ./Core/Src/ethernet/ethernet_crc.su ./Core/Src/ethernet/ethernet_driver.cyclo ./Core/Src/ethernet/ethernet_driver.d ./Core/Src/ethernet/ethernet_driver.o ./Core/Src/ethernet/ethernet_driver.su ./Core/Src/ethernet/ethernet_packets.cyclo ./Core/Src/ethernet/ethernet_packets.d ./Core/Src/ethernet/ethernet_packets.o ./Core/Src/ethernet/ethernet_packets.su ./Core/Src/ethernet/ethernet_protocol.cyclo ./Core/Src/ethernet/ethernet_protocol.d ./Core/Src/ethernet/ethernet_protocol.o ./Core/Src/ethernet/ethernet_protocol.su

.PHONY: clean-Core-2f-Src-2f-ethernet

