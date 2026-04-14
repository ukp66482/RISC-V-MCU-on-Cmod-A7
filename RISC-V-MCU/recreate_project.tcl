################################################################
# recreate_project.tcl
#
# Custom wrapper script for RISC-V-MCU project.
# This file is manually maintained and will NOT be overwritten by Vivado.
#
# Usage (from Vivado Tcl console):
#   source recreate_project.tcl
#
# When the BD design is updated in Vivado, export only the BD design:
#   write_bd_tcl -force top.tcl
# Then re-run this script to recreate the full project.
################################################################

set script_folder [file dirname [file normalize [info script]]]

################################################################
# Add local board files to Vivado's board repository path
################################################################

set board_repo_path [file normalize "$script_folder/../Cmod-A7-spec/Board-Files"]
set current_repo_paths [get_param board.repoPaths]
if { [lsearch $current_repo_paths $board_repo_path] == -1 } {
   set_param board.repoPaths [concat $current_repo_paths [list $board_repo_path]]
}

################################################################
# Create project (if not already open)
################################################################

set board_part_id "digilentinc.com:cmod_a7-35t:part0:1.2"

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project RISC-V-MCU $script_folder -part xc7a35tcpg236-1
   set_property BOARD_PART $board_part_id [current_project]
}

################################################################
# Source the auto-generated BD design TCL
################################################################

source [file normalize "$script_folder/top.tcl"]

################################################################
# Add constraints
################################################################

set xdc_file [file normalize "$script_folder/../Cmod-A7-spec/Cmod-A7-Master.xdc"]
if { [file exists $xdc_file] } {
   add_files -fileset constrs_1 $xdc_file
   common::send_gid_msg -ssname BD::TCL -id 2010 -severity "INFO" "Added constraints file: $xdc_file"
} else {
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "WARNING" "Constraints file not found: $xdc_file"
}

################################################################
# Create HDL wrapper
################################################################

set design_name top
make_wrapper -files [get_files ${design_name}.bd] -top
add_files -norecurse [glob $script_folder/RISC-V-MCU.gen/sources_1/bd/${design_name}/hdl/${design_name}_wrapper.v]
set_property top ${design_name}_wrapper [current_fileset]
common::send_gid_msg -ssname BD::TCL -id 2012 -severity "INFO" "HDL wrapper created and set as top."
