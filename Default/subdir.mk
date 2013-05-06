################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bdecode.c \
../bentypes.c \
../corsair.c 

OBJS += \
./bdecode.o \
./bentypes.o \
./corsair.o 

C_DEPS += \
./bdecode.d \
./bentypes.d \
./corsair.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../include -O0 -g -Wall `pkg-config fuse --cflags --libs` -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


