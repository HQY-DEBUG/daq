# 系统时钟
# set_property -dict {PACKAGE_PIN U18 IOSTANDARD LVCMOS33} [get_ports pl_clk]
# 系统复位
set_property -dict {PACKAGE_PIN N16 IOSTANDARD LVCMOS33} [get_ports pl_rstn]

# 输入引脚
set_property -dict {PACKAGE_PIN V15 IOSTANDARD LVCMOS33} [get_ports rxdin]

# 按键
set_property -dict {PACKAGE_PIN L14 IOSTANDARD LVCMOS33} [get_ports ps_ddr_clear]

set_property -dict {PACKAGE_PIN V5  IOSTANDARD LVCMOS33} [get_ports laser_on]


# 底板 LED
set_property -dict {PACKAGE_PIN H15 IOSTANDARD LVCMOS33} [get_ports led0]
set_property -dict {PACKAGE_PIN L15 IOSTANDARD LVCMOS33} [get_ports led1]
# 核心板 LED
# set_property -dict {PACKAGE_PIN J16 IOSTANDARD LVCMOS33} [get_ports led]

