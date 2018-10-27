
 PARAMETER VERSION = 2.2.0


BEGIN OS
 PARAMETER OS_NAME = standalone
 PARAMETER OS_VER = 6.7
 PARAMETER PROC_INSTANCE = iop_arduino_mb
 PARAMETER stdin = iop_arduino_lmb_lmb_bram_if_cntlr
 PARAMETER stdout = iop_arduino_lmb_lmb_bram_if_cntlr
END


BEGIN PROCESSOR
 PARAMETER DRIVER_NAME = cpu
 PARAMETER DRIVER_VER = 2.7
 PARAMETER HW_INSTANCE = iop_arduino_mb
 PARAMETER compiler_flags =  -mlittle-endian -mxl-soft-mul -mcpu=v10.0
END


BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_axi_timer_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_axi_timer_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_axi_timer_2
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 3.2
 PARAMETER HW_INSTANCE = iop_arduino_axi_uartlite_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.3
 PARAMETER HW_INSTANCE = iop_arduino_gpio_subsystem_arduino_gpio
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.3
 PARAMETER HW_INSTANCE = iop_arduino_gpio_subsystem_ck_gpio
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = iic
 PARAMETER DRIVER_VER = 3.4
 PARAMETER HW_INSTANCE = iop_arduino_iic_subsystem_iic_direct
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = intc
 PARAMETER DRIVER_VER = 3.7
 PARAMETER HW_INSTANCE = iop_arduino_intc
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = intrgpio
 PARAMETER DRIVER_VER = 4.1
 PARAMETER HW_INSTANCE = iop_arduino_intr
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = io_switch
 PARAMETER DRIVER_VER = 1.0
 PARAMETER HW_INSTANCE = iop_arduino_io_switch_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = mailbox_bram
 PARAMETER DRIVER_VER = 0.1
 PARAMETER HW_INSTANCE = iop_arduino_lmb_lmb_bram_if_cntlr
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = spi
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_spi_subsystem_spi_direct
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = spi
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_spi_subsystem_spi_shared
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_2
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_3
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_4
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 4.4
 PARAMETER HW_INSTANCE = iop_arduino_timers_subsystem_timer_5
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 3.2
 PARAMETER HW_INSTANCE = iop_arduino_uartlite
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = sysmon
 PARAMETER DRIVER_VER = 7.4
 PARAMETER HW_INSTANCE = iop_arduino_xadc
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = emacps
 PARAMETER DRIVER_VER = 3.7
 PARAMETER HW_INSTANCE = ps7_ethernet_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpiops
 PARAMETER DRIVER_VER = 3.3
 PARAMETER HW_INSTANCE = ps7_gpio_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = iicps
 PARAMETER DRIVER_VER = 3.7
 PARAMETER HW_INSTANCE = ps7_i2c_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER DRIVER_VER = 2.0
 PARAMETER HW_INSTANCE = ps7_m_axi_gp0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER DRIVER_VER = 2.0
 PARAMETER HW_INSTANCE = ps7_m_axi_gp1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = qspips
 PARAMETER DRIVER_VER = 3.4
 PARAMETER HW_INSTANCE = ps7_qspi_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER DRIVER_VER = 2.0
 PARAMETER HW_INSTANCE = ps7_qspi_linear_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = sdps
 PARAMETER DRIVER_VER = 3.5
 PARAMETER HW_INSTANCE = ps7_sd_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartps
 PARAMETER DRIVER_VER = 3.6
 PARAMETER HW_INSTANCE = ps7_uart_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = usbps
 PARAMETER DRIVER_VER = 2.4
 PARAMETER HW_INSTANCE = ps7_usb_0
END


BEGIN LIBRARY
 PARAMETER LIBRARY_NAME = pynqmb
 PARAMETER LIBRARY_VER = 1.0
 PARAMETER PROC_INSTANCE = iop_arduino_mb
END


BEGIN LIBRARY
 PARAMETER LIBRARY_NAME = pynquino
 PARAMETER LIBRARY_VER = 1.0
 PARAMETER PROC_INSTANCE = iop_arduino_mb
END


