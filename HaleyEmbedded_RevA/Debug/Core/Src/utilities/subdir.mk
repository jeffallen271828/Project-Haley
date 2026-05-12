################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/utilities/byte_utils.c \
../Core/Src/utilities/crc_utils.c \
../Core/Src/utilities/math_utils.c \
../Core/Src/utilities/string_utils.c \
../Core/Src/utilities/time_utils.c 

OBJS += \
./Core/Src/utilities/byte_utils.o \
./Core/Src/utilities/crc_utils.o \
./Core/Src/utilities/math_utils.o \
./Core/Src/utilities/string_utils.o \
./Core/Src/utilities/time_utils.o 

C_DEPS += \
./Core/Src/utilities/byte_utils.d \
./Core/Src/utilities/crc_utils.d \
./Core/Src/utilities/math_utils.d \
./Core/Src/utilities/string_utils.d \
./Core/Src/utilities/time_utils.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/utilities/%.o Core/Src/utilities/%.su Core/Src/utilities/%.cyclo: ../Core/Src/utilities/%.c Core/Src/utilities/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-utilities

clean-Core-2f-Src-2f-utilities:
	-$(RM) ./Core/Src/utilities/byte_utils.cyclo ./Core/Src/utilities/byte_utils.d ./Core/Src/utilities/byte_utils.o ./Core/Src/utilities/byte_utils.su ./Core/Src/utilities/crc_utils.cyclo ./Core/Src/utilities/crc_utils.d ./Core/Src/utilities/crc_utils.o ./Core/Src/utilities/crc_utils.su ./Core/Src/utilities/math_utils.cyclo ./Core/Src/utilities/math_utils.d ./Core/Src/utilities/math_utils.o ./Core/Src/utilities/math_utils.su ./Core/Src/utilities/string_utils.cyclo ./Core/Src/utilities/string_utils.d ./Core/Src/utilities/string_utils.o ./Core/Src/utilities/string_utils.su ./Core/Src/utilities/time_utils.cyclo ./Core/Src/utilities/time_utils.d ./Core/Src/utilities/time_utils.o ./Core/Src/utilities/time_utils.su

.PHONY: clean-Core-2f-Src-2f-utilities

