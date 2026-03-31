
################################################################
# This is a generated script based on design: top
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2025.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   if { [string compare $scripts_vivado_version $current_vivado_version] > 0 } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2042 -severity "ERROR" " This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Sourcing the script failed since it was created with a future version of Vivado."}

   } else {
     catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   }

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source top_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xc7a35tcpg236-1
   set_property BOARD_PART digilentinc.com:cmod_a7-35t:part0:1.2 [current_project]
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name top

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_gid_msg -ssname BD::TCL -id 2001 -severity "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_gid_msg -ssname BD::TCL -id 2002 -severity "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_gid_msg -ssname BD::TCL -id 2003 -severity "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_gid_msg -ssname BD::TCL -id 2004 -severity "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_gid_msg -ssname BD::TCL -id 2005 -severity "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_gid_msg -ssname BD::TCL -id 2006 -severity "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:microblaze_riscv:1.0\
xilinx.com:ip:smartconnect:1.0\
xilinx.com:ip:axi_intc:4.1\
xilinx.com:inline_hdl:ilconcat:1.0\
xilinx.com:ip:mdm_riscv:1.0\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:axi_timer:2.0\
xilinx.com:ip:axi_gpio:2.0\
xilinx.com:ip:axi_uart16550:2.0\
xilinx.com:ip:axi_quad_spi:3.2\
xilinx.com:ip:axi_emc:3.0\
xilinx.com:ip:xadc_wiz:3.3\
xilinx.com:ip:lmb_v10:3.0\
xilinx.com:ip:lmb_bram_if_cntlr:4.0\
xilinx.com:ip:blk_mem_gen:8.4\
"

   set list_ips_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2012 -severity "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

