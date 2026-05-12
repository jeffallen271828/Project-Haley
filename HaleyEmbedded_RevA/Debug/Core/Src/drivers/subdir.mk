################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/drivers/button_driver.c \
../Core/Src/drivers/fan_driver.c \
../Core/Src/drivers/led_driver.c \
../Core/Src/drivers/rtc_driver.c \
../Core/Src/drivers/sensor_driver.c \
../Core/Src/drivers/watchdog_driver.c 

OBJS += \
./Core/Src/drivers/button_driver.o \
./Core/Src/drivers/fan_driver.o \
./Core/Src/drivers/led_driver.o \
./Core/Src/drivers/rtc_driver.o \
./Core/Src/drivers/sensor_driver.o \
./Core/Src/drivers/watchdog_driver.o 

C_DEPS += \
./Core/Src/drivers/button_driver.d \
./Core/Src/drivers/fan_driver.d \
./Core/Src/drivers/led_driver.d \
./Core/Src/drivers/rtc_driver.d \
./Core/Src/drivers/sensor_driver.d \
./Core/Src/drivers/watchdog_driver.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/drivers/%.o Core/Src/drivers/%.su Core/Src/drivers/%.cyclo: ../Core/Src/drivers/%.c Core/Src/drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-drivers

clean-Core-2f-Src-2f-drivers:
	-$(RM) ./Core/Src/drivers/button_driver.cyclo ./Core/Src/drivers/button_driver.d ./Core/Src/drivers/button_driver.o ./Core/Src/drivers/button_driver.su ./Core/Src/drivers/fan_driver.cyclo ./Core/Src/drivers/fan_driver.d ./Core/Src/drivers/fan_driver.o ./Core/Src/drivers/fan_driver.su ./Core/Src/drivers/led_driver.cyclo ./Core/Src/drivers/led_driver.d ./Core/Src/drivers/led_driver.o ./Core/Src/drivers/led_driver.su ./Core/Src/drivers/rtc_driver.cyclo ./Core/Src/drivers/rtc_driver.d ./Core/Src/drivers/rtc_driver.o ./Core/Src/drivers/rtc_driver.su ./Core/Src/drivers/sensor_driver.cyclo ./Core/Src/drivers/sensor_driver.d ./Core/Src/drivers/sensor_driver.o ./Core/Src/drivers/sensor_driver.su ./Core/Src/drivers/watchdog_driver.cyclo ./Core/Src/drivers/watchdog_driver.d ./Core/Src/drivers/watchdog_driver.o ./Core/Src/drivers/watchdog_driver.su

.PHONY: clean-Core-2f-Src-2f-drivers

