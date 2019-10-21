USER_INCLUDE	 =	./usrInc/
USER_SOURCE	 	=	./usrSrc


ALLINC			+=	$(USER_INCLUDE)
					
ALLCSRC			+=	$(CHIBIOS)/os/various/syscalls.c		\
					$(USER_SOURCE)/cJSON_Utils.c			\
					$(USER_SOURCE)/cJSON.c					\
					$(USER_SOURCE)/crc16.c					\
					$(USER_SOURCE)/digitalIO.c				\
					$(USER_SOURCE)/eventManager.c 			\
					$(USER_SOURCE)/global.c					\
					$(USER_SOURCE)/initialization.c 		\
					$(USER_SOURCE)/main.c					\
					$(USER_SOURCE)/modem.c					\
					$(USER_SOURCE)/rs485Communication.c		\
					$(USER_SOURCE)/rs485Process.c			\
					$(USER_SOURCE)/sdCard.c					\
					$(USER_SOURCE)/sms.c					\
					$(USER_SOURCE)/tcpDataProcess.c			\
					$(USER_SOURCE)/testSpace.c				\
					$(USER_SOURCE)/uartModem.c				\
					$(USER_SOURCE)/uartRS485.c				
