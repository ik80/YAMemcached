################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Common.cpp \
../src/FastSubstringMatcher.cpp \
../src/RequestProcessor.cpp \
../src/Server.cpp \
../src/ServerThread.cpp \
../src/Session.cpp \
../src/YAMBinaryProtocolProcessor.cpp \
../src/YAMemcached.cpp 

C_SRCS += \
../src/xxhash.c 

OBJS += \
./src/Common.o \
./src/FastSubstringMatcher.o \
./src/RequestProcessor.o \
./src/Server.o \
./src/ServerThread.o \
./src/Session.o \
./src/YAMBinaryProtocolProcessor.o \
./src/YAMemcached.o \
./src/xxhash.o 

C_DEPS += \
./src/xxhash.d 

CPP_DEPS += \
./src/Common.d \
./src/FastSubstringMatcher.d \
./src/RequestProcessor.d \
./src/Server.d \
./src/ServerThread.d \
./src/Session.d \
./src/YAMBinaryProtocolProcessor.d \
./src/YAMemcached.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -g -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


