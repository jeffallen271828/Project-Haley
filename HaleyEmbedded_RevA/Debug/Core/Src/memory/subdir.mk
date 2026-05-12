################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/memory/cache_control.c \
../Core/Src/memory/dma_memory.c \
../Core/Src/memory/memory_pool.c \
../Core/Src/memory/ringbuffer.c 

OBJS += \
./Core/Src/memory/cache_control.o \
./Core/Src/memory/dma_memory.o \
./Core/Src/memory/memory_pool.o \
./Core/Src/memory/ringbuffer.o 

C_DEPS += \
./Core/Src/memory/cache_control.d \
./Core/Src/memory/dma_memory.d \
./Core/Src/memory/memory_pool.d \
./Core/Src/memory/ringbuffer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/memory/%.o Core/Src/memory/%.su Core/Src/memory/%.cyclo: ../Core/Src/memory/%.c Core/Src/memory/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-memory

clean-Core-2f-Src-2f-memory:
	-$(RM) ./Core/Src/memory/cache_control.cyclo ./Core/Src/memory/cache_control.d ./Core/Src/memory/cache_control.o ./Core/Src/memory/cache_control.su ./Core/Src/memory/dma_memory.cyclo ./Core/Src/memory/dma_memory.d ./Core/Src/memory/dma_memory.o ./Core/Src/memory/dma_memory.su ./Core/Src/memory/memory_pool.cyclo ./Core/Src/memory/memory_pool.d ./Core/Src/memory/memory_pool.o ./Core/Src/memory/memory_pool.su ./Core/Src/memory/ringbuffer.cyclo ./Core/Src/memory/ringbuffer.d ./Core/Src/memory/ringbuffer.o ./Core/Src/memory/ringbuffer.su

.PHONY: clean-Core-2f-Src-2f-memory

