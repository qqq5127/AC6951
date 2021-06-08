


ifeq ($(APP_CASE),soundbox)
objs += \
	  $(ROOT)/cpu/$(CPU)/ui_driver/ui_common.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/led7/led7_driver.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_seg/lcd_seg3x9_driver.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/LED_1888/LED1888.o \

objs += \
	  $(ROOT)/cpu/$(CPU)/ui_driver/interface/ui_platform.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/lcd_drive.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7735s.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7789vw.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7789v.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_oled.o \

else ifeq ($(APP_CASE),watch)
objs += \
	  $(ROOT)/cpu/$(CPU)/ui_driver/ui_common.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/led7/led7_driver.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_seg/lcd_seg3x9_driver.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/LED_1888/LED1888.o \

objs += \
	  $(ROOT)/cpu/$(CPU)/ui_driver/interface/ui_platform.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/lcd_drive.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7735s.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7789vw.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_st7789v.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_rm69330.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/qspi_lcd_rm69330.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_lcd_gc9a01.o \
	  $(ROOT)/cpu/$(CPU)/ui_driver/lcd_spi/spi_oled.o \

endif

	  # $(ROOT)/cpu/$(CPU)/ui_driver/oled/oled.o \