if { $bCheckIPsPassed != 1 } {
  common::send_gid_msg -ssname BD::TCL -id 2023 -severity "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################


# Hierarchical cell: microblaze_riscv_0_local_memory
proc create_hier_cell_microblaze_riscv_0_local_memory { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_microblaze_riscv_0_local_memory() - Empty argument(s)!"}
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 DLMB

  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 ILMB


  # Create pins
  create_bd_pin -dir I -type clk LMB_Clk
  create_bd_pin -dir I -type rst SYS_Rst

  # Create instance: dlmb_v10, and set properties
  set dlmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 dlmb_v10 ]

  # Create instance: ilmb_v10, and set properties
  set ilmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 ilmb_v10 ]

  # Create instance: dlmb_bram_if_cntlr, and set properties
  set dlmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 dlmb_bram_if_cntlr ]
  set_property CONFIG.C_ECC {0} $dlmb_bram_if_cntlr


  # Create instance: ilmb_bram_if_cntlr, and set properties
  set ilmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 ilmb_bram_if_cntlr ]
  set_property CONFIG.C_ECC {0} $ilmb_bram_if_cntlr


  # Create instance: lmb_bram, and set properties
  set lmb_bram [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 lmb_bram ]
  set_property -dict [list \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Use_RSTB_Pin {true} \
    CONFIG.use_bram_block {BRAM_Controller} \
  ] $lmb_bram


  # Create interface connections
  connect_bd_intf_net -intf_net microblaze_riscv_0_dlmb [get_bd_intf_pins dlmb_v10/LMB_M] [get_bd_intf_pins DLMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_dlmb_bus [get_bd_intf_pins dlmb_v10/LMB_Sl_0] [get_bd_intf_pins dlmb_bram_if_cntlr/SLMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_dlmb_cntlr [get_bd_intf_pins dlmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTA]
  connect_bd_intf_net -intf_net microblaze_riscv_0_ilmb [get_bd_intf_pins ilmb_v10/LMB_M] [get_bd_intf_pins ILMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_ilmb_bus [get_bd_intf_pins ilmb_v10/LMB_Sl_0] [get_bd_intf_pins ilmb_bram_if_cntlr/SLMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_ilmb_cntlr [get_bd_intf_pins ilmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTB]

  # Create port connections
  connect_bd_net -net SYS_Rst_1  [get_bd_pins SYS_Rst] \
  [get_bd_pins dlmb_bram_if_cntlr/LMB_Rst] \
  [get_bd_pins dlmb_v10/SYS_Rst] \
  [get_bd_pins ilmb_bram_if_cntlr/LMB_Rst] \
  [get_bd_pins ilmb_v10/SYS_Rst]
  connect_bd_net -net microblaze_riscv_0_Clk  [get_bd_pins LMB_Clk] \
  [get_bd_pins dlmb_bram_if_cntlr/LMB_Clk] \
  [get_bd_pins dlmb_v10/LMB_Clk] \
  [get_bd_pins ilmb_bram_if_cntlr/LMB_Clk] \
  [get_bd_pins ilmb_v10/LMB_Clk]

  # Restore current instance
  current_bd_instance $oldCurInst
}


# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set led_2bits [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 led_2bits ]

  set rgb_led [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 rgb_led ]

  set push_buttons_1bit [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 push_buttons_1bit ]

  set gpio_A [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_A ]

  set gpio_B [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_B ]

  set gpio_C [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_C ]

  set gpio_D [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_D ]

  set intr [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 intr ]

  set qspi_flash [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:spi_rtl:1.0 qspi_flash ]

  set cellular_ram [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:emc_rtl:1.0 cellular_ram ]


  # Create ports
  set reset [ create_bd_port -dir I -type rst reset ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $reset
  set sys_clock [ create_bd_port -dir I -type clk -freq_hz 12000000 sys_clock ]
  set_property -dict [ list \
   CONFIG.PHASE {0.0} \
 ] $sys_clock
  set pwm_0 [ create_bd_port -dir O pwm_0 ]
  set pwm_2 [ create_bd_port -dir O pwm_2 ]
  set pwm_1 [ create_bd_port -dir O pwm_1 ]
  set uart_0_rx [ create_bd_port -dir I uart_0_rx ]
  set uart_0_tx [ create_bd_port -dir O uart_0_tx ]
  set uart_1_rx [ create_bd_port -dir I uart_1_rx ]
  set uart_1_tx [ create_bd_port -dir O uart_1_tx ]
  set vauxn4 [ create_bd_port -dir I vauxn4 ]
  set vauxp4 [ create_bd_port -dir I vauxp4 ]
  set vauxn12 [ create_bd_port -dir I vauxn12 ]
  set vauxp12 [ create_bd_port -dir I vauxp12 ]

  # Create instance: microblaze_riscv_0, and set properties
  set microblaze_riscv_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze_riscv:1.0 microblaze_riscv_0 ]
  set_property -dict [list \
    CONFIG.C_DEBUG_ENABLED {1} \
    CONFIG.C_D_AXI {1} \
    CONFIG.C_D_LMB {1} \
    CONFIG.C_I_LMB {1} \
    CONFIG.C_USE_BITMAN_A {1} \
    CONFIG.C_USE_BITMAN_B {1} \
    CONFIG.C_USE_BITMAN_C {1} \
    CONFIG.C_USE_BITMAN_S {1} \
    CONFIG.C_USE_DCACHE {0} \
    CONFIG.C_USE_ICACHE {0} \
    CONFIG.C_USE_MULDIV {2} \
  ] $microblaze_riscv_0


  # Create instance: microblaze_riscv_0_local_memory
  create_hier_cell_microblaze_riscv_0_local_memory [current_bd_instance .] microblaze_riscv_0_local_memory

  # Create instance: microblaze_riscv_0_axi_periph, and set properties
  set microblaze_riscv_0_axi_periph [ create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 microblaze_riscv_0_axi_periph ]
  set_property -dict [list \
    CONFIG.NUM_MI {20} \
    CONFIG.NUM_SI {1} \
  ] $microblaze_riscv_0_axi_periph


  # Create instance: microblaze_riscv_0_axi_intc, and set properties
  set microblaze_riscv_0_axi_intc [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_intc:4.1 microblaze_riscv_0_axi_intc ]
  set_property CONFIG.C_HAS_FAST {0} $microblaze_riscv_0_axi_intc


  # Create instance: microblaze_riscv_0_xlconcat, and set properties
  set microblaze_riscv_0_xlconcat [ create_bd_cell -type inline_hdl -vlnv xilinx.com:inline_hdl:ilconcat:1.0 microblaze_riscv_0_xlconcat ]
  set_property CONFIG.NUM_PORTS {6} $microblaze_riscv_0_xlconcat


  # Create instance: mdm_1, and set properties
  set mdm_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:mdm_riscv:1.0 mdm_1 ]

  # Create instance: clk_wiz_1, and set properties
  set clk_wiz_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_1 ]
  set_property -dict [list \
    CONFIG.CLKOUT1_JITTER {479.872} \
    CONFIG.CLKOUT1_PHASE_ERROR {668.310} \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {100.000} \
    CONFIG.CLK_IN1_BOARD_INTERFACE {sys_clock} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {62.500} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {7.500} \
    CONFIG.PRIM_SOURCE {Single_ended_clock_capable_pin} \
    CONFIG.USE_BOARD_FLOW {true} \
    CONFIG.USE_LOCKED {true} \
    CONFIG.USE_RESET {false} \
  ] $clk_wiz_1


  # Create instance: rst_clk_wiz_1_100M, and set properties
  set rst_clk_wiz_1_100M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_clk_wiz_1_100M ]
  set_property -dict [list \
    CONFIG.RESET_BOARD_INTERFACE {reset} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $rst_clk_wiz_1_100M


  # Create instance: timer_0, and set properties
  set timer_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 timer_0 ]
  set_property CONFIG.mode_64bit {0} $timer_0


  # Create instance: board_led_2bits, and set properties
  set board_led_2bits [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 board_led_2bits ]
  set_property -dict [list \
    CONFIG.GPIO_BOARD_INTERFACE {led_2bits} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $board_led_2bits


  # Create instance: board_button, and set properties
  set board_button [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 board_button ]
  set_property -dict [list \
    CONFIG.GPIO_BOARD_INTERFACE {push_buttons_1bit} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $board_button


  # Create instance: board_rgb, and set properties
  set board_rgb [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 board_rgb ]
  set_property -dict [list \
    CONFIG.GPIO_BOARD_INTERFACE {rgb_led} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $board_rgb


  # Create instance: PWM_0, and set properties
  set PWM_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 PWM_0 ]

  # Create instance: uart_USB, and set properties
  set uart_USB [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uart16550:2.0 uart_USB ]
  set_property -dict [list \
    CONFIG.UART_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $uart_USB


  # Create instance: PWM_1, and set properties
  set PWM_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 PWM_1 ]

  # Create instance: gpio_A_0_6, and set properties
  set gpio_A_0_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 gpio_A_0_6 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {0} \
    CONFIG.C_GPIO_WIDTH {7} \
    CONFIG.GPIO_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $gpio_A_0_6


  # Create instance: gpio_B_0_6, and set properties
  set gpio_B_0_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 gpio_B_0_6 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {0} \
    CONFIG.C_GPIO_WIDTH {7} \
    CONFIG.GPIO_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $gpio_B_0_6


  # Create instance: gpio_D_0_6, and set properties
  set gpio_D_0_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 gpio_D_0_6 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {0} \
    CONFIG.C_GPIO_WIDTH {7} \
    CONFIG.GPIO_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $gpio_D_0_6


  # Create instance: gpio_C_0_6, and set properties
  set gpio_C_0_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 gpio_C_0_6 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {0} \
    CONFIG.C_GPIO_WIDTH {7} \
    CONFIG.GPIO_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $gpio_C_0_6


  # Create instance: INT_0_3, and set properties
  set INT_0_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 INT_0_3 ]
  set_property -dict [list \
    CONFIG.C_ALL_INPUTS {1} \
    CONFIG.C_GPIO_WIDTH {4} \
    CONFIG.C_INTERRUPT_PRESENT {1} \
    CONFIG.GPIO_BOARD_INTERFACE {Custom} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $INT_0_3


  # Create instance: PWM_2, and set properties
  set PWM_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 PWM_2 ]

  # Create instance: timer_1, and set properties
  set timer_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 timer_1 ]

  # Create instance: timer_2, and set properties
  set timer_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 timer_2 ]

  # Create instance: uart_1, and set properties
  set uart_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uart16550:2.0 uart_1 ]
  set_property CONFIG.UART_BOARD_INTERFACE {Custom} $uart_1


  # Create instance: axi_quad_spi_0, and set properties
  set axi_quad_spi_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_quad_spi:3.2 axi_quad_spi_0 ]
  set_property -dict [list \
    CONFIG.QSPI_BOARD_INTERFACE {qspi_flash} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $axi_quad_spi_0


  # Create instance: axi_emc_0, and set properties
  set axi_emc_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_emc:3.0 axi_emc_0 ]
  set_property -dict [list \
    CONFIG.EMC_BOARD_INTERFACE {cellular_ram} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $axi_emc_0


  # Create instance: xadc_wiz_0, and set properties
  set xadc_wiz_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xadc_wiz:3.3 xadc_wiz_0 ]
  set_property -dict [list \
    CONFIG.ADC_CONVERSION_RATE {500} \
    CONFIG.CHANNEL_ENABLE_TEMPERATURE {true} \
    CONFIG.CHANNEL_ENABLE_VAUXP12_VAUXN12 {true} \
    CONFIG.CHANNEL_ENABLE_VAUXP4_VAUXN4 {true} \
    CONFIG.CHANNEL_ENABLE_VCCAUX {true} \
    CONFIG.CHANNEL_ENABLE_VCCINT {true} \
    CONFIG.CHANNEL_ENABLE_VP_VN {false} \
    CONFIG.EXTERNAL_MUX_CHANNEL {VP_VN} \
    CONFIG.SEQUENCER_MODE {Continuous} \
    CONFIG.SINGLE_CHANNEL_SELECTION {TEMPERATURE} \
    CONFIG.XADC_STARUP_SELECTION {channel_sequencer} \
  ] $xadc_wiz_0


  # Create interface connections
  connect_bd_intf_net -intf_net INT_0_3_GPIO [get_bd_intf_ports intr] [get_bd_intf_pins INT_0_3/GPIO]
  connect_bd_intf_net -intf_net axi_emc_0_EMC_INTF [get_bd_intf_ports cellular_ram] [get_bd_intf_pins axi_emc_0/EMC_INTF]
  connect_bd_intf_net -intf_net axi_gpio_0_GPIO [get_bd_intf_ports led_2bits] [get_bd_intf_pins board_led_2bits/GPIO]
  connect_bd_intf_net -intf_net axi_gpio_1_GPIO [get_bd_intf_ports push_buttons_1bit] [get_bd_intf_pins board_button/GPIO]
  connect_bd_intf_net -intf_net axi_gpio_2_GPIO [get_bd_intf_ports rgb_led] [get_bd_intf_pins board_rgb/GPIO]
  connect_bd_intf_net -intf_net axi_quad_spi_0_SPI_0 [get_bd_intf_ports qspi_flash] [get_bd_intf_pins axi_quad_spi_0/SPI_0]
  connect_bd_intf_net -intf_net gpio_0_7_GPIO [get_bd_intf_ports gpio_A] [get_bd_intf_pins gpio_A_0_6/GPIO]
  connect_bd_intf_net -intf_net gpio_B_0_6_GPIO [get_bd_intf_ports gpio_B] [get_bd_intf_pins gpio_B_0_6/GPIO]
  connect_bd_intf_net -intf_net gpio_C_0_6_GPIO [get_bd_intf_ports gpio_C] [get_bd_intf_pins gpio_C_0_6/GPIO]
  connect_bd_intf_net -intf_net gpio_D_0_6_GPIO [get_bd_intf_ports gpio_D] [get_bd_intf_pins gpio_D_0_6/GPIO]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_dp [get_bd_intf_pins microblaze_riscv_0_axi_periph/S00_AXI] [get_bd_intf_pins microblaze_riscv_0/M_AXI_DP]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M01_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M01_AXI] [get_bd_intf_pins timer_0/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M02_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M02_AXI] [get_bd_intf_pins board_led_2bits/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M03_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M03_AXI] [get_bd_intf_pins board_button/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M04_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M04_AXI] [get_bd_intf_pins board_rgb/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M05_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M05_AXI] [get_bd_intf_pins uart_USB/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M06_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M06_AXI] [get_bd_intf_pins PWM_0/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M07_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M07_AXI] [get_bd_intf_pins PWM_1/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M08_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M08_AXI] [get_bd_intf_pins gpio_A_0_6/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M09_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M09_AXI] [get_bd_intf_pins gpio_B_0_6/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M10_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M10_AXI] [get_bd_intf_pins gpio_C_0_6/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M11_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M11_AXI] [get_bd_intf_pins gpio_D_0_6/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M12_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M12_AXI] [get_bd_intf_pins INT_0_3/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M13_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M13_AXI] [get_bd_intf_pins PWM_2/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M14_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M14_AXI] [get_bd_intf_pins timer_1/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M15_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M15_AXI] [get_bd_intf_pins timer_2/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M16_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M16_AXI] [get_bd_intf_pins uart_1/S_AXI]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M17_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M17_AXI] [get_bd_intf_pins axi_quad_spi_0/AXI_LITE]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M18_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M18_AXI] [get_bd_intf_pins axi_emc_0/S_AXI_MEM]
  connect_bd_intf_net -intf_net microblaze_riscv_0_axi_periph_M19_AXI [get_bd_intf_pins microblaze_riscv_0_axi_periph/M19_AXI] [get_bd_intf_pins xadc_wiz_0/s_axi_lite]
  connect_bd_intf_net -intf_net microblaze_riscv_0_debug [get_bd_intf_pins mdm_1/MBDEBUG_0] [get_bd_intf_pins microblaze_riscv_0/DEBUG]
  connect_bd_intf_net -intf_net microblaze_riscv_0_dlmb_1 [get_bd_intf_pins microblaze_riscv_0/DLMB] [get_bd_intf_pins microblaze_riscv_0_local_memory/DLMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_ilmb_1 [get_bd_intf_pins microblaze_riscv_0/ILMB] [get_bd_intf_pins microblaze_riscv_0_local_memory/ILMB]
  connect_bd_intf_net -intf_net microblaze_riscv_0_intc_axi [get_bd_intf_pins microblaze_riscv_0_axi_periph/M00_AXI] [get_bd_intf_pins microblaze_riscv_0_axi_intc/s_axi]
  connect_bd_intf_net -intf_net microblaze_riscv_0_interrupt [get_bd_intf_pins microblaze_riscv_0_axi_intc/interrupt] [get_bd_intf_pins microblaze_riscv_0/INTERRUPT]

  # Create port connections
  connect_bd_net -net INT_0_3_ip2intc_irpt  [get_bd_pins INT_0_3/ip2intc_irpt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In5]
  connect_bd_net -net PWM1_pwm0  [get_bd_pins PWM_1/pwm0] \
  [get_bd_ports pwm_1]
  connect_bd_net -net PWM_0_pwm0  [get_bd_pins PWM_0/pwm0] \
  [get_bd_ports pwm_0]
  connect_bd_net -net PWM_2_pwm0  [get_bd_pins PWM_2/pwm0] \
  [get_bd_ports pwm_2]
  connect_bd_net -net axi_timer_0_interrupt  [get_bd_pins timer_0/interrupt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In0]
  connect_bd_net -net axi_uart16550_0_ip2intc_irpt  [get_bd_pins uart_1/ip2intc_irpt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In3]
  connect_bd_net -net clk_wiz_1_locked  [get_bd_pins clk_wiz_1/locked] \
  [get_bd_pins rst_clk_wiz_1_100M/dcm_locked]
  connect_bd_net -net mdm_1_debug_sys_rst  [get_bd_pins mdm_1/Debug_SYS_Rst] \
  [get_bd_pins rst_clk_wiz_1_100M/mb_debug_sys_rst]
  connect_bd_net -net microblaze_riscv_0_Clk  [get_bd_pins clk_wiz_1/clk_out1] \
  [get_bd_pins microblaze_riscv_0_local_memory/LMB_Clk] \
  [get_bd_pins timer_0/s_axi_aclk] \
  [get_bd_pins microblaze_riscv_0/Clk] \
  [get_bd_pins microblaze_riscv_0_axi_intc/s_axi_aclk] \
  [get_bd_pins microblaze_riscv_0_axi_periph/aclk] \
  [get_bd_pins rst_clk_wiz_1_100M/slowest_sync_clk] \
  [get_bd_pins board_led_2bits/s_axi_aclk] \
  [get_bd_pins board_button/s_axi_aclk] \
  [get_bd_pins board_rgb/s_axi_aclk] \
  [get_bd_pins PWM_0/s_axi_aclk] \
  [get_bd_pins uart_USB/s_axi_aclk] \
  [get_bd_pins PWM_1/s_axi_aclk] \
  [get_bd_pins gpio_A_0_6/s_axi_aclk] \
  [get_bd_pins gpio_B_0_6/s_axi_aclk] \
  [get_bd_pins gpio_C_0_6/s_axi_aclk] \
  [get_bd_pins gpio_D_0_6/s_axi_aclk] \
  [get_bd_pins INT_0_3/s_axi_aclk] \
  [get_bd_pins PWM_2/s_axi_aclk] \
  [get_bd_pins timer_1/s_axi_aclk] \
  [get_bd_pins timer_2/s_axi_aclk] \
  [get_bd_pins uart_1/s_axi_aclk] \
  [get_bd_pins axi_quad_spi_0/s_axi_aclk] \
  [get_bd_pins axi_quad_spi_0/ext_spi_clk] \
  [get_bd_pins axi_emc_0/rdclk] \
  [get_bd_pins axi_emc_0/s_axi_aclk] \
  [get_bd_pins xadc_wiz_0/s_axi_aclk]
  connect_bd_net -net microblaze_riscv_0_intr  [get_bd_pins microblaze_riscv_0_xlconcat/dout] \
  [get_bd_pins microblaze_riscv_0_axi_intc/intr]
  connect_bd_net -net reset_1  [get_bd_ports reset] \
  [get_bd_pins rst_clk_wiz_1_100M/ext_reset_in]
  connect_bd_net -net rst_clk_wiz_1_100M_bus_struct_reset  [get_bd_pins rst_clk_wiz_1_100M/bus_struct_reset] \
  [get_bd_pins microblaze_riscv_0_local_memory/SYS_Rst]
  connect_bd_net -net rst_clk_wiz_1_100M_mb_reset  [get_bd_pins rst_clk_wiz_1_100M/mb_reset] \
  [get_bd_pins microblaze_riscv_0/Reset]
  connect_bd_net -net rst_clk_wiz_1_100M_peripheral_aresetn  [get_bd_pins rst_clk_wiz_1_100M/peripheral_aresetn] \
  [get_bd_pins timer_0/s_axi_aresetn] \
  [get_bd_pins microblaze_riscv_0_axi_intc/s_axi_aresetn] \
  [get_bd_pins microblaze_riscv_0_axi_periph/aresetn] \
  [get_bd_pins board_led_2bits/s_axi_aresetn] \
  [get_bd_pins board_button/s_axi_aresetn] \
  [get_bd_pins board_rgb/s_axi_aresetn] \
  [get_bd_pins PWM_0/s_axi_aresetn] \
  [get_bd_pins uart_USB/s_axi_aresetn] \
  [get_bd_pins PWM_1/s_axi_aresetn] \
  [get_bd_pins gpio_A_0_6/s_axi_aresetn] \
  [get_bd_pins gpio_B_0_6/s_axi_aresetn] \
  [get_bd_pins gpio_C_0_6/s_axi_aresetn] \
  [get_bd_pins gpio_D_0_6/s_axi_aresetn] \
  [get_bd_pins INT_0_3/s_axi_aresetn] \
  [get_bd_pins PWM_2/s_axi_aresetn] \
  [get_bd_pins timer_1/s_axi_aresetn] \
  [get_bd_pins timer_2/s_axi_aresetn] \
  [get_bd_pins uart_1/s_axi_aresetn] \
  [get_bd_pins axi_quad_spi_0/s_axi_aresetn] \
  [get_bd_pins axi_emc_0/s_axi_aresetn] \
  [get_bd_pins xadc_wiz_0/s_axi_aresetn]
  connect_bd_net -net sin_0_1  [get_bd_ports uart_0_rx] \
  [get_bd_pins uart_USB/sin]
  connect_bd_net -net sin_0_2  [get_bd_ports uart_1_rx] \
  [get_bd_pins uart_1/sin]
  connect_bd_net -net sys_clock_1  [get_bd_ports sys_clock] \
  [get_bd_pins clk_wiz_1/clk_in1]
  connect_bd_net -net timer_1_interrupt  [get_bd_pins timer_1/interrupt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In1]
  connect_bd_net -net timer_2_interrupt  [get_bd_pins timer_2/interrupt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In2]
  connect_bd_net -net uart_1_sout  [get_bd_pins uart_1/sout] \
  [get_bd_ports uart_1_tx]
  connect_bd_net -net uart_USB_ip2intc_irpt  [get_bd_pins uart_USB/ip2intc_irpt] \
  [get_bd_pins microblaze_riscv_0_xlconcat/In4]
  connect_bd_net -net uart_USB_sout  [get_bd_pins uart_USB/sout] \
  [get_bd_ports uart_0_tx]
  connect_bd_net -net vauxn12_0_1  [get_bd_ports vauxn12] \
  [get_bd_pins xadc_wiz_0/vauxn12]
  connect_bd_net -net vauxn4_0_1  [get_bd_ports vauxn4] \
  [get_bd_pins xadc_wiz_0/vauxn4]
  connect_bd_net -net vauxp12_0_1  [get_bd_ports vauxp12] \
  [get_bd_pins xadc_wiz_0/vauxp12]
  connect_bd_net -net vauxp4_0_1  [get_bd_ports vauxp4] \
  [get_bd_pins xadc_wiz_0/vauxp4]

  # Create address segments
  assign_bd_address -offset 0x40070000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs INT_0_3/S_AXI/Reg] -force
  assign_bd_address -offset 0x41C30000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs PWM_2/S_AXI/Reg] -force
  assign_bd_address -offset 0x60000000 -range 0x02000000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs axi_emc_0/S_AXI_MEM/MEM0] -force
  assign_bd_address -offset 0x40000000 -range 0x00010000 -with_name SEG_axi_gpio_0_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs board_led_2bits/S_AXI/Reg] -force
  assign_bd_address -offset 0x40010000 -range 0x00010000 -with_name SEG_axi_gpio_1_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs board_button/S_AXI/Reg] -force
  assign_bd_address -offset 0x40020000 -range 0x00010000 -with_name SEG_axi_gpio_2_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs board_rgb/S_AXI/Reg] -force
  assign_bd_address -offset 0x44A20000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs axi_quad_spi_0/AXI_LITE/Reg] -force
  assign_bd_address -offset 0x41C00000 -range 0x00010000 -with_name SEG_axi_timer_0_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs timer_0/S_AXI/Reg] -force
  assign_bd_address -offset 0x41C10000 -range 0x00010000 -with_name SEG_axi_timer_1_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs PWM_0/S_AXI/Reg] -force
  assign_bd_address -offset 0x44A00000 -range 0x00010000 -with_name SEG_axi_uart16550_0_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs uart_USB/S_AXI/Reg] -force
  assign_bd_address -offset 0x44A10000 -range 0x00010000 -with_name SEG_axi_uart16550_0_Reg_1 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs uart_1/S_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x00020000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs microblaze_riscv_0_local_memory/dlmb_bram_if_cntlr/SLMB/Mem] -force
  assign_bd_address -offset 0x40030000 -range 0x00010000 -with_name SEG_gpio_0_7_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs gpio_A_0_6/S_AXI/Reg] -force
  assign_bd_address -offset 0x40040000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs gpio_B_0_6/S_AXI/Reg] -force
  assign_bd_address -offset 0x40050000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs gpio_C_0_6/S_AXI/Reg] -force
  assign_bd_address -offset 0x40060000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs gpio_D_0_6/S_AXI/Reg] -force
  assign_bd_address -offset 0x41200000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs microblaze_riscv_0_axi_intc/S_AXI/Reg] -force
  assign_bd_address -offset 0x41C40000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs timer_1/S_AXI/Reg] -force
  assign_bd_address -offset 0x41C20000 -range 0x00010000 -with_name SEG_timer_2_Reg -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs PWM_1/S_AXI/Reg] -force
  assign_bd_address -offset 0x41C50000 -range 0x00010000 -with_name SEG_timer_2_Reg_1 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs timer_2/S_AXI/Reg] -force
  assign_bd_address -offset 0x44A30000 -range 0x00010000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Data] [get_bd_addr_segs xadc_wiz_0/s_axi_lite/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x00020000 -target_address_space [get_bd_addr_spaces microblaze_riscv_0/Instruction] [get_bd_addr_segs microblaze_riscv_0_local_memory/ilmb_bram_if_cntlr/SLMB/Mem] -force


  # Restore current instance
  current_bd_instance $oldCurInst

  validate_bd_design
  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


