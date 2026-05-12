################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/audio/audio_dma.c \
../Core/Src/audio/audio_input.c \
../Core/Src/audio/audio_packets.c \
../Core/Src/audio/audio_ringbuffer.c \
../Core/Src/audio/vad.c 

OBJS += \
./Core/Src/audio/audio_dma.o \
./Core/Src/audio/audio_input.o \
./Core/Src/audio/audio_packets.o \
./Core/Src/audio/audio_ringbuffer.o \
./Core/Src/audio/vad.o 

C_DEPS += \
./Core/Src/audio/audio_dma.d \
./Core/Src/audio/audio_input.d \
./Core/Src/audio/audio_packets.d \
./Core/Src/audio/audio_ringbuffer.d \
./Core/Src/audio/vad.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/audio/%.o Core/Src/audio/%.su Core/Src/audio/%.cyclo: ../Core/Src/audio/%.c Core/Src/audio/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-audio

clean-Core-2f-Src-2f-audio:
	-$(RM) ./Core/Src/audio/audio_dma.cyclo ./Core/Src/audio/audio_dma.d ./Core/Src/audio/audio_dma.o ./Core/Src/audio/audio_dma.su ./Core/Src/audio/audio_input.cyclo ./Core/Src/audio/audio_input.d ./Core/Src/audio/audio_input.o ./Core/Src/audio/audio_input.su ./Core/Src/audio/audio_packets.cyclo ./Core/Src/audio/audio_packets.d ./Core/Src/audio/audio_packets.o ./Core/Src/audio/audio_packets.su ./Core/Src/audio/audio_ringbuffer.cyclo ./Core/Src/audio/audio_ringbuffer.d ./Core/Src/audio/audio_ringbuffer.o ./Core/Src/audio/audio_ringbuffer.su ./Core/Src/audio/vad.cyclo ./Core/Src/audio/vad.d ./Core/Src/audio/vad.o ./Core/Src/audio/vad.su

.PHONY: clean-Core-2f-Src-2f-audio

