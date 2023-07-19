LIB_DIR := ../lib
LIB := $(wildcard $(LIB_DIR)/*.so)
LDFLAGS := -L$(LIB_DIR) $(addprefix -l,$(notdir $(basename $(LIB))))
IFLAGS = -I/home/studentas/Desktop/tuyaSDK/tuya-iot-core-sdk/utils -I/home/studentas/Desktop/tuyaSDK/tuya-iot-core-sdk/interface -I/home/studentas/Desktop/tuyaSDK/tuya-iot-core-sdk/include
LDLIBS = -llink_core -lmiddleware_implementation -lplatform_port -lutils_modules


CC := gcc
CFLAGS := -Wall -Wextra

.PHONY: all clean

all: myprogram

myprogram: first_daemon.o
	$(CC) -L../lib $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	$(RM) *.o myprogram
